#ifndef BITCOIN_API_H
#define BITCOIN_API_H

#include <Arduino.h>

class BitcoinAPI {
public:
    static BitcoinAPI& getInstance();
    
    void begin();
    bool fetchPrice();
    float getPrice() const { return price; }
    bool isPriceValid() const { return priceValid; }
    
private:
    BitcoinAPI() : price(0.0), priceValid(false) {}
    
    float price;
    bool priceValid;
};

#endif // BITCOIN_API_H
