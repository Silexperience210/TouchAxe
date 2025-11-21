#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Structure pour stocker les données météo
struct WeatherData {
    float temperature;
    String condition;  // clear, cloudy, rain, snow, etc.
    String icon;       // Emoji ou symbole
    bool valid;
    uint32_t lastUpdate;
};

class WeatherManager {
public:
    static WeatherManager* getInstance();

    // Initialisation
    void init();

    // Mise à jour des données météo
    bool updateWeather();

    // Accès aux données
    WeatherData getWeatherData();
    bool isWeatherValid();
    String getWeatherDisplayText();  // Format: "🌤️ 22°C"

    // Géolocalisation
    bool getLocationFromIP(float& latitude, float& longitude);

private:
    WeatherManager();
    static WeatherManager* instance;

    WeatherData weatherData;
    String apiKey;  // Vide pour Open-Meteo (pas de clé requise)

    // URLs des APIs
    const char* GEOLOCATION_API = "http://ip-api.com/json/";
    const char* WEATHER_API = "https://api.open-meteo.com/v1/forecast";

    // Cache
    const uint32_t CACHE_TIMEOUT_MS = 1800000;  // 30 minutes

    // Méthodes privées
    String getWeatherIcon(int weatherCode);
    String httpGET(const char* url);
};

#endif