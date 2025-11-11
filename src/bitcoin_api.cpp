#include "bitcoin_api.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

BitcoinAPI& BitcoinAPI::getInstance() {
    static BitcoinAPI instance;
    return instance;
}

void BitcoinAPI::begin() {
    price = 0.0;
    priceValid = false;
    Serial.println("[BitcoinAPI] Initialized");
}

bool BitcoinAPI::fetchPrice() {
    HTTPClient http;
    
    // CoinGecko API - Free tier: 10-50 req/min
    // We're using 1 req/min = 60 req/hour - within limits
    const char* url = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";
    
    Serial.println("[BitcoinAPI] Fetching Bitcoin price...");
    
    http.begin(url);
    http.setTimeout(10000); // 10 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            if (doc.containsKey("bitcoin") && doc["bitcoin"].containsKey("usd")) {
                price = doc["bitcoin"]["usd"].as<float>();
                priceValid = true;
                Serial.printf("[BitcoinAPI] Price updated: $%.2f\n", price);
                http.end();
                return true;
            } else {
                Serial.println("[BitcoinAPI] Invalid JSON structure");
            }
        } else {
            Serial.printf("[BitcoinAPI] JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[BitcoinAPI] HTTP error: %d\n", httpCode);
    }
    
    http.end();
    priceValid = false;
    return false;
}
