#include <Arduino.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>

#include "common.h" // Für anchorMac und anchorMacString
#include "NetworkModule.h"
#include "Webservermodule.h"
#include "DistanceModule.h"
#include "DisplayModule.h"
#include "Device.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

extern Device thisAnchor;
const int maxAttempts = 10;

DistanceModule* distanceModule;

NetworkModule::NetworkModule() {
    //prepare thisAnchor
    thisAnchor.id =-1;
    populateAnchorMac();

}

// Singleton-Instanz
NetworkModule& NetworkModule::getInstance() {
    static NetworkModule instance; // Statische Instanz
    return instance;
}

void NetworkModule::initializeWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("WiFi initialized in STA mode.");

    if (nvs_flash_init() == ESP_ERR_NVS_NO_FREE_PAGES || nvs_flash_init() == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    Serial.println("[INFO] NVS erfolgreich initialisiert.");


    preferences.begin("wifi_config", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    thisAnchor.id = preferences.getInt("id", 0);
    baseStationIP = preferences.getString("Bip", "");
    preferences.end();

    anchorID = thisAnchor.id;

    Serial.printf("SSID: %s, Passwort: %s\n", ssid.c_str(), password.c_str());

     if (ssid.isEmpty() || password.isEmpty()) {
        Serial.println("[ERROR] Keine gespeicherten WiFi-Daten gefunden. Wechsle zu ESP-NOW.");
        delay(100); // Warte auf Kanalstabilisierung
        initializeESPNow();
        return;
    }

    bool ipReceived = false; // Flag, um sicherzustellen, dass das IP-Ereignis korrekt verarbeitet wird

    WiFi.onEvent([&ipReceived](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("[DEBUG] WiFi Event Handler triggered.");
        if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            Serial.println("[INFO] IP address received.");
            Serial.println(WiFi.localIP());
            ipReceived = true; // Markiere, dass die IP-Adresse empfangen wurde
            NetworkModule::getInstance().sendMacAndIP();
        }
    });

    WiFi.begin(ssid, password);
    Serial.println("Verbinde mit WiFi...");

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
        delay(1000);
        attempt++;
        Serial.print("Verbindungsversuch ");
        Serial.println(attempt);
    }

        // Falls das IP-Event zu spät kommt
        if (!ipReceived) {
            Serial.println("[WARNING] IP-Ereignis noch nicht ausgelöst. Warte auf IP...");
        }

        if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WLAN verbunden, starte MQTT...");
        distanceModule = new DistanceModule(DW1000_IRQ_PIN,DW1000_RST_PIN,DW1000_CS_PIN);
        distanceModule->initialize();
        Serial.println("Erfolgreich mit WiFi verbunden!");
        WebServerModule& webServer = WebServerModule::getInstance();
        webServer.initializeAnchorServer();
        initializeESPNow();


    } else {
        Serial.println("[ERROR] Verbindung mit WiFi fehlgeschlagen. Starte ESP-NOW.");
        initializeESPNow();
    }
}


