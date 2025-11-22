#include "stats_manager.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define MAX_DATA_POINTS 288  // 24h avec 1 point toutes les 5 minutes
#define DATA_INTERVAL_MS 300000  // 5 minutes en millisecondes
#define DATA_RETENTION_HOURS 24

void StatsManager::init() {
    Serial.println("[StatsManager] Initializing...");
    
    // Charger les données depuis SPIFFS si disponibles
    // loadFromFile();
    
    Serial.println("[StatsManager] Initialized");
}

MinerHistory* StatsManager::findOrCreateHistory(const String& minerIP) {
    // Chercher si l'historique existe déjà
    for (size_t i = 0; i < minerHistories.size(); i++) {
        if (minerHistories[i].minerIP == minerIP) {
            return &minerHistories[i];
        }
    }
    
    // Créer un nouvel historique
    MinerHistory newHistory;
    newHistory.minerIP = minerIP;
    newHistory.minerName = "";  // Sera rempli par le nom du device
    newHistory.sessionStart = millis() / 1000;
    newHistory.maxHashrate = 0;
    newHistory.minHashrate = 999999;
    newHistory.avgHashrate = 0;
    newHistory.maxTemp = 0;
    newHistory.minTemp = 999;
    newHistory.highTempAlert = false;
    newHistory.lowHashrateAlert = false;
    newHistory.offlineAlert = false;
    
    // Réserver de l'espace pour éviter les réallocations
    newHistory.hashrateHistory.reserve(MAX_DATA_POINTS);
    newHistory.temperatureHistory.reserve(MAX_DATA_POINTS);
    newHistory.powerHistory.reserve(MAX_DATA_POINTS);
    newHistory.efficiencyHistory.reserve(MAX_DATA_POINTS);
    
    minerHistories.push_back(newHistory);
    
    Serial.printf("[StatsManager] Created new history for %s\n", minerIP.c_str());
    
    return &minerHistories[minerHistories.size() - 1];
}

void StatsManager::addDataPoint(const String& minerIP, float hashrate, float temp, float power) {
    MinerHistory* history = findOrCreateHistory(minerIP);
    if (!history) return;
    
    uint32_t now = millis() / 1000;  // Timestamp en secondes
    
    // Vérifier si assez de temps s'est écoulé depuis le dernier point
    // (éviter d'ajouter trop de points)
    static uint32_t lastDataTime = 0;
    if (now - lastDataTime < (DATA_INTERVAL_MS / 1000)) {
        return;  // Trop tôt, attendre
    }
    lastDataTime = now;
    
    // Ajouter les points de données
    StatDataPoint hashratePoint = {now, hashrate};
    StatDataPoint tempPoint = {now, temp};
    StatDataPoint powerPoint = {now, power};
    
    history->hashrateHistory.push_back(hashratePoint);
    history->temperatureHistory.push_back(tempPoint);
    history->powerHistory.push_back(powerPoint);
    
    // Calculer l'efficacité (J/TH) si les données sont valides
    if (hashrate > 0 && power > 0) {
        // Efficacité = (Power in Watts) / (Hashrate in TH/s)
        // 1 GH/s = 0.001 TH/s
        float efficiency = power / (hashrate / 1000.0);
        StatDataPoint efficiencyPoint = {now, efficiency};
        history->efficiencyHistory.push_back(efficiencyPoint);
    }
    
    // Limiter le nombre de points (garder seulement les derniers MAX_DATA_POINTS)
    if (history->hashrateHistory.size() > MAX_DATA_POINTS) {
        history->hashrateHistory.erase(history->hashrateHistory.begin());
    }
    if (history->temperatureHistory.size() > MAX_DATA_POINTS) {
        history->temperatureHistory.erase(history->temperatureHistory.begin());
    }
    if (history->powerHistory.size() > MAX_DATA_POINTS) {
        history->powerHistory.erase(history->powerHistory.begin());
    }
    if (history->efficiencyHistory.size() > MAX_DATA_POINTS) {
        history->efficiencyHistory.erase(history->efficiencyHistory.begin());
    }
    
    // Mettre à jour les statistiques de session
    updateSessionStats(history);
    
    // Vérifier les alertes
    checkAlerts(history, temp, hashrate);
    
    Serial.printf("[StatsManager] Added data point for %s: %.1f GH/s, %.1f°C, %.1fW (total points: %d)\n",
                  minerIP.c_str(), hashrate, temp, power, history->hashrateHistory.size());
}

