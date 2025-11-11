#include "touch.h"
#include "config.h"
#include "driver/gpio.h"

#define GT911_RST_PIN 38  // Pin reset GT911
#define GT911_INT_PIN 18  // Pin INT du GT911 - Connecté via R17
#define TOUCH_SDA 8       // SDA du GT911 sur GPIO8
#define TOUCH_SCL 9       // SCL du GT911 sur GPIO9
#define I2C_FREQ 100000   // 100kHz standard pour GT911

Touch* Touch::instance = nullptr;
TwoWire TouchWire = TwoWire(0);

bool Touch::init(int16_t sda, int16_t scl) {
    if (touchInitialized) return true;
    
    Serial.println("Initializing GT911 touch controller...");
    Serial.println("Vérifier que R17 est bien ponté et R5 supprimé!");
    
    // Store pin configurations
    this->sda = TOUCH_SDA;  // Force les pins standards
    this->scl = TOUCH_SCL;
    
    // Configure I2C
    TouchWire.begin(TOUCH_SDA, TOUCH_SCL, I2C_FREQ);
    
    // Active les pull-ups internes
    pinMode(TOUCH_SDA, INPUT_PULLUP);
    pinMode(TOUCH_SCL, INPUT_PULLUP);
    
    // Forcer les lignes I2C à HIGH
    digitalWrite(TOUCH_SDA, HIGH);
    digitalWrite(TOUCH_SCL, HIGH);
    
    delay(100); // Attendre que tout soit stable
    
    // Scan I2C pour voir quels périphériques sont présents
    // Initialisation des pins
    pinMode(GT911_RST_PIN, OUTPUT);
    pinMode(GT911_INT_PIN, OUTPUT);
    
    // Reset initial pour être sûr de partir d'un état connu
    digitalWrite(GT911_RST_PIN, HIGH);
    digitalWrite(GT911_INT_PIN, HIGH);
    delay(50);
    
    Serial.println("\nScanning I2C bus before reset...");
    for(uint8_t address = 1; address < 127; address++) {
        TouchWire.beginTransmission(address);
        int error = TouchWire.endTransmission();
        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
        }
    }
    Serial.println("Pre-reset I2C scan complete\n");
    
    // Séquence de reset précise pour forcer l'adresse 0x14
    Serial.println("Starting GT911 reset sequence...");
    Serial.printf("Using RST pin: %d, INT pin: %d\n", GT911_RST_PIN, GT911_INT_PIN);
    Serial.printf("Using I2C pins - SDA: %d, SCL: %d\n", sda, scl);
    
    Serial.println("Starting GT911 reset sequence for address 0x14...");
    
    // 1. Commencer avec RST à HIGH et INT à LOW pour préparer le reset
    digitalWrite(GT911_RST_PIN, HIGH);
    digitalWrite(GT911_INT_PIN, LOW);
    delay(50);
    Serial.println("Step 1: RST=HIGH, INT=LOW");
    
    // 2. Reset pulse
    digitalWrite(GT911_RST_PIN, LOW);   // RST = 0
    delay(20);                          // Reset pulse de 20ms
    digitalWrite(GT911_RST_PIN, HIGH);  // RST = 1, INT toujours = 0
    Serial.println("Step 2: Reset pulse completed");
    
    // 3. Attendre que le GT911 soit prêt à détecter l'état de INT
    delay(150);                         // Délai important pour la détection d'adresse
    Serial.println("Step 3: Waiting for address selection");
    
    // 4. Maintenant on peut mettre INT en entrée
    pinMode(GT911_INT_PIN, INPUT);      // INT en entrée
    delay(300);                         // Attendre que tout soit bien stable
    Serial.println("Step 4: INT set to input, reset sequence complete");
    
    Serial.println("\nScanning I2C bus after reset...");
    for(uint8_t address = 1; address < 127; address++) {
        TouchWire.beginTransmission(address);
        int error = TouchWire.endTransmission();
        if (error == 0) {
            Serial.printf("I2C device found at address 0x%02X\n", address);
        }
    }
    Serial.println("Post-reset I2C scan complete\n");
    
    // Avec R17 jumpé et R5 supprimé, l'adresse devrait être 0x14
    uint8_t buffer[4];
    addr = GT911_I2C_ADDR_28;  // 0x14
    
    Serial.printf("Testing GT911 at address 0x%02X...\n", addr);
    if (!readRegister(GT911_REG_PRODUCT_ID, buffer, 4)) {
        Serial.printf("GT911 not detected at 0x%02X (error during read)\n", addr);
        return false;
    }

    // Vérifier le Product ID
    Serial.printf("GT911 Product ID: %c%c%c%c\n", 
                 buffer[0], buffer[1], buffer[2], buffer[3]);
                 
    // Lire la version de configuration
    if (!readRegister(GT911_REG_CONFIG_VERSION, buffer, 1)) {
        Serial.println("Failed to read config version");
        return false;
    }
    
    Serial.printf("GT911 config version: 0x%02X\n", buffer[0]);
    
    // Clear interruption registers
    if (!writeRegister(GT911_REG_BUFFER_STATUS, 0)) {
        Serial.println("Failed to clear status buffer");
        return false;
    }
    
    touchInitialized = true;
    Serial.println("GT911 initialized successfully");
    return true;
}

