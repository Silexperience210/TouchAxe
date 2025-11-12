#include "wifi_manager.h"
#include <SPIFFS.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>

WifiManager::WifiManager() {
    server = nullptr;
    apMode = true;
    wasConnected = false;
    wifiConnectedCallback = nullptr;
    bitaxeCount = 0;
}

void WifiManager::init() {
    Serial.println("[WiFi] Initializing WiFi Manager...");
    
    // Initialize SPIFFS for web files
    Serial.println("[WiFi] Mounting SPIFFS...");
    if (!SPIFFS.begin(true)) {
        Serial.println("[WiFi] SPIFFS mount failed!");
    } else {
        Serial.println("[WiFi] SPIFFS mounted successfully");
        
        // List SPIFFS files for debugging
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        Serial.println("[WiFi] SPIFFS files:");
        while(file){
            Serial.printf("[WiFi]   - %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
    }
    
    // Load saved config
    loadConfig();
    loadBitaxeConfig();
    
    // Try to connect to saved WiFi
    if (ssid.length() > 0) {
        Serial.printf("[WiFi] Connecting to %s...\n", ssid.c_str());
        WiFi.mode(WIFI_STA);
        
        // Retry 5 fois avant de basculer en mode AP
        int retry_count = 0;
        const int MAX_RETRIES = 5;
        bool connected = false;
        
        while (retry_count < MAX_RETRIES && !connected) {
            retry_count++;
            Serial.printf("[WiFi] Attempt %d/%d...\n", retry_count, MAX_RETRIES);
            
            WiFi.begin(ssid.c_str(), password.c_str());
            
            // Wait 5 seconds for connection (10 × 500ms)
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 10) {
                delay(500);
                Serial.print(".");
                attempts++;
            }
            Serial.println();
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
                apMode = false;
                connected = true;
            } else {
                Serial.printf("[WiFi] Attempt %d failed\n", retry_count);
                if (retry_count < MAX_RETRIES) {
                    Serial.println("[WiFi] Retrying in 1 second...");
                    delay(1000);
                }
            }
        }
        
        if (!connected) {
            Serial.println("[WiFi] All connection attempts failed, starting AP mode");
            setupAP();
        }
    } else {
        Serial.println("[WiFi] No saved credentials, starting AP mode");
        setupAP();
    }
    
    // Setup web server
    setupWebServer();
}

void WifiManager::setupAP() {
    // Arrêter tout mode WiFi existant
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Démarrer en mode AP uniquement
    WiFi.mode(WIFI_AP);
    delay(100);
    
    // Configuration AP avec canal fixe et visibilité maximale
    bool result = WiFi.softAP("TouchAxe-Setup", "", 1, 0, 4);  // SSID, password, channel, hidden, max_connections
    
    if (!result) {
        Serial.println("[WiFi] AP Mode FAILED to start!");
        return;
    }
    
    delay(500);  // Attendre que l'AP soit complètement démarré
    
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[WiFi] ========================================\n");
    Serial.printf("[WiFi] AP Mode started successfully!\n");
    Serial.printf("[WiFi] SSID: TouchAxe-Setup\n");
    Serial.printf("[WiFi] Password: (none - open network)\n");
    Serial.printf("[WiFi] AP IP address: %s\n", IP.toString().c_str());
    Serial.printf("[WiFi] Connect to WiFi and open: http://%s\n", IP.toString().c_str());
    Serial.printf("[WiFi] ========================================\n");
    
    apMode = true;
}

