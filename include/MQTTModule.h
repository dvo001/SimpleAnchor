#ifndef MQTTMODULE_H
#define MQTTMODULE_H

#include <WiFi.h>
#include <PubSubClient.h>

class MQTTModule {
public:
    MQTTModule(const char* serverAddress, int serverPort, const char* clientId);
    void begin();
    void reconnect();
    void publish(const char* topic, const char* payload);
    bool isConnected();

private:
    const char* serverAddress;
    int serverPort;
    const char* clientId;
    PubSubClient mqttClient;
};

#endif // MQTTMODULE_H
