#ifndef BITCOIN_API_H
#define BITCOIN_API_H

#include <Arduino.h>

class BitcoinAPI {
public:
    static BitcoinAPI& getInstance();
    
    void begin();
    bool fetchPrice();
    bool fetchBlockData();
    float getPrice() const { return price; }
    bool isPriceValid() const { return priceValid; }
    uint32_t getBlockHeight() const { return blockHeight; }
    float getAvgFee() const { return avgFee; }
    uint32_t getBlockAgeMinutes() const { return blockAgeMinutes; }
    float getMinFee() const { return minFee; }
    float getMaxFee() const { return maxFee; }
    String getPoolName() const { return poolName; }
    bool isBlockDataValid() const { return blockDataValid; }
    
private:
    BitcoinAPI() : price(0.0), priceValid(false), blockHeight(0), avgFee(0.0), blockAgeMinutes(0), minFee(0.0), maxFee(0.0), poolName(""), blockDataValid(false) {}
    
    float price;
    bool priceValid;
    uint32_t blockHeight;
    float avgFee;
    uint32_t blockAgeMinutes;
    float minFee;
    float maxFee;
    String poolName;
    bool blockDataValid;
};

#endif // BITCOIN_API_H
