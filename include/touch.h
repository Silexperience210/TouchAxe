#pragma once
#include <Arduino.h>
#include <Wire.h>

extern TwoWire TouchWire;

// GT911 I2C Addresses
#define GT911_I2C_ADDR_28   0x14  // When INT is pulled LOW during startup
#define GT911_I2C_ADDR_5D   0x5D  // When INT is pulled HIGH during startup

// GT911 Register Map
#define GT911_REG_PRODUCT_ID    0x8140  // 4 bytes, devrait retourner "911"
#define GT911_REG_CONFIG_VERSION 0x8047  // Version de la configuration
#define GT911_REG_VENDOR_ID     0x814A  // ID du vendeur
#define GT911_REG_POINT_INFO    0x814E  // Info sur les points de toucher
#define GT911_REG_POINTS        0x814F  // Données des points de toucher
#define GT911_REG_BUFFER_STATUS 0x814E  // État du buffer

class Touch {
private:
    static Touch* instance;
    int16_t sda;
    int16_t scl;
    bool touchInitialized;
    uint8_t addr;  // Current I2C address
    
    Touch() : touchInitialized(false) {}
    bool readRegister(uint16_t reg, uint8_t* buffer, uint8_t len);
    bool writeRegister(uint16_t reg, uint8_t data);

public:
    static Touch* getInstance() {
        if (instance == nullptr) {
            instance = new Touch();
        }
        return instance;
    }
    
    bool init(int16_t sda = 11, int16_t scl = 10);
    bool readTouch(uint16_t* x, uint16_t* y, uint8_t* pressure = nullptr);
    bool isInitialized() const { return touchInitialized; }
};