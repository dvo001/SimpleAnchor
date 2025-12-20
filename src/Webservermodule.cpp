#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <atomic>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <nvs_flash.h>

#include "WebServerModule.h"
#include "NetworkModule.h"
#include "common.h"

WebServerModule::WebServerModule() : anchorServer(80) {
    // Ensure proper initialization
    Serial.println("WebServerModule initialized STA server.");
}

// Singleton-Instanz
WebServerModule& WebServerModule::getInstance() {
    static WebServerModule instance; // Statische Instanz
    return instance;
}

std::atomic<int> activeConnections{0};

void WebServerModule::initializeAnchorServer() {
    Serial.println("[DEBUG] Initializing Anchor Server...");

    Serial.println("Starting Anchor Webserver...");

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
    } else {
        Serial.println("SPIFFS Mount Successful");
    }

    Serial.println("Starting necessary modules...");

    // Verbindungslimitierung
    const int maxConnections = 5;

    // Define route for index.html
    anchorServer.on("/", HTTP_GET, [maxConnections](AsyncWebServerRequest *request) {
        if (activeConnections >= maxConnections) {
            Serial.println("[WARN] Too many connections.");
            request->send(503, "text/plain", "Server busy, try again later.");
            return;
        }
        activeConnections++;
        Serial.println("[DEBUG] Handling request for '/'.");

        size_t freeHeap = ESP.getFreeHeap();
        Serial.printf("Free heap before serving file: %d bytes\n", freeHeap);

        // Serve index.html directly
        request->send(SPIFFS, "/index.html", "text/html");

        freeHeap = ESP.getFreeHeap();
        Serial.printf("Free heap after serving file: %d bytes\n", freeHeap);
        activeConnections--;
    });

    anchorServer.on("/api/startCalibration", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[INFO] API '/api/startCalibration' called.");
        bool success = true; // startAnchorCalibration();
        if (success) {
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(500, "application/json", "{\"error\":\"Kalibrierung fehlgeschlagen\"}");
        }
    });

    anchorServer.on("/api/getDistance", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc; // Nutzung von DynamicJsonDocument
        doc["status"] = "success";
        doc["distance"] = 1.1; // Beispielwert
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    anchorServer.on("/api/resetAnchor", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc; // Nutzung von DynamicJsonDocument

        Serial.println("[INFO] Resetting Anchor to initial state...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        esp_wifi_stop();
        esp_now_deinit();

        esp_err_t err = nvs_flash_erase();
        if (err == ESP_OK) {
            Serial.println("[INFO] NVS storage erased successfully.");
        } else {
            Serial.printf("[ERROR] Failed to erase NVS storage: %s\n", esp_err_to_name(err));
        }

        Serial.println("[INFO] Restarting system...");
        delay(1000);
        ESP.restart();

        doc["status"] = "success";
        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    // Enhanced header validation and error handling
    anchorServer.onNotFound([](AsyncWebServerRequest *request) {
        Serial.println("[ERROR] File not found or invalid request headers.");
        if (request) {
            for (int i = 0; i < request->headers(); i++) {
                Serial.printf("Header %s: %s\n",
                              request->getHeader(i)->name().c_str(),
                              request->getHeader(i)->value().c_str());
            }
            request->send(404, "text/plain", "File not found");
        } else {
            Serial.println("[ERROR] Request object is null.");
        }
    });

    Serial.println("[DEBUG] Starte Webserver...");
    anchorServer.begin();
    Serial.println("[DEBUG] Anchor Server started successfully.");

    // Debugging: Log free heap before serving files
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("Free heap before serving files: %d bytes\n", freeHeap);

}

