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

bool BitcoinAPI::fetchBlockData() {
    HTTPClient http;
    
    // Mempool.space API - Free tier, no rate limits mentioned
    // Get latest block info and fee estimates
    const char* url = "https://mempool.space/api/v1/blocks";
    
    Serial.println("[BitcoinAPI] Fetching block data...");
    
    http.begin(url);
    http.setTimeout(10000); // 10 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response (array of recent blocks)
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc.is<JsonArray>() && doc.size() > 0) {
            // Get the latest block (first in array)
            JsonObject latestBlock = doc[0];
            
            if (latestBlock.containsKey("height") && latestBlock.containsKey("timestamp") && latestBlock.containsKey("id")) {
                blockHeight = latestBlock["height"].as<uint32_t>();
                uint32_t blockTimestamp = latestBlock["timestamp"].as<uint32_t>();
                String blockHash = latestBlock["id"].as<String>();
                
                // Calculate block age in minutes
                uint32_t currentTime = time(nullptr); // Unix timestamp
                if (currentTime > blockTimestamp) {
                    blockAgeMinutes = (currentTime - blockTimestamp) / 60;
                } else {
                    blockAgeMinutes = 0;
                }
                
                // Now get detailed block info
                http.end();
                http.begin("https://mempool.space/api/v1/block/" + blockHash);
                
                int blockHttpCode = http.GET();
                if (blockHttpCode == HTTP_CODE_OK) {
                    String blockPayload = http.getString();
                    JsonDocument blockDoc;
                    DeserializationError blockError = deserializeJson(blockDoc, blockPayload);
                    
                    if (!blockError) {
                        // Extract fee information - try fee_range first, then estimates
                        bool feeDataFound = false;
                        
                        if (blockDoc.containsKey("fee_range")) {
                            JsonArray feeRange = blockDoc["fee_range"];
                            if (feeRange.size() >= 2) {
                                minFee = feeRange[0].as<float>();
                                maxFee = feeRange[feeRange.size() - 1].as<float>();
                                if (minFee > 0 || maxFee > 0) {
                                    feeDataFound = true;
                                }
                            }
                        }
                        
                        // Extract fee information from mempool fee estimates
                        http.end();
                        http.begin("https://mempool.space/api/v1/fees/recommended");

                        int feeHttpCode = http.GET();
                        if (feeHttpCode == HTTP_CODE_OK) {
                            String feePayload = http.getString();
                            JsonDocument feeDoc;
                            DeserializationError feeError = deserializeJson(feeDoc, feePayload);

                            if (!feeError) {
                                // Use mempool fee estimates for min/max
                                if (feeDoc.containsKey("minimumFee")) {
                                    minFee = feeDoc["minimumFee"].as<float>();
                                } else {
                                    minFee = feeDoc["hourFee"].as<float>() * 0.5; // Fallback
                                }

                                if (feeDoc.containsKey("fastestFee")) {
                                    maxFee = feeDoc["fastestFee"].as<float>();
                                } else {
                                    maxFee = feeDoc["halfHourFee"].as<float>() * 2.0; // Fallback
                                }

                                feeDataFound = true;
                            }
                        }
                        http.end();
                        http.begin("https://mempool.space/api/v1/block/" + blockHash); // Reconnect for pool data
                        http.GET(); // Re-fetch block data
                        deserializeJson(blockDoc, http.getString());
                        
                        // Extract average fee
                        if (blockDoc.containsKey("extras") && blockDoc["extras"].containsKey("avgFeeRate")) {
                            avgFee = blockDoc["extras"]["avgFeeRate"].as<float>();
                        }
                        
                        // Extract pool name
                        if (blockDoc.containsKey("extras") && blockDoc["extras"].containsKey("pool") && 
                            blockDoc["extras"]["pool"].containsKey("name")) {
                            poolName = blockDoc["extras"]["pool"]["name"].as<String>();
                        } else {
                            poolName = "Unknown";
                        }
                        
                        blockDataValid = true;
                        Serial.printf("[BitcoinAPI] Block data updated: height=%u, age=%u min, fees=%.1f-%.1f sat/vB, pool=%s\n", 
                                    blockHeight, blockAgeMinutes, minFee, maxFee, poolName.c_str());
                        http.end();
                        return true;
                    }
                }
                
                // If detailed block fetch failed, try fee estimates
                http.end();
                http.begin("https://mempool.space/api/v1/fees/recommended");
                
                int feeHttpCode = http.GET();
                if (feeHttpCode == HTTP_CODE_OK) {
                    String feePayload = http.getString();
                    JsonDocument feeDoc;
                    DeserializationError feeError = deserializeJson(feeDoc, feePayload);
                    
                    if (!feeError && feeDoc.containsKey("halfHourFee")) {
                        avgFee = feeDoc["halfHourFee"].as<float>();
                        minFee = avgFee * 0.5; // Estimate min fee
                        maxFee = avgFee * 2.0; // Estimate max fee
                        poolName = "Unknown";
                        blockDataValid = true;
                        
                        Serial.printf("[BitcoinAPI] Block data updated (basic): height=%u, age=%u min, avgFee=%.1f sat/vB\n", 
                                    blockHeight, blockAgeMinutes, avgFee);
                        http.end();
                        return true;
                    }
                }
            } else {
                Serial.println("[BitcoinAPI] Invalid block JSON structure");
            }
        } else {
            Serial.printf("[BitcoinAPI] JSON parse error or empty array: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[BitcoinAPI] HTTP error: %d\n", httpCode);
    }
    
    http.end();
    blockDataValid = false;
    return false;
}