void StatsManager::updateSessionStats(MinerHistory* history) {
    if (!history || history->hashrateHistory.empty()) return;
    
    // Calculer min/max/avg hashrate
    float sum = 0;
    history->maxHashrate = 0;
    history->minHashrate = 999999;
    
    for (const auto& point : history->hashrateHistory) {
        sum += point.value;
        if (point.value > history->maxHashrate) {
            history->maxHashrate = point.value;
        }
        if (point.value < history->minHashrate) {
            history->minHashrate = point.value;
        }
    }
    
    history->avgHashrate = sum / history->hashrateHistory.size();
    
    // Calculer min/max température
    history->maxTemp = 0;
    history->minTemp = 999;
    
    for (const auto& point : history->temperatureHistory) {
        if (point.value > history->maxTemp) {
            history->maxTemp = point.value;
        }
        if (point.value < history->minTemp) {
            history->minTemp = point.value;
        }
    }
}

void StatsManager::checkAlerts(MinerHistory* history, float temp, float hashrate) {
    if (!history) return;
    
    // Alerte température haute
    history->highTempAlert = (temp > tempThreshold);
    
    // Alerte hashrate bas
    history->lowHashrateAlert = (hashrate < hashrateThreshold);
    
    // Log des alertes
    if (history->highTempAlert) {
        Serial.printf("[StatsManager] HIGH TEMP ALERT for %s: %.1f°C (threshold: %.1f°C)\n",
                      history->minerIP.c_str(), temp, tempThreshold);
    }
    if (history->lowHashrateAlert) {
        Serial.printf("[StatsManager] LOW HASHRATE ALERT for %s: %.1f GH/s (threshold: %.1f GH/s)\n",
                      history->minerIP.c_str(), hashrate, hashrateThreshold);
    }
}

MinerHistory* StatsManager::getMinerHistory(const String& minerIP) {
    return findOrCreateHistory(minerIP);
}

void StatsManager::cleanOldData() {
    uint32_t now = millis() / 1000;
    uint32_t retentionSeconds = DATA_RETENTION_HOURS * 3600;
    
    for (auto& history : minerHistories) {
        // Nettoyer hashrate history
        history.hashrateHistory.erase(
            std::remove_if(history.hashrateHistory.begin(), history.hashrateHistory.end(),
                          [now, retentionSeconds](const StatDataPoint& point) {
                              return (now - point.timestamp) > retentionSeconds;
                          }),
            history.hashrateHistory.end()
        );
        
        // Nettoyer temperature history
        history.temperatureHistory.erase(
            std::remove_if(history.temperatureHistory.begin(), history.temperatureHistory.end(),
                          [now, retentionSeconds](const StatDataPoint& point) {
                              return (now - point.timestamp) > retentionSeconds;
                          }),
            history.temperatureHistory.end()
        );
        
        // Nettoyer power history
        history.powerHistory.erase(
            std::remove_if(history.powerHistory.begin(), history.powerHistory.end(),
                          [now, retentionSeconds](const StatDataPoint& point) {
                              return (now - point.timestamp) > retentionSeconds;
                          }),
            history.powerHistory.end()
        );
        
        // Nettoyer efficiency history
        history.efficiencyHistory.erase(
            std::remove_if(history.efficiencyHistory.begin(), history.efficiencyHistory.end(),
                          [now, retentionSeconds](const StatDataPoint& point) {
                              return (now - point.timestamp) > retentionSeconds;
                          }),
            history.efficiencyHistory.end()
        );
    }
    
    Serial.println("[StatsManager] Old data cleaned");
}

bool StatsManager::saveToFile() {
    // TODO: Implémenter la sauvegarde vers SPIFFS si nécessaire
    // Pour l'instant, les données sont en mémoire uniquement
    return true;
}

bool StatsManager::loadFromFile() {
    // TODO: Implémenter le chargement depuis SPIFFS si nécessaire
    return true;
}
