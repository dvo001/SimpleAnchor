#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino.h>

class DisplayModule {
public:
    DisplayModule();
    void initialize();
    void showMACAddress(const String& macAddress);
    void showOperationalStatus();
    void showError(const char* message);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
};

#endif
