#include "DistanceModule.h"
#include "MQTTModule.h"
#include <DW1000.h>
#include <DW1000Ranging.h>
#include <Arduino.h>
#include "common.h"

 MQTTModule* mqttModule;  // MQTTModule-Instanz hinzufügen

DistanceModule::DistanceModule(int irq, int rst, int cs) 
    : irqPin(irq), resetPin(rst), csPin(cs) {
}

void DistanceModule::initialize() {
    DW1000.begin(irqPin, resetPin);
    DW1000.select(csPin);
    DW1000Ranging.initCommunication(resetPin, csPin, irqPin);
    Serial.println("Distance module initialized.");

    // Initialisiere MQTT erst jetzt!
    mqttModule = new MQTTModule(baseStationIP.c_str(), 1883, "client_id");
    mqttModule->begin();
}

float DistanceModule::getDistanceToTag() {
    auto device = DW1000Ranging.getDistantDevice();
    if (device == nullptr) {
        Serial.println("Error: No distant device found.");
        return -1.0; // Fehler
    }
    float range = device->getRange();
    if (range < 0) {
        Serial.println("Error: Invalid range measurement.");
        return -1.0; // Fehler
    }

    // Sende die Entfernung an den MQTT-Broker
    if (mqttModule->isConnected()) {
        String topic = "uwb/anchor/distance";
        String payload = String(range, 2);
        mqttModule->publish(topic.c_str(), payload.c_str());
        Serial.printf("Distance published to MQTT: %.2f m\n", range);
    } else {
        Serial.println("MQTT not connected. Distance not published.");
    }

    return range;
}

void DistanceModule::startCalibration() {
    Serial.println("Starting calibration...");
    float totalDistance = 0.0;
    int validMeasurements = 0;

    for (int i = 0; i < 10; ++i) {
        float distance = getDistanceToTag();
        if (distance >= 0) {
            totalDistance += distance;
            validMeasurements++;
        }
        delay(100);
    }

    if (validMeasurements > 0) {
        float averageDistance = totalDistance / validMeasurements;
        Serial.printf("Calibration completed. Average distance: %.2f m\n", averageDistance);

        // Sende das Kalibrierungsergebnis an den MQTT-Broker
        if (mqttModule->isConnected()) {
            String topic = "uwb/anchor/calibration";
            String payload = String(averageDistance, 2);
            mqttModule->publish(topic.c_str(), payload.c_str());
            Serial.printf("Calibration result published to MQTT: %.2f m\n", averageDistance);
        } else {
            Serial.println("MQTT not connected. Calibration result not published.");
        }
    } else {
        Serial.println("Calibration failed: No valid measurements collected.");
    }
}
