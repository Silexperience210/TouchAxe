#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct BitaxeStats {
    // System info
    String hostname;
    String version;
    float temp;
    
    // Mining stats
    float hashrate;          // GH/s
    float power;             // Watts
    float efficiency;        // J/TH
    uint32_t bestDiff;
    uint32_t shares;
    
    // Pool info
    String poolUrl;
    String poolUser;
    bool poolConnected;
    
    // Uptime
    uint32_t uptimeSeconds;
    
    bool valid;
};

class BitaxeAPI {
private:
    HTTPClient http;
    String baseUrl;
    
    bool makeRequest(const char* endpoint, JsonDocument& doc);

public:
    BitaxeAPI();
    ~BitaxeAPI();
    
    // Set the Bitaxe IP address
    void setDevice(String ip);
    
    // Fetch stats from Bitaxe API
    // Endpoints: /api/system/info and /api/system/stats
    bool getStats(BitaxeStats& stats);
    
    // Test if device is reachable
    bool testConnection();
    
    // Control actions
    bool restart();         // Restart mining software
    bool reboot();          // Reboot the device
    bool stopMining();      // Stop mining
    bool startMining();     // Start mining
    
    // Configuration
    bool getConfig(JsonDocument& config);  // Get current config
    bool setConfig(JsonDocument& config);  // Update config
    
    // Firmware update
    bool updateFirmware(String url);       // Update firmware from URL
};
