#include "MQTTModule.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient wifiClient;

MQTTModule::MQTTModule(const char* serverAddress, int serverPort, const char* id)
    : serverAddress(serverAddress), serverPort(serverPort), clientId(id), mqttClient(wifiClient) {}

void MQTTModule::begin() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Warte auf bestehende WLAN-Verbindung...");
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWLAN verbunden.");
    }
    
    mqttClient.setServer(serverAddress, serverPort);
    reconnect();
}

void MQTTModule::reconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Verbinde mit MQTT-Broker...");
        if (mqttClient.connect(clientId)) {
            Serial.println(" verbunden!");
        } else {
            Serial.print(" fehlgeschlagen, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" Versuche es erneut in 5 Sekunden.");
            delay(5000);
        }
    }
}

void MQTTModule::publish(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.publish(topic, payload);
}

bool MQTTModule::isConnected() {
    return mqttClient.connected();
}
