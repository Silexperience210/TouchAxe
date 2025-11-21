#include "bitaxe_api.h"

BitaxeAPI::BitaxeAPI() {
    baseUrl = "";
}

BitaxeAPI::~BitaxeAPI() {
    http.end();
}

void BitaxeAPI::setDevice(String ip) {
    baseUrl = "http://" + ip;
    Serial.printf("[BitaxeAPI] Device set to: %s\n", baseUrl.c_str());
}

bool BitaxeAPI::makeRequest(const char* endpoint, JsonDocument& doc) {
    if (baseUrl.length() == 0) {
        return false;
    }
    
    String url = baseUrl + endpoint;
    
    http.begin(url);
    http.setTimeout(2000);  // REDUCED: 2 seconds timeout (was 1s) for better reliability with multiple devices
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // LIMIT JSON SIZE to prevent memory issues with multiple devices
        if (payload.length() > 2048) {  // Max 2KB per response
            Serial.printf("[BitaxeAPI] Response too large: %d bytes, skipping\n", payload.length());
            http.end();
            return false;
        }
        
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.printf("[BitaxeAPI] JSON parse error: %s\n", error.c_str());
            http.end();
            return false;
        }
        
        // DEBUG: Print entire JSON response (only for first device to reduce spam)
        static bool firstDebug = true;
        if (firstDebug) {
            Serial.println("[BitaxeAPI] JSON Response (first device only):");
            serializeJsonPretty(doc, Serial);
            Serial.println();
            firstDebug = false;
        }
        
        http.end();
        return true;
    } else {
        Serial.printf("[BitaxeAPI] HTTP error: %d for %s\n", httpCode, url.c_str());
        http.end();
        return false;
    }
}

bool BitaxeAPI::testConnection() {
    if (baseUrl.length() == 0) {
        return false;
    }
    
    http.begin(baseUrl + "/api/system/info");
    http.setTimeout(3000);
    
    int httpCode = http.GET();
    http.end();
    
    return (httpCode == HTTP_CODE_OK);
}

