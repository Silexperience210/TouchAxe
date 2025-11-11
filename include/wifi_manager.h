#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define MAX_BITAXE_DEVICES 10

struct BitaxeDevice {
    String name;
    String ip;
    bool online;
};

class WifiManager {
private:
    static WifiManager* instance;
    AsyncWebServer* server;
    Preferences prefs;
    
    String ssid;
    String password;
    bool apMode;
    bool wasConnected;  // Pour détecter la première connexion
    
    BitaxeDevice bitaxes[MAX_BITAXE_DEVICES];
    int bitaxeCount;
    
    // Callback typedef et membre
    typedef void (*WifiConnectedCallback)();
    WifiConnectedCallback wifiConnectedCallback;
    
    WifiManager();
    
    void setupAP();
    void setupWebServer();
    void loadConfig();
    void saveConfig();
    void saveBitaxeConfig();
    void loadBitaxeConfig();

public:
    static WifiManager* getInstance() {
        if (!instance) {
            instance = new WifiManager();
        }
        return instance;
    }
    
    void init();
    void update();
    
    // Callback pour transition auto vers Clock quand WiFi connecté
    void setWifiConnectedCallback(void (*callback)()) { wifiConnectedCallback = callback; }
    
    bool isAPMode() { return apMode; }
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    String getSSID() { return ssid; }
    String getIP() { return WiFi.localIP().toString(); }
    
    // Config reset
    void resetConfig();
    
    // Web server control
    void stopWebServer();
    void startWebServer();
    bool isWebServerRunning() { return server != nullptr; }
    
    // Debug functions
    void clearAllBitaxes();
    void printBitaxeConfig();
    
    // Bitaxe management
    bool addBitaxe(String name, String ip);
    bool removeBitaxe(int index);
    BitaxeDevice* getBitaxe(int index);
    int getBitaxeCount() { return bitaxeCount; }
};

inline WifiManager* WifiManager::instance = nullptr;