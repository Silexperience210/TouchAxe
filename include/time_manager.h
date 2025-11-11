#pragma once
#include <time.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class TimeManager {
private:
    static TimeManager* instance;
    static const char* NTP_SERVER;
    static const char* TIMEZONE_FILE;
    
    String currentTimezone;
    long gmtOffset_sec;
    int daylightOffset_sec;
    bool timeInitialized;
    
    TimeManager();
    void loadTimezone();
    void saveTimezone();
    void configureNTP();
    
public:
    static TimeManager* getInstance();
    void init();
    void update();
    
    bool setTimezone(const String& timezone);
    String getCurrentTimezone() const { return currentTimezone; }
    bool isTimeInitialized() const { return timeInitialized; }
    
    String getFormattedTime();
    String getFormattedDate();
};