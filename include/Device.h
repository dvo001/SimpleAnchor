#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <array>
#include <cstdint>
#include <Preferences.h>

#ifdef ANCHOR
#undef ANCHOR
#endif

#ifdef TAG
#undef TAG
#endif


class Device {
public:
    enum class Type {
        Anchor = 0,
        Tag = 1
    };

    Device() : id(-1), type(Device::Type::Anchor), position({0.0f, 0.0f, 0.0f}), calibration_offset(0.0f), distance_to_tag(0.0f), isActive(false) {
        ip_address[0] = '\0'; // Initialisiert IP-Adresse als leer
    }
    Device(Type type, int id, std::array<float, 3> pos) : id(id), type(type), position(pos), calibration_offset(0.0f), distance_to_tag(0.0f), isActive(false) {
        ip_address[0] = '\0'; // Initialisiert IP-Adresse als leer
    }

    int id;
    Type type;
    std::array<float, 3> position;
    uint8_t mac_address[6];
    char ip_address[16];      // Hinzugefügt: Platz für IPv4-Adresse (z. B. "192.168.1.100")
    float distance;            // Gemessene Entfernung
    float calibration_offset;  // Offset für Kalibrierung
    float distance_to_tag;     // Gemessene Entfernung zum Tag
    bool isActive;             // Aktivstatus
	


    // Setter and Getter for calibration_offset
    void setCalibrationOffset(float offset) { calibration_offset = offset; }
    float getCalibrationOffset() const { return calibration_offset; }

    // Setter and Getter for distance_to_tag
    void setDistanceToTag(float distance) { distance_to_tag = distance; }
    float getDistanceToTag() const { return distance_to_tag; }

};

#endif // DEVICE_H

// Simulated changes for improved error handling and synchronization