bool Touch::readTouch(uint16_t* x, uint16_t* y, uint8_t* pressure) {
    if (!touchInitialized) return false;
    
    uint8_t pointInfo;
    if (!readRegister(GT911_REG_POINT_INFO, &pointInfo, 1)) {
        return false;
    }
    
    // Check touch detection
    if (!(pointInfo & 0x80)) {
        // Clear status register
        uint8_t clearStatus = 0;
        writeRegister(GT911_REG_POINT_INFO, clearStatus);
        return false;
    }
    
    // Get number of touch points
    uint8_t touchPoints = pointInfo & 0x0F;
    if (touchPoints < 1) {
        return false;
    }
    
    // Read first touch point data (8 bytes)
    uint8_t buffer[8];
    if (!readRegister(GT911_REG_POINTS, buffer, 8)) {
        return false;
    }
    
    // Extract coordinates (x and y are reversed in landscape mode)
    *y = (buffer[3] << 8) | buffer[2];  // y is first
    *x = (buffer[1] << 8) | buffer[0];  // then x
    
    if (pressure) {
        *pressure = buffer[5];  // size can be used as pressure
    }
    
    // Clear status register
    uint8_t clearStatus = 0;
    writeRegister(GT911_REG_POINT_INFO, clearStatus);
    
    return true;
}

bool Touch::readRegister(uint16_t reg, uint8_t* buffer, uint8_t len) {
    uint8_t retry = 0;
    bool success = false;
    
    while (retry < 3 && !success) {
        // Première transaction : écriture de l'adresse du registre
        TouchWire.beginTransmission(addr);
        TouchWire.write(reg >> 8);    // Register high byte
        TouchWire.write(reg & 0xFF);  // Register low byte
        
        // Terminer la transaction d'écriture avec un STOP
        if (TouchWire.endTransmission(true) != 0) {
            retry++;
            delay(5);
            continue;
        }
        
        delay(2); // Court délai entre les transactions
        
        // Deuxième transaction : lecture des données
        size_t received = TouchWire.requestFrom(addr, (size_t)len, (bool)true);
        if (received != len) {
            retry++;
            delay(5);
            continue;
        }
        
        // Lecture des données
        for (uint8_t i = 0; i < len; i++) {
            buffer[i] = TouchWire.read();
        }
        
        success = true;
    }
    
    return success;
}

bool Touch::writeRegister(uint16_t reg, uint8_t data) {
    uint8_t retry = 0;
    while (retry < 3) {
        TouchWire.beginTransmission(addr);
        TouchWire.write(reg >> 8);    // Register high byte
        TouchWire.write(reg & 0xFF);  // Register low byte
        TouchWire.write(data);        // Data byte
        
        int error = TouchWire.endTransmission();
        if (error == 0) {
            return true;
        }
        
        Serial.printf("I2C write error: %d on retry %d\n", error, retry);
        delay(5); // Court délai entre les tentatives
        retry++;
    }
    
    Serial.printf("Failed to write register 0x%04X after 3 retries\n", reg);
    return false;
}