#include <Arduino.h>
#include <SPI.h>
#include <DW1000.h>

// ESP32-S3 DevKit C -> DW1000 Adapter (dein Mapping)
static constexpr uint8_t PIN_RST  = 5;   // RSTN
static constexpr uint8_t PIN_IRQ  = 4;   // IRQ
static constexpr uint8_t PIN_SS   = 10;  // CSN

static constexpr uint8_t PIN_SCK  = 12;  // CLK
static constexpr uint8_t PIN_MISO = 13;  // MISO
static constexpr uint8_t PIN_MOSI = 11;  // MOSI

static void dumpBytes(const uint8_t* b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (b[i] < 16) Serial.print('0');
    Serial.print(b[i], HEX);
    if (i + 1 < n) Serial.print(':');
  }
}

static bool readDw1000Basics() {
  // 1) DW1000 Core-Init über Library
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);

  // 2) Setup (setzt Standard-Konfigurationen + SPI Timing)
  DW1000.setup();

  // 3) Register lesen: Device-ID (DEV_ID, 4 Byte)
  uint8_t devId[4] = {0};
  DW1000.readBytes(DEV_ID, NO_SUB, devId, sizeof(devId));

  Serial.print("DEV_ID: 0x");
  for (int i = 3; i >= 0; --i) { // DW1000 often displayed MSB..LSB
    if (devId[i] < 16) Serial.print('0');
    Serial.print(devId[i], HEX);
  }
  Serial.println();

  // Grober Plausibilitätscheck: Wenn SPI/CSN/MISO/MOSI falsch sind,
  // sieht man hier oft 0x00000000 oder 0xFFFFFFFF oder wechselnden Müll.
  bool all00 = true, allFF = true;
  for (auto v : devId) {
    if (v != 0x00) all00 = false;
    if (v != 0xFF) allFF = false;
  }
  if (all00 || allFF) {
    Serial.println("WARNUNG: DEV_ID wirkt unplausibel (alles 00 oder FF). Prüfe CSN/MISO/MOSI/CLK, 3V3, GND, RSTN.");
    return false;
  }

  // 4) EUI (8 Byte) – kann 00 sein, je nach Modul/OTP, aber Lesen muss funktionieren
  uint8_t eui[8] = {0};
  DW1000.getEUI(eui);
  Serial.print("EUI: ");
  dumpBytes(eui, sizeof(eui));
  Serial.println();

  // 5) Status Register (SYS_STATUS, 5 Byte) auslesen – auch nur als SPI-Lesetest
  uint8_t sysStatus[5] = {0};
  DW1000.readBytes(SYS_STATUS, NO_SUB, sysStatus, sizeof(sysStatus));
  Serial.print("SYS_STATUS: 0x");
  for (int i = 4; i >= 0; --i) {
    if (sysStatus[i] < 16) Serial.print('0');
    Serial.print(sysStatus[i], HEX);
  }
  Serial.println();

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("DW1000/MAX2001 SPI Test (ESP32-S3 DevKit C)");
  Serial.println("-----------------------------------------");

  // ESP32-S3: explizite SPI-Pins setzen
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

  // Optional: sauberen Reset-Puls geben (RSTN ist aktiv-low)
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  delay(10);
  digitalWrite(PIN_RST, HIGH);
  delay(10);

  bool ok = false;
  for (int attempt = 1; attempt <= 3; ++attempt) {
    Serial.print("Init attempt ");
    Serial.print(attempt);
    Serial.println("...");
    ok = readDw1000Basics();
    if (ok) break;
    delay(300);
  }

  if (ok) {
    Serial.println("OK: DW1000 antwortet plausibel ueber SPI.");
  } else {
    Serial.println("FEHLER: Keine plausible Antwort. Verdrahtung/Versorgung prüfen.");
  }
}

void loop() {
  // Alle 2 Sekunden DEV_ID erneut lesen (stabiler SPI-Test)
  uint8_t devId[4] = {0};
  DW1000.readBytes(DEV_ID, NO_SUB, devId, sizeof(devId));

  Serial.print("Heartbeat DEV_ID: 0x");
  for (int i = 3; i >= 0; --i) {
    if (devId[i] < 16) Serial.print('0');
    Serial.print(devId[i], HEX);
  }
  Serial.println();

  delay(2000);
}
