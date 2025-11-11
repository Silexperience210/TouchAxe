#include "time_manager.h"
#include <SPIFFS.h>

const char* TimeManager::NTP_SERVER = "pool.ntp.org";
const char* TimeManager::TIMEZONE_FILE = "/timezone.json";
TimeManager* TimeManager::instance = nullptr;

TimeManager::TimeManager() : 
    currentTimezone("Europe/Paris"),
    gmtOffset_sec(3600),
    daylightOffset_sec(3600),
    timeInitialized(false) {
}

TimeManager* TimeManager::getInstance() {
    if (!instance) {
        instance = new TimeManager();
    }
    return instance;
}

void TimeManager::init() {
    loadTimezone();
    configureNTP();
}

void TimeManager::loadTimezone() {
    if (SPIFFS.exists(TIMEZONE_FILE)) {
        File file = SPIFFS.open(TIMEZONE_FILE, "r");
        JsonDocument doc;
        deserializeJson(doc, file);
        
        currentTimezone = doc["timezone"].as<String>();
        gmtOffset_sec = doc["gmtOffset"].as<long>();
        daylightOffset_sec = doc["daylightOffset"].as<int>();
        
        file.close();
    }
}

void TimeManager::saveTimezone() {
    JsonDocument doc;
    doc["timezone"] = currentTimezone;
    doc["gmtOffset"] = gmtOffset_sec;
    doc["daylightOffset"] = daylightOffset_sec;
    
    File file = SPIFFS.open(TIMEZONE_FILE, "w");
    serializeJson(doc, file);
    file.close();
}

void TimeManager::configureNTP() {
    configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
}

bool TimeManager::setTimezone(const String& timezone) {
    File file = SPIFFS.open("/timezones.json", "r");
    if (!file) return false;
    
    JsonDocument doc;
    deserializeJson(doc, file);
    file.close();
    
    for (JsonObject tz : doc["timezones"].as<JsonArray>()) {
        if (tz["name"].as<String>() == timezone) {
            currentTimezone = timezone;
            
            // Parse offset string (e.g., "+0100")
            String offset = tz["offset"].as<String>();
            int hours = offset.substring(1,3).toInt();
            int minutes = offset.substring(3).toInt();
            gmtOffset_sec = (hours * 3600) + (minutes * 60);
            
            // Gestion DST
            daylightOffset_sec = tz["dst"].as<bool>() ? 3600 : 0;
            
            configureNTP();
            saveTimezone();
            return true;
        }
    }
    
    return false;
}

String TimeManager::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "??:??:??";
    }
    
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    return String(timeStr);
}

String TimeManager::getFormattedDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "??/??/????";
    }
    
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
    return String(dateStr);
}

void TimeManager::update() {
    if (!timeInitialized) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            timeInitialized = true;
            Serial.printf("[TimeManager] NTP synchronized! Time: %02d:%02d:%02d\n", 
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
}