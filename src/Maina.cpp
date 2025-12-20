
#include <Arduino.h>
#include <SPIFFS.h>
#include "Main.h"
#include "common.h"
#include "NetworkModule.h"
#include "DistanceModule.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_system.h"

#define TAG "OTA_RECOVERY"

// Instantiate the modules
DisplayModule displayModule;


void listSPIFFSContents() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS.");
        return;
    }
    Serial.println("SPIFFS mounted successfully. Listing files:");

    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("Failed to open root directory.");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Root is not a directory.");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        Serial.printf("File: %s, Size: %u bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }
    Serial.println("SPIFFS content listing completed.");
}


void setup() {
    Serial.begin(115200);

    listSPIFFSContents();

    wifi_config_t wifi_config;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    wifi_config.sta.channel = 1;  // Kanal 1 setzen
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    Serial.println("[WIFI] STA-Modus: Kanal auf 1 erzwungen.");

    // Initialisiere NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    Serial.println("NVS erfolgreich initialisiert.");

    // Initialize modules
    NetworkModule::getInstance().initializeWiFi();  // WiFi muss zuerst initialisiert werden
    delay(100);        // Warte, um Konflikte zu vermeiden
    NetworkModule::getInstance().initializeESPNow(); 
 
    displayModule.initialize();


    // Populate anchor MAC address
    NetworkModule::getInstance().populateAnchorMac();
    
    // Display MAC address on startup
    displayModule.showMACAddress(anchorMacString);
}

int loopCount = 0;

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        displayModule.showOperationalStatus();
    } else {
        displayModule.showMACAddress(anchorMacString);
        if (loopCount >= 50 && !TransSuccess) {
            NetworkModule::getInstance().sendMacAndType();
            loopCount = 0;
        } else {
            loopCount++;
        }
   }

    delay(50);
}

extern "C" void app_main() {

    ESP_LOGI(TAG, "Starting application...");
    checkAndRecoverOTA();

    initArduino();
    setup();
    while (true) {
        loop();
    }
}



// Recovery-Funktion
void checkAndRecoverOTA() {
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    if (running_partition == NULL) {
        ESP_LOGE(TAG, "Running partition not found!");
        return;
    }

    ESP_LOGI(TAG, "Running partition: %s", running_partition->label);

    // Überprüfe, ob die aktive Partition bootfähig ist
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) != ESP_OK || ota_state != ESP_OTA_IMG_VALID) {
        ESP_LOGW(TAG, "Invalid OTA state detected. Attempting recovery.");

        // Finde die nächste OTA-Partition
        const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);
        if (next_partition != NULL) {
            ESP_LOGI(TAG, "Switching to next OTA partition: %s", next_partition->label);

            // Setze die nächste Partition als Bootpartition
            if (esp_ota_set_boot_partition(next_partition) == ESP_OK) {
                ESP_LOGI(TAG, "Boot partition switched successfully. Restarting...");
                esp_restart();
            } else {
                ESP_LOGE(TAG, "Failed to switch boot partition!");
            }
        } else {
            ESP_LOGE(TAG, "No valid OTA partition found for recovery.");
        }
    } else {
        ESP_LOGI(TAG, "Current OTA state is valid. No recovery needed.");
    }
}
