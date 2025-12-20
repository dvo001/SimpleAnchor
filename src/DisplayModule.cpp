#include <Arduino.h>
#include "DisplayModule.h"
#include "common.h"

DisplayModule::DisplayModule() 
    : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE) {}

void DisplayModule::initialize() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf); // Example font
    Serial.println("Display initialized.");
}

void DisplayModule::showMACAddress(const String& macAddress) {
    u8g2.clearBuffer();
    String fullMacAddress = "MAC:" + macAddress;
    u8g2.drawStr(0, 12, fullMacAddress.c_str());
    u8g2.drawStr(0, 32, "Please Register on Basis..");
    u8g2.sendBuffer();
}

void DisplayModule::showOperationalStatus() {
    char distanceStr[16];
    sprintf(distanceStr, "Dist: %.2f m", distanceToTag);

    char anchorIDStr[16];
    sprintf(anchorIDStr, "%d", anchorID);

    //Serial.printf("Anchor ID: %d", thisAnchor.id);
    //Serial.printf("Anchor ID: %d", anchorID);


    u8g2.clearBuffer();

    // Top line: IP Address
    u8g2.drawStr(0, 12, ("IP: " + anchorIP).c_str());

    // Right side: Anchor ID (half the display height)
    int displayHeight = u8g2.getDisplayHeight();
    int anchorIDY = displayHeight / 2; // Half the display height
    u8g2.setFont(u8g2_font_7_Seg_33x19_mn); // Use a smaller font for the anchor ID
    u8g2.drawStr(u8g2.getDisplayWidth() - 20, anchorIDY, anchorIDStr);

    // Bottom line: Distance
    u8g2.setFont(u8g2_font_ncenB10_tr); // Restore default font
    u8g2.drawStr(0, displayHeight - 4, distanceStr);

    u8g2.sendBuffer();
}


void DisplayModule::showError(const char* message) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 32, "Error:");
    u8g2.drawStr(0, 52, message);
    u8g2.sendBuffer();
}
