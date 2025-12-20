#ifndef DISTANCE_MODULE_H
#define DISTANCE_MODULE_H

#include <Arduino.h>
#include <DW1000.h>
#include <DW1000Ranging.h>
#include "MQTTModule.h"
#include "common.h"

class DistanceModule {
public:
    DistanceModule(int irq, int rst, int cs);
    void initialize();
    float getDistanceToTag();
    void startDistanceMeasurementTask();
    void startDW1000Task();
    void startCalibration();

private:
    int irqPin;
    int resetPin;
    int csPin;
    
    static TaskHandle_t dw1000TaskHandle;
    static void dw1000Task(void* param);
    static void IRAM_ATTR handleDW1000Interrupt();
    
    MQTTModule* mqttModule;
};

#endif // DISTANCE_MODULE_H