void WifiManager::setupWebServer() {
    Serial.println("[WiFi] Setting up web server...");
    
    if (server) {
        delete server;
    }
    
    server = new AsyncWebServer(80);
    Serial.println("[WiFi] AsyncWebServer created");
    
    // Enable CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Serve index.html from SPIFFS
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("[WiFi] Root request received");
        if (SPIFFS.exists("/index.html")) {
            Serial.println("[WiFi] Serving index.html");
            request->send(SPIFFS, "/index.html", "text/html");
        } else {
            Serial.println("[WiFi] ERROR: index.html not found!");
            request->send(404, "text/plain", "index.html not found in SPIFFS");
        }
    });
    
    // WiFi configuration endpoint
    AsyncCallbackJsonWebHandler* wifiHandler = new AsyncCallbackJsonWebHandler("/api/wifi", 
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            
            String newSSID = jsonObj["ssid"].as<String>();
            String newPassword = jsonObj["password"].as<String>();
            
            if (newSSID.length() > 0) {
                ssid = newSSID;
                password = newPassword;
                saveConfig();
                
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                response->print("{\"success\":true}");
                request->send(response);
                
                Serial.println("[WiFi] Config saved, restarting in 2 seconds...");
                
                // Restart after 2 seconds to connect to new WiFi
                delay(2000);
                ESP.restart();
            } else {
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                response->print("{\"success\":false,\"error\":\"SSID required\"}");
                request->send(response);
            }
        }
    );
    server->addHandler(wifiHandler);
    
    // Get Bitaxe list
    server->on("/api/bitaxes", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        
        for (int i = 0; i < bitaxeCount; i++) {
            JsonObject obj = array.add<JsonObject>();
            obj["name"] = bitaxes[i].name;
            obj["ip"] = bitaxes[i].ip;
            obj["online"] = bitaxes[i].online;
        }
        
        String response;
        serializeJson(doc, response);
        
        AsyncResponseStream *stream = request->beginResponseStream("application/json");
        stream->print(response);
        request->send(stream);
    });
    
    // Add Bitaxe
    AsyncCallbackJsonWebHandler* bitaxeHandler = new AsyncCallbackJsonWebHandler("/api/bitaxe",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            
            String name = jsonObj["name"].as<String>();
            String ip = jsonObj["ip"].as<String>();
            
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            
            // Validate name and IP are not empty
            if (name.length() == 0 || ip.length() == 0 || ip == "null") {
                response->print("{\"success\":false,\"error\":\"Name and IP cannot be empty\"}");
                request->send(response);
                return;
            }
            
            if (addBitaxe(name, ip)) {
                response->print("{\"success\":true}");
            } else {
                response->print("{\"success\":false,\"error\":\"Max devices reached\"}");
            }
            
            request->send(response);
        }
    );
    server->addHandler(bitaxeHandler);
    
    // Remove Bitaxe
    AsyncCallbackJsonWebHandler* removeHandler = new AsyncCallbackJsonWebHandler("/api/bitaxe/remove",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            Serial.println("[WiFi] ========================================");
            Serial.println("[WiFi] API Request: Remove bitaxe");
            
            // Debug: print raw JSON
            String jsonStr;
            serializeJson(json, jsonStr);
            Serial.printf("[WiFi] Received JSON: %s\n", jsonStr.c_str());
            
            JsonObject jsonObj = json.as<JsonObject>();
            
            // Check if index exists in JSON
            if (!jsonObj.containsKey("index")) {
                Serial.println("[WiFi] ERROR: 'index' key not found in JSON!");
                AsyncResponseStream *response = request->beginResponseStream("application/json");
                response->print("{\"success\":false,\"error\":\"Missing index parameter\"}");
                request->send(response);
                Serial.println("[WiFi] ========================================");
                return;
            }
            
            int indexToRemove = jsonObj["index"].as<int>();
            Serial.printf("[WiFi] Parsed index: %d\n", indexToRemove);
            Serial.printf("[WiFi] Current bitaxe count: %d\n", bitaxeCount);
            
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            
            if (removeBitaxe(indexToRemove)) {
                response->print("{\"success\":true}");
                Serial.println("[WiFi] Bitaxe removed successfully");
            } else {
                response->print("{\"success\":false,\"error\":\"Invalid index\"}");
                Serial.println("[WiFi] Failed to remove bitaxe: invalid index");
            }
            
            request->send(response);
            Serial.println("[WiFi] ========================================");
        }
    );
    server->addHandler(removeHandler);
    
    // Test Bitaxe connection (placeholder)
    server->on("/api/bitaxe/test", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        
        if (request->hasParam("ip")) {
            String ip = request->getParam("ip")->value();
            // TODO: Implement actual API test
            response->print("{\"success\":true,\"hashrate\":\"420 GH/s\",\"temp\":\"65\",\"power\":\"15\"}");
        } else {
            response->print("{\"success\":false,\"error\":\"IP required\"}");
        }
        
        request->send(response);
    });
    
    // Reset WiFi config and restart in AP mode
    server->on("/api/config/reset", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFi] Resetting WiFi config...");
        request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi config cleared. Restarting in AP mode...\"}");
        delay(500);  // Give time for response to be sent
        resetConfig();
    });
    
    // Clear all Bitaxe devices
    server->on("/api/bitaxe/clear", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFi] =================================");
        Serial.println("[WiFi] Clearing all Bitaxe devices...");
        Serial.printf("[WiFi] Current count: %d\n", bitaxeCount);
        
        bitaxeCount = 0;
        
        // Clear all NVS data
        prefs.begin("bitaxe", false);
        prefs.clear();
        prefs.end();
        
        Serial.println("[WiFi] All Bitaxe devices removed and NVS cleared");
        Serial.println("[WiFi] =================================");
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"All Bitaxe devices cleared\"}");
    });
    
    // Clear everything (WiFi + Bitaxe + restart)
    server->on("/api/factory/reset", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFi] Factory reset - clearing all data...");
        bitaxeCount = 0;
        prefs.begin("wifi", false);
        prefs.clear();
        prefs.end();
        prefs.begin("bitaxe", false);
        prefs.clear();
        prefs.end();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset complete. Restarting...\"}");
        delay(500);
        ESP.restart();
    });
    
    // Stop web server to save resources
    server->on("/api/webserver/stop", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFi] Stopping web server via API...");
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Web server will stop in 2 seconds\"}");
        delay(2000);
        stopWebServer();
    });
    
    server->begin();
    Serial.println("[WiFi] ========================================");
    Serial.printf("[WiFi] Web server started on port 80\n");
    if (apMode) {
        Serial.printf("[WiFi] AP Mode: http://%s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.printf("[WiFi] Station Mode: http://%s\n", WiFi.localIP().toString().c_str());
    }
    Serial.println("[WiFi] ========================================");
}

