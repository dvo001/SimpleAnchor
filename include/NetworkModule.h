#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <esp_now.h>
#include <WiFi.h>
#include <string>

class NetworkModule {
public:
    NetworkModule();
    static NetworkModule& getInstance(); 
    void initializeWiFi();
    void initializeESPNow();
    void populateAnchorMac();
    void sendMessage(const char* message);
    void sendJsonMessage(const char* key, const char* value);
    void sendMacAndType(); // Neue Funktion zum Senden von MAC-Adresse und Typ
    void sendMacAndIP();
        
    static void onDataReceive(const uint8_t *mac, const uint8_t *data, int len); // Deklaration


private:
    uint8_t mac_address[6];

    void addOrUpdatePeer(const uint8_t* peerMAC);
};

#endif // NETWORK_MODULE_H

