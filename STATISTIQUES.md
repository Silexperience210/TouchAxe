# TouchAxe - Nouvelles Fonctionnalités de Statistiques

## Vue d'ensemble

Cette mise à jour apporte des améliorations significatives à TouchAxe, notamment :
- Suivi historique des performances de chaque mineur
- Graphiques de statistiques en temps réel
- Système d'alertes pour conditions anormales
- Métriques d'efficacité énergétique

## Fonctionnalités Principales

### 1. Gestionnaire de Statistiques (StatsManager)

Le `StatsManager` est un singleton qui gère toutes les données historiques des mineurs.

**Capacités:**
- Stockage de jusqu'à 288 points de données par métrique (24h à intervalles de 5 minutes)
- Suivi de 4 métriques par mineur:
  - Hashrate (GH/s)
  - Température (°C)
  - Consommation électrique (W)
  - Efficacité (J/TH)

**Optimisations mémoire:**
- Limitation automatique à 288 points maximum par métrique
- Nettoyage automatique des données > 24h (toutes les heures)
- Utilisation de std::vector avec reserve() pour minimiser les réallocations

### 2. Écran de Statistiques

Nouvel écran accessible via le bouton "STS" (vert) sur chaque carte de mineur.

**Affichage:**
- **En-tête:** Nom du mineur
- **Statistiques de session:**
  - Hashrate moyen, minimum et maximum
  - Température min/max (dans le gestionnaire)
- **Alertes actives:**
  - Température élevée (>75°C par défaut)
  - Hashrate faible (<100 GH/s par défaut)
- **Métriques:**
  - Efficacité moyenne (J/TH)
- **Graphiques:**
  - Hashrate sur 24h (ligne verte)
  - Température sur 24h (ligne orange)

### 3. Système d'Alertes

Le système détecte automatiquement les conditions anormales:

**Types d'alertes:**
- **Température élevée:** Température > seuil configuré (défaut: 75°C)
- **Hashrate faible:** Hashrate < seuil configuré (défaut: 100 GH/s)
- **Mineur hors ligne:** Perte de connexion

**Indicateurs visuels:**
- Bordure de couleur sur la carte du mineur:
  - Vert: Tout va bien
  - Orange: Attention (température 60-70°C)
  - Rouge: Alerte active
- Badge "⚠ ALERTE" sur la carte si alerte active
- Message détaillé dans l'écran de statistiques

### 4. Collection de Données

**Fréquence:**
- Vérification des mineurs: toutes les 30 secondes (inchangé)
- Enregistrement des données: toutes les 5 minutes (automatique)
- Nettoyage des anciennes données: toutes les heures

**Processus:**
1. Lors de `checkBitaxeStatus()`, les données sont récupérées de chaque mineur
2. `StatsManager::addDataPoint()` est appelé avec hashrate, température et puissance
3. Le gestionnaire calcule automatiquement l'efficacité (J/TH)
4. Les alertes sont vérifiées et mises à jour
5. Les statistiques de session sont recalculées

### 5. Améliorations UI

**Écran Miners:**
- Nouveau bouton "STS" (Stats) - vert
- Bordures colorées selon l'état du mineur
- Badge d'alerte visuel si problème détecté

**Navigation:**
- Depuis Miners: clic sur bouton "STS" → Écran Stats
- Depuis Stats: bouton "← Retour" → Retour à Miners

## Utilisation

### Consulter les Statistiques

1. Depuis l'écran principal (Clock), touchez le bouton "MINERS" (rouge, à droite)
2. Naviguez entre les mineurs avec les boutons "◄" et "►" ou par swipe
3. Touchez le bouton "STS" (vert) pour voir les statistiques détaillées du mineur
4. Consultez les graphiques et métriques
5. Touchez "← Retour" pour revenir à l'écran Miners

### Configuration des Seuils d'Alerte

Les seuils peuvent être modifiés dans le code:

```cpp
// Dans main.cpp, après l'initialisation du StatsManager
StatsManager::getInstance().setTempThreshold(80.0);  // Seuil température en °C
StatsManager::getInstance().setHashrateThreshold(200.0);  // Seuil hashrate en GH/s
```

## Architecture Technique

### Structures de Données

**StatDataPoint:**
```cpp
struct StatDataPoint {
    uint32_t timestamp;  // Timestamp en secondes
    float value;         // Valeur de la métrique
};
```

**MinerHistory:**
```cpp
struct MinerHistory {
    String minerName;
    String minerIP;
    std::vector<StatDataPoint> hashrateHistory;
    std::vector<StatDataPoint> temperatureHistory;
    std::vector<StatDataPoint> powerHistory;
    std::vector<StatDataPoint> efficiencyHistory;
    // Statistiques de session
    float maxHashrate, minHashrate, avgHashrate;
    float maxTemp, minTemp;
    // Alertes
    bool highTempAlert, lowHashrateAlert, offlineAlert;
};
```

### Flux de Données

```
main.cpp::loop()
    └─> UI::checkBitaxeStatus() [toutes les 30s]
        └─> Pour chaque mineur:
            ├─> BitaxeAPI::getStats()
            └─> StatsManager::addDataPoint()
                ├─> Vérifier intervalle (5 min)
                ├─> Ajouter point de données
                ├─> Calculer efficacité
                ├─> Limiter à 288 points
                ├─> updateSessionStats()
                └─> checkAlerts()

main.cpp::loop()
    └─> StatsManager::cleanOldData() [toutes les heures]
        └─> Supprimer données > 24h
```

### Utilisation Mémoire

**Estimation par mineur:**
- 4 métriques × 288 points × 8 bytes = ~9 KB par mineur
- Structures + overhead: ~1 KB par mineur
- **Total: ~10 KB par mineur**

Pour 10 mineurs: ~100 KB de RAM utilisée pour les statistiques.

## Limitations et Considérations

1. **Données volatiles:** Les statistiques sont stockées en RAM et perdues au redémarrage
2. **Intervalle de collecte:** Fixé à 5 minutes pour équilibrer précision et mémoire
3. **Rétention:** 24 heures maximum de données historiques
4. **Performance:** Impact minimal grâce à la vérification d'intervalle avant stockage

## Améliorations Futures Possibles

1. **Persistance:** Sauvegarder les statistiques dans SPIFFS pour survivre aux redémarrages
2. **Export:** Fonction d'export des données en CSV ou JSON
3. **Graphiques supplémentaires:** Power history, efficiency trends
4. **Alertes configurables:** Interface pour modifier les seuils sans recompilation
5. **Notifications:** Alertes visuelles/sonores plus proéminentes
6. **Comparaison:** Vue comparative de plusieurs mineurs
7. **Prédictions:** Estimation de la consommation électrique mensuelle

## Fichiers Modifiés/Créés

**Nouveaux fichiers:**
- `include/stats_manager.h` - Interface du gestionnaire de statistiques
- `src/stats_manager.cpp` - Implémentation du gestionnaire

**Fichiers modifiés:**
- `include/ui.h` - Ajout de showStatsScreen()
- `src/ui.cpp` - Implémentation écran stats, alertes, bouton STS
- `src/main.cpp` - Initialisation StatsManager, collection de données

## Support

Pour toute question ou suggestion d'amélioration, veuillez ouvrir une issue sur GitHub.