void WifiManager::loadConfig() {
    prefs.begin("wifi", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("password", "");
    prefs.end();
}

void WifiManager::saveConfig() {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
    Serial.println("[WiFi] Config saved");
}

void WifiManager::loadBitaxeConfig() {
    prefs.begin("bitaxe", true);
    bitaxeCount = prefs.getInt("count", 0);
    
    Serial.printf("[WiFi] Loading Bitaxe config - count: %d\n", bitaxeCount);
    
    // Validate count
    if (bitaxeCount < 0 || bitaxeCount > MAX_BITAXE_DEVICES) {
        Serial.printf("[WiFi] WARNING: Invalid bitaxe count %d, resetting to 0\n", bitaxeCount);
        bitaxeCount = 0;
    }
    
    for (int i = 0; i < bitaxeCount; i++) {
        String nameKey = "name" + String(i);
        String ipKey = "ip" + String(i);
        
        bitaxes[i].name = prefs.getString(nameKey.c_str(), "");
        bitaxes[i].ip = prefs.getString(ipKey.c_str(), "");
        bitaxes[i].online = false;
        
        Serial.printf("[WiFi]   [%d] %s - %s\n", i, bitaxes[i].name.c_str(), bitaxes[i].ip.c_str());
    }
    
    prefs.end();
    Serial.printf("[WiFi] Loaded %d Bitaxe device(s)\n", bitaxeCount);
}

void WifiManager::saveBitaxeConfig() {
    Serial.printf("[WiFi] Saving Bitaxe config (%d devices)...\n", bitaxeCount);
    
    prefs.begin("bitaxe", false);
    
    // Clear all existing keys first to avoid orphaned entries
    prefs.clear();
    Serial.println("[WiFi] NVS cleared");
    
    // Save new count
    prefs.putInt("count", bitaxeCount);
    Serial.printf("[WiFi] Saved count: %d\n", bitaxeCount);
    
    // Save all current bitaxes
    for (int i = 0; i < bitaxeCount; i++) {
        String nameKey = "name" + String(i);
        String ipKey = "ip" + String(i);
        
        prefs.putString(nameKey.c_str(), bitaxes[i].name);
        prefs.putString(ipKey.c_str(), bitaxes[i].ip);
        
        Serial.printf("[WiFi]   [%d] Saved: %s = %s\n", i, nameKey.c_str(), bitaxes[i].name.c_str());
        Serial.printf("[WiFi]   [%d] Saved: %s = %s\n", i, ipKey.c_str(), bitaxes[i].ip.c_str());
    }
    
    prefs.end();
    Serial.printf("[WiFi] Bitaxe config saved successfully (%d devices)\n", bitaxeCount);
}

bool WifiManager::addBitaxe(String name, String ip) {
    if (bitaxeCount >= MAX_BITAXE_DEVICES) {
        Serial.printf("[WiFi] Cannot add Bitaxe: max limit reached (%d/%d)\n", bitaxeCount, MAX_BITAXE_DEVICES);
        return false;
    }
    
    // Validate inputs
    if (name.length() == 0 || ip.length() == 0) {
        Serial.println("[WiFi] Cannot add Bitaxe: name or IP is empty");
        return false;
    }
    
    bitaxes[bitaxeCount].name = name;
    bitaxes[bitaxeCount].ip = ip;
    bitaxes[bitaxeCount].online = false;
    bitaxeCount++;
    
    saveBitaxeConfig();
    Serial.printf("[WiFi] Added Bitaxe [%d/%d]: %s (%s)\n", bitaxeCount, MAX_BITAXE_DEVICES, name.c_str(), ip.c_str());
    
    return true;
}

bool WifiManager::removeBitaxe(int index) {
    if (index < 0 || index >= bitaxeCount) {
        Serial.printf("[WiFi] Invalid index %d (count=%d)\n", index, bitaxeCount);
        return false;
    }
    
    Serial.printf("[WiFi] Removing Bitaxe at index %d: %s (%s)\n", index, bitaxes[index].name.c_str(), bitaxes[index].ip.c_str());
    
    // Shift remaining devices down
    for (int i = index; i < bitaxeCount - 1; i++) {
        bitaxes[i] = bitaxes[i + 1];
    }
    
    // Clear the last entry
    bitaxes[bitaxeCount - 1].name = "";
    bitaxes[bitaxeCount - 1].ip = "";
    bitaxes[bitaxeCount - 1].online = false;
    
    bitaxeCount--;
    
    // Save the cleaned config (saveBitaxeConfig will clear and rewrite everything)
    saveBitaxeConfig();
    
    Serial.printf("[WiFi] Removed Bitaxe. New count: %d\n", bitaxeCount);
    return true;
}

BitaxeDevice* WifiManager::getBitaxe(int index) {
    if (index >= 0 && index < bitaxeCount) {
        return &bitaxes[index];
    }
    return nullptr;
}

void WifiManager::update() {
    // Détecter la première connexion WiFi et déclencher le callback
    bool nowConnected = isConnected();
    
    if (nowConnected && !wasConnected && wifiConnectedCallback != nullptr) {
        Serial.println("[WiFi] First connection detected - triggering callback");
        wifiConnectedCallback();
        wasConnected = true;  // Ne déclencher qu'une seule fois
    }
    
    if (!nowConnected) {
        wasConnected = false;  // Reset si déconnecté
    }
}

void WifiManager::resetConfig() {
    Serial.println("[WiFi] Clearing WiFi credentials...");
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
    
    Serial.println("[WiFi] Restarting in AP mode...");
    delay(1000);
    ESP.restart();
}

void WifiManager::clearAllBitaxes() {
    Serial.println("[WiFi] =================================");
    Serial.println("[WiFi] Clearing all Bitaxe devices...");
    Serial.printf("[WiFi] Current count: %d\n", bitaxeCount);
    
    bitaxeCount = 0;
    
    // Clear all NVS data
    prefs.begin("bitaxe", false);
    prefs.clear();
    prefs.end();
    
    Serial.println("[WiFi] All Bitaxe devices removed and NVS cleared");
    Serial.println("[WiFi] =================================");
}

void WifiManager::printBitaxeConfig() {
    Serial.println("\n[WiFi] === Bitaxe Configuration ===");
    Serial.printf("[WiFi] Total devices: %d/%d\n", bitaxeCount, MAX_BITAXE_DEVICES);
    
    if (bitaxeCount == 0) {
        Serial.println("[WiFi] No devices configured");
    } else {
        for (int i = 0; i < bitaxeCount; i++) {
            Serial.printf("[WiFi] [%d] %s - %s (online: %s)\n", 
                i, 
                bitaxes[i].name.c_str(), 
                bitaxes[i].ip.c_str(),
                bitaxes[i].online ? "yes" : "no");
        }
    }
    Serial.println("[WiFi] ==============================\n");
}

void WifiManager::stopWebServer() {
    if (server) {
        Serial.println("[WiFi] Stopping web server to save resources...");
        server->end();
        delete server;
        server = nullptr;
        Serial.println("[WiFi] Web server stopped");
    }
}

void WifiManager::startWebServer() {
    if (!server) {
        Serial.println("[WiFi] Starting web server...");
        setupWebServer();
    }
}