// Control actions
bool BitaxeAPI::restart() {
    if (baseUrl.length() == 0) return false;
    
    http.begin(baseUrl + "/api/system/restart");
    http.setTimeout(5000);
    
    int httpCode = http.POST("");
    http.end();
    
    Serial.printf("[BitaxeAPI] Restart command: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT);
}

bool BitaxeAPI::reboot() {
    if (baseUrl.length() == 0) return false;
    
    http.begin(baseUrl + "/api/system/reboot");
    http.setTimeout(5000);
    
    int httpCode = http.POST("");
    http.end();
    
    Serial.printf("[BitaxeAPI] Reboot command: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT);
}

bool BitaxeAPI::stopMining() {
    if (baseUrl.length() == 0) return false;
    
    http.begin(baseUrl + "/api/mining/stop");
    http.setTimeout(3000);
    
    int httpCode = http.POST("");
    http.end();
    
    Serial.printf("[BitaxeAPI] Stop mining command: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT);
}

bool BitaxeAPI::startMining() {
    if (baseUrl.length() == 0) return false;
    
    http.begin(baseUrl + "/api/mining/start");
    http.setTimeout(3000);
    
    int httpCode = http.POST("");
    http.end();
    
    Serial.printf("[BitaxeAPI] Start mining command: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT);
}

// Configuration
bool BitaxeAPI::getConfig(JsonDocument& config) {
    return makeRequest("/api/system/config", config);
}

bool BitaxeAPI::setConfig(JsonDocument& config) {
    if (baseUrl.length() == 0) return false;
    
    String url = baseUrl + "/api/system/config";
    String jsonString;
    serializeJson(config, jsonString);
    
    http.begin(url);
    http.setTimeout(5000);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(jsonString);
    http.end();
    
    Serial.printf("[BitaxeAPI] Set config: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT);
}

// Firmware update
bool BitaxeAPI::updateFirmware(String url) {
    if (baseUrl.length() == 0) return false;
    
    String updateUrl = baseUrl + "/api/system/update";
    String payload = "{\"url\":\"" + url + "\"}";
    
    http.begin(updateUrl);
    http.setTimeout(10000);  // Longer timeout for firmware update
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST(payload);
    http.end();
    
    Serial.printf("[BitaxeAPI] Firmware update: HTTP %d\n", httpCode);
    return (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED);
}

bool BitaxeAPI::getStats(BitaxeStats& stats) {
    stats.valid = false;
    
    // Get system info from /api/system/info
    JsonDocument infoDoc;
    if (!makeRequest("/api/system/info", infoDoc)) {
        return false;
    }
    
    // Parse system info - check if response is valid
    if (infoDoc.isNull()) {
        return false;
    }
    
    // Try different field names (AxeOS vs ESP-Miner variants)
    if (infoDoc.containsKey("ASICModel")) {
        stats.hostname = infoDoc["ASICModel"].as<String>();
    } else if (infoDoc.containsKey("hostname")) {
        stats.hostname = infoDoc["hostname"].as<String>();
    } else {
        stats.hostname = "Bitaxe";
    }
    
    if (infoDoc.containsKey("version")) {
        stats.version = infoDoc["version"].as<String>();
    } else {
        stats.version = "Unknown";
    }
    
    // Temperature - can be in different places
    if (infoDoc.containsKey("temp")) {
        stats.temp = infoDoc["temp"].as<float>();
    } else if (infoDoc.containsKey("temperature")) {
        stats.temp = infoDoc["temperature"].as<float>();
    } else {
        stats.temp = 0.0;
    }
    
    // Power
    if (infoDoc.containsKey("power")) {
        stats.power = infoDoc["power"].as<float>();
    } else if (infoDoc.containsKey("voltage") && infoDoc.containsKey("current")) {
        // Calculate power from voltage and current
        stats.power = infoDoc["voltage"].as<float>() * infoDoc["current"].as<float>();
    } else {
        stats.power = 0.0;
    }
    
    // Hashrate - usually in GH/s or needs conversion from H/s
    if (infoDoc.containsKey("hashRate")) {
        float hashrate = infoDoc["hashRate"].as<float>();
        // If value is very large, it's probably in H/s, convert to GH/s
        if (hashrate > 1000000) {
            stats.hashrate = hashrate / 1000000000.0;
        } else {
            stats.hashrate = hashrate;
        }
    } else if (infoDoc.containsKey("currentHashrate")) {
        stats.hashrate = infoDoc["currentHashrate"].as<float>() / 1000000000.0;
    } else {
        stats.hashrate = 0.0;
    }
    
    // Best difficulty - peut être un nombre OU une string formatée (ex: "997.30 M", "72.6G")
    if (infoDoc.containsKey("bestDiff")) {
        if (infoDoc["bestDiff"].is<String>()) {
            // C'est une string formatée comme "997.30 M" ou "72.6G"
            String diffStr = infoDoc["bestDiff"].as<String>();
            float diffValue = 0;
            char unit = ' ';
            
            // Parse "997.30 M" ou "72.6G"
            if (sscanf(diffStr.c_str(), "%f %c", &diffValue, &unit) >= 1) {
                if (unit == 'G' || unit == 'g') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000000000.0); // Giga
                } else if (unit == 'M' || unit == 'm') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000000.0);    // Mega
                } else if (unit == 'K' || unit == 'k') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000.0);       // Kilo
                } else {
                    stats.bestDiff = (uint32_t)diffValue;                  // Pas d'unité
                }
                Serial.printf("[BitaxeAPI] Parsed bestDiff string '%s' -> %u\n", diffStr.c_str(), stats.bestDiff);
            } else {
                stats.bestDiff = 0;
                Serial.printf("[BitaxeAPI] Failed to parse bestDiff string: '%s'\n", diffStr.c_str());
            }
        } else {
            // C'est un nombre direct
            stats.bestDiff = infoDoc["bestDiff"].as<uint32_t>();
            Serial.printf("[BitaxeAPI] Found bestDiff as number: %u\n", stats.bestDiff);
        }
    } else if (infoDoc.containsKey("best_diff")) {
        stats.bestDiff = infoDoc["best_diff"].as<uint32_t>();
        Serial.printf("[BitaxeAPI] Found best_diff in JSON: %u\n", stats.bestDiff);
    } else if (infoDoc.containsKey("bestSessionDiff")) {
        if (infoDoc["bestSessionDiff"].is<String>()) {
            String diffStr = infoDoc["bestSessionDiff"].as<String>();
            float diffValue = 0;
            char unit = ' ';
            
            if (sscanf(diffStr.c_str(), "%f %c", &diffValue, &unit) >= 1) {
                if (unit == 'G' || unit == 'g') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000000000.0);
                } else if (unit == 'M' || unit == 'm') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000000.0);
                } else if (unit == 'K' || unit == 'k') {
                    stats.bestDiff = (uint32_t)(diffValue * 1000.0);
                } else {
                    stats.bestDiff = (uint32_t)diffValue;
                }
                Serial.printf("[BitaxeAPI] Parsed bestSessionDiff string '%s' -> %u\n", diffStr.c_str(), stats.bestDiff);
            } else {
                stats.bestDiff = 0;
            }
        } else {
            stats.bestDiff = infoDoc["bestSessionDiff"].as<uint32_t>();
        }
        Serial.printf("[BitaxeAPI] Found bestSessionDiff in JSON: %u\n", stats.bestDiff);
    } else {
        stats.bestDiff = 0;
        Serial.println("[BitaxeAPI] WARNING: No bestDiff/best_diff/bestSessionDiff field found in JSON!");
    }
    
    // Shares
    if (infoDoc.containsKey("sharesAccepted")) {
        stats.shares = infoDoc["sharesAccepted"].as<uint32_t>();
    } else if (infoDoc.containsKey("shares_accepted")) {
        stats.shares = infoDoc["shares_accepted"].as<uint32_t>();
    } else if (infoDoc.containsKey("accepted")) {
        stats.shares = infoDoc["accepted"].as<uint32_t>();
    } else {
        stats.shares = 0;
    }
    
    // Calculate efficiency (J/TH)
    if (stats.hashrate > 0 && stats.power > 0) {
        stats.efficiency = (stats.power / stats.hashrate) * 1000.0;
    } else {
        stats.efficiency = 0;
    }
    
    // Pool info - might be in the same response
    if (infoDoc.containsKey("stratumURL")) {
        stats.poolUrl = infoDoc["stratumURL"].as<String>();
    } else if (infoDoc.containsKey("pool_url")) {
        stats.poolUrl = infoDoc["pool_url"].as<String>();
    } else {
        stats.poolUrl = "";
    }
    
    if (infoDoc.containsKey("stratumUser")) {
        stats.poolUser = infoDoc["stratumUser"].as<String>();
    } else if (infoDoc.containsKey("pool_user")) {
        stats.poolUser = infoDoc["pool_user"].as<String>();
    } else {
        stats.poolUser = "";
    }
    
    // Pool connection status
    stats.poolConnected = (stats.hashrate > 0);  // Assume connected if hashing
    
    // Uptime
    if (infoDoc.containsKey("uptimeSeconds")) {
        stats.uptimeSeconds = infoDoc["uptimeSeconds"].as<uint32_t>();
    } else if (infoDoc.containsKey("uptime")) {
        stats.uptimeSeconds = infoDoc["uptime"].as<uint32_t>();
    } else {
        stats.uptimeSeconds = 0;
    }
    
    stats.valid = true;
    return true;
}
