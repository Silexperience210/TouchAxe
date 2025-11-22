#pragma once

#include <Arduino.h>
#include <vector>

// Structure pour stocker un point de données historique
struct StatDataPoint {
    uint32_t timestamp;    // Timestamp en secondes depuis epoch
    float value;           // Valeur (hashrate, temperature, power, etc.)
};

// Structure pour stocker l'historique d'un mineur
struct MinerHistory {
    String minerName;
    String minerIP;
    
    // Historique sur 24h (1 point toutes les 5 minutes = 288 points max)
    std::vector<StatDataPoint> hashrateHistory;
    std::vector<StatDataPoint> temperatureHistory;
    std::vector<StatDataPoint> powerHistory;
    std::vector<StatDataPoint> efficiencyHistory;  // J/TH
    
    // Statistiques de session
    uint32_t sessionStart;
    float maxHashrate;
    float minHashrate;
    float avgHashrate;
    float maxTemp;
    float minTemp;
    
    // Alertes
    bool highTempAlert;      // Température > seuil
    bool lowHashrateAlert;   // Hashrate < seuil
    bool offlineAlert;       // Mineur hors ligne
};

// Gestionnaire de statistiques pour tous les mineurs
class StatsManager {
public:
    static StatsManager& getInstance() {
        static StatsManager instance;
        return instance;
    }
    
    // Initialisation
    void init();
    
    // Ajouter un point de données pour un mineur
    void addDataPoint(const String& minerIP, float hashrate, float temp, float power);
    
    // Obtenir l'historique d'un mineur
    MinerHistory* getMinerHistory(const String& minerIP);
    
    // Obtenir le nombre de mineurs suivis
    int getMinerCount() const { return minerHistories.size(); }
    
    // Nettoyer les anciennes données (> 24h)
    void cleanOldData();
    
    // Sauvegarder/charger depuis SPIFFS (optionnel pour persistance)
    bool saveToFile();
    bool loadFromFile();
    
    // Configuration des seuils d'alerte
    void setTempThreshold(float threshold) { tempThreshold = threshold; }
    void setHashrateThreshold(float threshold) { hashrateThreshold = threshold; }
    float getTempThreshold() const { return tempThreshold; }
    float getHashrateThreshold() const { return hashrateThreshold; }

private:
    StatsManager() : tempThreshold(75.0), hashrateThreshold(100.0) {}
    StatsManager(const StatsManager&) = delete;
    StatsManager& operator=(const StatsManager&) = delete;
    
    std::vector<MinerHistory> minerHistories;
    
    // Seuils d'alerte par défaut
    float tempThreshold;      // °C
    float hashrateThreshold;  // GH/s
    
    // Trouver ou créer un historique de mineur
    MinerHistory* findOrCreateHistory(const String& minerIP);
    
    // Calculer les statistiques de session
    void updateSessionStats(MinerHistory* history);
    
    // Vérifier les alertes
    void checkAlerts(MinerHistory* history, float temp, float hashrate);
};