void NetworkModule::initializeESPNow() {
    static bool espNowInitialized = false;  // Verhindert doppelte Initialisierung
    static bool macAndIpSent = false;       // Verhindert doppelten Aufruf von sendMacAndIP()

    Serial.printf("DEBUG: Basisstations-MAC in NetworkModule: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  baseStationMAC[0], baseStationMAC[1], baseStationMAC[2],
                  baseStationMAC[3], baseStationMAC[4], baseStationMAC[5]);

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("current Wifi Channel: %d\n", WiFi.channel());
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    } else {
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        Serial.printf("current Wifi Channel: %d\n", WiFi.channel());
    }

    // **ESP-NOW nur initialisieren, wenn noch nicht geschehen**
    if (!espNowInitialized) {
        if (esp_now_init() != ESP_OK) {
            Serial.println("ESP-NOW Initialisierung fehlgeschlagen!");
            ESP.restart();
        }
        Serial.println("ESP-NOW erfolgreich initialisiert.");
        espNowInitialized = true;  // Verhindert erneute Initialisierung
    } else {
        Serial.println("[INFO] ESP-NOW war bereits initialisiert.");
    }

    // **Callback nur registrieren, wenn noch nicht gesetzt**
    static bool callbackRegistered = false;
    if (!callbackRegistered) {
        esp_now_register_recv_cb(NetworkModule::onDataReceive);
        Serial.println("ESP-NOW Empfangs-Callback registriert.");
        callbackRegistered = true;
    }

    // Debugging: Basisstations-MAC
    Serial.printf("Basisstations-MAC zur Peer-Registrierung: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  baseStationMAC[0], baseStationMAC[1], baseStationMAC[2],
                  baseStationMAC[3], baseStationMAC[4], baseStationMAC[5]);

    // Basisstations-Peer hinzufügen
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, baseStationMAC, 6);
    peerInfo.channel = WiFi.channel();
    peerInfo.encrypt = false;

    Serial.printf("WiFi-Kanal: %d, ESP-NOW-Kanal: %d\n", WiFi.channel(), peerInfo.channel);

    addOrUpdatePeer(baseStationMAC);

    // **Sende MAC & IP nur einmal**
    if (WiFi.status() == WL_CONNECTED && !macAndIpSent) {
        sendMacAndIP();
        macAndIpSent = true;  // Verhindert weiteren Aufruf
        Serial.println("[INFO] MAC und IP erfolgreich gesendet.");
    } else if (macAndIpSent) {
        Serial.println("[INFO] MAC und IP wurden bereits gesendet, erneuter Aufruf verhindert.");
    }
}


void NetworkModule::addOrUpdatePeer(const uint8_t* peerMAC) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMAC, 6);
    peerInfo.channel = WiFi.channel(); // Synchronisiere den Kanal
    peerInfo.encrypt = false;

    if (esp_now_is_peer_exist(peerMAC)) {
        Serial.println("[INFO] Peer already exists. Modifying instead.");
        if (esp_now_mod_peer(&peerInfo) != ESP_OK) {
            Serial.println("[ERROR] Failed to modify existing peer.");
        } else {
            Serial.println("[INFO] Peer modified successfully.");
        }
    } else {
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("[ERROR] Failed to add peer.");
        } else {
            Serial.println("[INFO] Peer added successfully.");
        }
    }
}


void NetworkModule::populateAnchorMac() {

    static uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);
     // Speichert die MAC-Adresse lokal
    Serial.printf("Anchor MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    memcpy(anchorMac, mac, 6);
    memcpy(thisAnchor.mac_address, mac, 6);

    anchorMacString = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + 
                      String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + 
                      String(mac[4], HEX) + ":" + String(mac[5], HEX);

    anchorMacString.toUpperCase();

    Serial.printf("Anchor MAC Address String: %s\n", anchorMacString.c_str());
}


void NetworkModule::sendMessage(const char* message) {
    esp_err_t result = esp_now_send(baseStationMAC, (const uint8_t*)message, strlen(message));
    if (result == ESP_OK) {
        Serial.println("Message sent successfully.");
    } else {
        Serial.printf("Failed to send message: %d\\n", result);
    }
}

void NetworkModule::sendMacAndType() {
    uint8_t data[7];
    memcpy(data, anchorMac, 6); // MAC-Adresse einfügen
    data[6] = static_cast<uint8_t>(Device::Type::Anchor); // Typ hinzufügen

    esp_err_t result = esp_now_send(baseStationMAC, data, sizeof(data));
    if (result == ESP_OK) {
        Serial.println("MAC-Adresse und Typ erfolgreich gesendet.");
        Serial.printf("sent MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  anchorMac[0], anchorMac[1], anchorMac[2], 
                  anchorMac[3], anchorMac[4], anchorMac[5]);
        Serial.printf("to Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  baseStationMAC[0], baseStationMAC[1], baseStationMAC[2], 
                  baseStationMAC[3], baseStationMAC[4], baseStationMAC[5]);
    } else {
        Serial.printf("Fehler beim Senden von MAC-Adresse und Typ: %d\n", result);
    }

    
}

