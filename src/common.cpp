
#include "Device.h"
#include "common.h"

// Pin definitions
const int DW1000_RST_PIN = 5;  // Reset Pin
const int DW1000_CS_PIN = 10;   // Chip Select Pin
const int DW1000_IRQ_PIN = 4;  // Interrupt Pin
const int DW1000_MOSI_PIN = 11;   // SPI MOSI
const int DW1000_MISO_PIN = 13;   // SPI MISO
const int DW1000_SCLK_PIN = 12;   // SPI Clock


const int I2C_SDA_PIN = 35;    // SDA Pin for I2C
const int I2C_SCL_PIN = 36;    // SCL Pin for I2C

Preferences preferences;

// Global variables
uint8_t baseStationMAC[6] = {0x02, 0xCF, 0x1A, 0x2B, 0x3C, 0x4D}; // Beispiel-Basisstations-MAC
String baseStationIP = "";
uint8_t anchorMac[6] = {};
String anchorMacString = "";
String wifiSSID = "";
String anchorIP = "0.0.0.0";
int anchorID = -1;
float distanceToTag = 0.0;
bool TransSuccess = false;

Device thisAnchor;

std::string macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(macStr);
}

void debugBaseStationMAC() {
    Serial.printf("DEBUG (common.cpp): Basisstations-MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  baseStationMAC[0], baseStationMAC[1], baseStationMAC[2],
                  baseStationMAC[3], baseStationMAC[4], baseStationMAC[5]);
}

    void loadAnchorData() {
        preferences.begin("AnchorStorage", true);
        thisAnchor.id = preferences.getInt("anchorID", 0);
        
        String temp = preferences.getString("ip_address", "");
        temp.toCharArray(thisAnchor.ip_address, sizeof(thisAnchor.ip_address));
    
        thisAnchor.calibration_offset = preferences.getFloat("calibration_offset", 0.0);
        thisAnchor.position[0] = preferences.getFloat("posX", 0.0);
        thisAnchor.position[1] = preferences.getFloat("posY", 0.0);
        thisAnchor.position[2] = preferences.getFloat("posZ", 0.0);
        preferences.end();
    }

    void saveAnchorData() {
        preferences.begin("AnchorStorage", false);
        preferences.putInt("anchorID", thisAnchor.id);
        preferences.putString("ip_address", thisAnchor.ip_address);
        preferences.putFloat("calibration_offset", thisAnchor.calibration_offset);
        preferences.putFloat("posX", thisAnchor.position[0]);
        preferences.putFloat("posY", thisAnchor.position[1]);
        preferences.putFloat("posZ", thisAnchor.position[2]);
        preferences.end();
    }
