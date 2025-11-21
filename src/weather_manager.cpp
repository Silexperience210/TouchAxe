#include "weather_manager.h"
#include <WiFi.h>

// Singleton instance
WeatherManager* WeatherManager::instance = nullptr;

WeatherManager::WeatherManager() {
    weatherData.valid = false;
    weatherData.temperature = 0.0;
    weatherData.lastUpdate = 0;
}

WeatherManager* WeatherManager::getInstance() {
    if (instance == nullptr) {
        instance = new WeatherManager();
    }
    return instance;
}

void WeatherManager::init() {
    Serial.println("[WEATHER] WeatherManager initialized");
}

bool WeatherManager::getLocationFromIP(float& latitude, float& longitude) {
    if (!WiFi.isConnected()) {
        Serial.println("[WEATHER] WiFi not connected for geolocation");
        return false;
    }

    Serial.println("[WEATHER] Getting location from IP...");

    String response = httpGET(GEOLOCATION_API);
    if (response.length() == 0) {
        Serial.println("[WEATHER] Failed to get geolocation response");
        return false;
    }

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.printf("[WEATHER] JSON parse error: %s\n", error.c_str());
        return false;
    }

    if (doc.containsKey("lat") && doc.containsKey("lon")) {
        latitude = doc["lat"];
        longitude = doc["lon"];
        Serial.printf("[WEATHER] Location: %.4f, %.4f\n", latitude, longitude);
        return true;
    }

    Serial.println("[WEATHER] Location data not found in response");
    return false;
}

bool WeatherManager::updateWeather() {
    if (!WiFi.isConnected()) {
        Serial.println("[WEATHER] WiFi not connected for weather update");
        return false;
    }

    // Check cache
    if (weatherData.valid && (millis() - weatherData.lastUpdate) < CACHE_TIMEOUT_MS) {
        Serial.println("[WEATHER] Using cached weather data");
        return true;
    }

    // Get location
    float lat, lon;
    if (!getLocationFromIP(lat, lon)) {
        Serial.println("[WEATHER] Failed to get location");
        return false;
    }

    // Build weather API URL
    String url = String(WEATHER_API) +
                 "?latitude=" + String(lat, 4) +
                 "&longitude=" + String(lon, 4) +
                 "&current=temperature_2m,weather_code" +
                 "&timezone=auto";

    Serial.printf("[WEATHER] Fetching weather from: %s\n", url.c_str());

    String response = httpGET(url.c_str());
    if (response.length() == 0) {
        Serial.println("[WEATHER] Failed to get weather response");
        return false;
    }

    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.printf("[WEATHER] Weather JSON parse error: %s\n", error.c_str());
        return false;
    }

    if (doc.containsKey("current")) {
        JsonObject current = doc["current"];

        if (current.containsKey("temperature_2m") && current.containsKey("weather_code")) {
            weatherData.temperature = current["temperature_2m"];
            int weatherCode = current["weather_code"];

            // Convert weather code to icon directly
            weatherData.icon = getWeatherIcon(weatherCode);
            weatherData.condition = "current"; // Not used for display
            weatherData.valid = true;
            weatherData.lastUpdate = millis();

            Serial.printf("[WEATHER] Updated: %.1f°C, weather code: %d, icon: %s\n",
                        weatherData.temperature,
                        weatherCode,
                        weatherData.icon.c_str());

            return true;
        }
    }

    Serial.println("[WEATHER] Weather data not found in response");
    return false;
}

WeatherData WeatherManager::getWeatherData() {
    return weatherData;
}

bool WeatherManager::isWeatherValid() {
    return weatherData.valid && (millis() - weatherData.lastUpdate) < CACHE_TIMEOUT_MS;
}

String WeatherManager::getWeatherDisplayText() {
    if (!isWeatherValid()) {
        return "--°C";
    }

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%s %.0f°C", weatherData.icon.c_str(), weatherData.temperature);
    return String(buffer);
}

String WeatherManager::getWeatherIcon(int weatherCode) {
    // WMO Weather interpretation codes (Open-Meteo)
    // Using Font Awesome icons for proper display
    switch (weatherCode) {
        case 0: return "\xEF\x86\x85";   // Clear sky - sun (U+F185)
        case 1: case 2: case 3: return "\xEF\x83\x82";  // Mainly clear, partly cloudy, overcast - cloud (U+F0C2)
        case 45: case 48: return "\xEF\x9B\x84";  // Fog and depositing rime fog - smog (U+F6C4)
        case 51: case 53: case 55: return "\xEF\x9C\xBD";  // Drizzle: Light, moderate, dense - cloud-rain (U+F73D)
        case 56: case 57: return "\xEF\x9C\xBD";  // Freezing Drizzle: Light, dense - cloud-rain (U+F73D)
        case 61: case 63: case 65: return "\xEF\x9C\xBD";  // Rain: Slight, moderate, heavy - cloud-rain (U+F73D)
        case 66: case 67: return "\xEF\x9C\xBD";  // Freezing Rain: Light, heavy - cloud-rain (U+F73D)
        case 71: case 73: case 75: return "\xEF\x8B\x9C";  // Snow fall: Slight, moderate, heavy - snowflake (U+F2DC)
        case 77: return "\xEF\x8B\x9C";  // Snow grains - snowflake (U+F2DC)
        case 80: case 81: case 82: return "\xEF\x9C\xBD";  // Rain showers: Slight, moderate, violent - cloud-rain (U+F73D)
        case 85: case 86: return "\xEF\x8B\x9C";  // Snow showers slight and heavy - snowflake (U+F2DC)
        case 95: return "\xEF\x83\xA7";  // Thunderstorm: Slight or moderate - bolt (U+F0E7)
        case 96: case 99: return "\xEF\x83\xA7";  // Thunderstorm with slight and heavy hail - bolt (U+F0E7)
        default: return "\xEF\x84\xA0";  // Question mark (U+F120) for unknown weather
    }
}

String WeatherManager::httpGET(const char* url) {
    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);  // 10 seconds timeout

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        return payload;
    } else {
        Serial.printf("[WEATHER] HTTP GET failed, code: %d\n", httpCode);
        http.end();
        return "";
    }
}