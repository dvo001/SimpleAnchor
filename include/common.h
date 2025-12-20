#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <Preferences.h>
#include "Device.h"


// Pin definitions
extern const int DW1000_RST_PIN;
extern const int DW1000_CS_PIN;
extern const int DW1000_IRQ_PIN;
extern const int DW1000_MOSI_PIN;
extern const int DW1000_MISO_PIN;
extern const int DW1000_SCLK_PIN;


extern const int I2C_SDA_PIN;
extern const int I2C_SCL_PIN;

// Global variables
extern uint8_t baseStationMAC[6];
extern String baseStationIP;
extern uint8_t anchorMac[6];
extern String anchorMacString;
extern String wifiSSID;
extern int anchorID;
extern String anchorIP;
extern float distanceToTag;
extern bool TransSuccess;
extern Device thisAnchor;

extern Preferences preferences;

std::string macToString(const uint8_t* mac);

extern void debugBaseStationMAC();
extern void saveAnchorData();
extern void loadAnchorData();

#endif