void NetworkModule::sendMacAndIP() {

    unsigned long startTime = millis();
    unsigned long timeout = 10000; // Timeout von 10 Sekunden

    // Warte auf eine gültige IP-Adresse
    while (WiFi.localIP() == INADDR_NONE) {
        if (millis() - startTime > timeout) {
            Serial.println("[ERROR] Timeout: Keine gültige IP-Adresse erhalten.");
            return;
        }
        delay(100); // Kurz warten, um die CPU nicht zu blockieren
    }
    
    IPAddress ip = WiFi.localIP();

    uint8_t data[10]; // 6 Bytes für MAC, 4 Bytes für IP
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    memcpy(data, mac, 6); // MAC-Adresse einfügen

    // Konvertiere die lokale IP in 4 Bytes und füge sie ein
    //IPAddress ip = WiFi.localIP();
    data[6] = ip[0];
    data[7] = ip[1];
    data[8] = ip[2];
    data[9] = ip[3];

    //Setzen der IP im lokalen anker
    anchorIP = ip.toString();

    // Debugging: MAC und IP ausgeben
    Serial.printf("Sende MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("Sende IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

 
    // Sende Daten über ESP-NOW
    esp_err_t result = esp_now_send(baseStationMAC, data, sizeof(data));
    if (result == ESP_OK) {
        Serial.println("MAC und IP erfolgreich gesendet.");
    } else {
        Serial.printf("Fehler beim Senden von MAC und IP: %s\n", esp_err_to_name(result));
    }
}



void NetworkModule::sendJsonMessage(const char* key, const char* value) {
    JsonDocument doc;
    doc[key] = value;

    char buffer[256];
    serializeJson(doc, buffer);
    sendMessage(buffer);
}

void NetworkModule::onDataReceive(const uint8_t *mac, const uint8_t *data, int len) {
    Serial.println("Anker: ESP-NOW Empfang ausgelöst.");
    Serial.print("Empfangene Daten von MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();

    Serial.print("Daten: ");
    for (int i = 0; i < len; i++) {
        Serial.printf("%c", data[i]); // Wenn der Payload Text ist
    }
    Serial.println();

    char idBuffer[16] = {0};
    char ssid[32] = {0};
    char password[64] = {0};
    char Bip[16] = {0};

    // Prüfen, ob die Nachricht eine ID, SSID und Passwort enthält
    if (sscanf((char *)data, "ID:%15[^;];SSID:%31[^;];Password:%63[^;];Bip:%15[^;]", idBuffer, ssid, password, Bip) == 4) {
        int deviceID = atoi(idBuffer);
        if (deviceID > 0) {
            Serial.printf("Empfangene Device ID: %d\n", deviceID);
            Serial.printf("Empfangene SSID: %s\n", ssid);
            Serial.printf("Empfangenes Passwort: %s\n", password);
            Serial.printf("Empfangene IP: %s\n", Bip);

            // Speichere die WiFi-Daten
            preferences.begin("wifi_config", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.putInt("id", deviceID);
            preferences.putString("Bip", Bip);
            Serial.printf("WiFi-Einstellungen gespeichert: SSID=%s, Passwort=%s\n", ssid, password);
            preferences.end();

            baseStationIP = Bip;
            Serial.printf("BasisIP: %s\n",baseStationIP.c_str());

 /*           // Speichere die Device ID
            if (memcmp(thisAnchor.mac_address, mac, 6) == 0) {
                thisAnchor.id = deviceID; // Device ID zuordnen
                anchorID = deviceID; // zuweisen zu Globaler variable.
                saveAnchorData();
                Serial.printf("Anker %d erfolgreich mit MAC %02X:%02X:%02X:%02X:%02X:%02X registriert.\n",
                              deviceID, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
*/
            // WiFi-Verbindung herstellen
            WiFi.begin(ssid, password);
            Serial.println("Verbinde mit WiFi...");

            WebServerModule::getInstance().initializeAnchorServer();
        } else {
            Serial.println("[WARNUNG] Ungültige Device ID empfangen.");
        }
            DisplayModule displayModule; // Globale Instanz verwenden, falls vorhanden
            displayModule.showOperationalStatus();
        return;
    }

    // Andere bestehende Bedingungen
    if (strncmp((char *)data, "GET_IP", len) == 0) {
        Serial.println("[INFO] Anfrage nach IP-Adresse erkannt.");
        NetworkModule::getInstance().sendMacAndIP();
        return;
    }

    if (strncmp((char *)data, "ACK_RECEIVED", len) == 0) {
        Serial.println("[INFO] ACK received. MAC address successfully registered.");
        digitalWrite(LED_BUILTIN, HIGH); // Beispiel für eine Rückmeldung
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        TransSuccess = true;
        return;
    }
}



