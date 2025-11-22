#include "ui.h"
#include <Arduino.h>
#include "blackletter_48.h"
#include "font_awesome_weather_32.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "bitaxe_api.h"
#include "bitcoin_api.h"
#include "weather_manager.h"
#include "stats_manager.h"

// Variables pour l'animation de slide
static lv_obj_t* animating_label = nullptr;
static const char* new_label_text = nullptr;
static lv_coord_t original_label_x = 0;

// Variables globales pour les écrans
static lv_obj_t *welcome_screen = nullptr;
static lv_obj_t *clock_screen = nullptr;
static lv_obj_t *miners_screen = nullptr;
static lv_obj_t *stats_screen = nullptr;  // Écran de statistiques
static lv_obj_t *dashboard_screen = nullptr;  // DEPRECATED
static lv_obj_t *time_label = nullptr;
static lv_obj_t *date_label = nullptr;
static lv_obj_t *hashrate_total_label = nullptr;
static lv_obj_t *hashrate_sum_label = nullptr;  // Somme des hashrates en GH/s
static lv_obj_t *bitcoin_price_label = nullptr;  // Prix du Bitcoin en USD
static lv_obj_t *sats_conversion_label = nullptr;  // Conversion 1$ en Sats
static lv_obj_t *weather_label = nullptr;  // Affichage température météo
static lv_obj_t *weather_icon_label = nullptr;  // Icône météo (Font Awesome)
static lv_obj_t *block_data_cube = nullptr;  // Conteneur 3D cube pour les données de bloc
static lv_obj_t *block_data_container = nullptr;  // Conteneur horizontal pour les données de bloc
static lv_obj_t *block_height_label = nullptr;  // Label pour la hauteur du bloc
static lv_obj_t *block_fees_label = nullptr;  // Label pour les frais min-max
static lv_obj_t *block_age_label = nullptr;  // Label pour l'âge du bloc
static lv_obj_t *block_pool_label = nullptr;  // Label pour le pool minier
static lv_obj_t *best_diff_label = nullptr;  // Meilleure difficulté des miners
static lv_obj_t *total_power_label = nullptr;  // Consommation totale des miners
static lv_obj_t *bitaxe_container = nullptr;
static lv_obj_t *btn_scroll_up = nullptr;  // Bouton scroll UP sur Miners screen
static lv_obj_t *btn_scroll_down = nullptr;  // Bouton scroll DOWN sur Miners screen
static lv_obj_t *btn_page_up = nullptr;  // Bouton page précédente
static lv_obj_t *btn_page_down = nullptr;  // Bouton page suivante

// Carrés tombants style Matrix/Tetris - ANIMATION MANUELLE (comme l'horloge)
#define FALLING_SQUARES_COUNT 10  // Réduit à 10 pour meilleures perfs

struct FallingSquare {
    lv_obj_t* obj;
    int start_y;
    int end_y;
    uint32_t start_time;
    uint32_t duration;
    bool active;
};

static FallingSquare falling_squares[FALLING_SQUARES_COUNT];
static bool squares_animation_active = false;

// Index du mineur actuellement affiché dans le carousel
static int current_miner_index = 0;

// Cache des statistiques des mineurs pour navigation instantanée
#define MAX_CACHED_MINERS 10
static BitaxeStats cached_stats[MAX_CACHED_MINERS];
static uint32_t cached_stats_timestamp[MAX_CACHED_MINERS];
static bool cached_stats_valid[MAX_CACHED_MINERS];
#define CACHE_TIMEOUT_MS 30000  // 30 secondes de validité du cache

// Flag pour différer le changement d'écran (évite crash pendant event callbacks)
static bool pending_screen_change = false;
static int pending_screen_target = 0;  // 0=none, 1=CLOCK, 2=MINERS
static lv_obj_t *wifi_status_label = nullptr;  // Pour le welcome screen
static lv_timer_t *time_update_timer = nullptr;  // Timer global pour l'heure
static lv_timer_t *miners_refresh_timer = nullptr;  // Timer auto-refresh pour l'écran Miners

// Constantes pour la pagination des mineurs
#define MINERS_PER_PAGE 3
static lv_obj_t* page_indicator = nullptr;  // Indicateur de page

// État actuel de l'écran
enum ScreenState { WELCOME_SCREEN, CLOCK_SCREEN, MINERS_SCREEN, STATS_SCREEN, DASHBOARD_SCREEN };
static ScreenState current_screen = WELCOME_SCREEN;

// Fonction pour rafraîchir le cache des statistiques de tous les mineurs
static void refreshStatsCache() {
    WifiManager* wifi = WifiManager::getInstance();
    int bitaxeCount = wifi->getBitaxeCount();
    uint32_t now = millis();

    if (!wifi->isConnected()) {
        Serial.println("[UI] WiFi not connected - skipping stats cache refresh");
        return;
    }

    Serial.printf("[UI] Refreshing stats cache for %d miners...\n", bitaxeCount);

    for (int i = 0; i < bitaxeCount && i < MAX_CACHED_MINERS; i++) {
        BitaxeDevice* device = wifi->getBitaxe(i);
        if (device == nullptr) continue;

        // Vérifier si le cache est encore valide (moins de 30 secondes)
        if (cached_stats_valid[i] && (now - cached_stats_timestamp[i]) < CACHE_TIMEOUT_MS) {
            continue;  // Cache encore valide, passer au suivant
        }

        BitaxeAPI api;
        api.setDevice(device->ip);

        BitaxeStats stats;
        if (api.getStats(stats)) {
            // Cache valide
            cached_stats[i] = stats;
            cached_stats_timestamp[i] = now;
            cached_stats_valid[i] = true;
            device->online = true;

            Serial.printf("[UI] Cached stats for miner %d (%s): %.1f GH/s, %.1f°C\n",
                        i, device->name.c_str(), stats.hashrate, stats.temp);
        } else {
            // Échec de récupération - invalider le cache
            cached_stats_valid[i] = false;
            device->online = false;
            Serial.printf("[UI] Failed to cache stats for miner %d (%s)\n",
                        i, device->name.c_str());
        }
    }

    Serial.println("[UI] Stats cache refresh completed");
}

// Fonction pour obtenir les stats d'un mineur depuis le cache (ou forcer refresh si expiré)
static bool getCachedStats(int minerIndex, BitaxeStats& stats) {
    if (minerIndex < 0 || minerIndex >= MAX_CACHED_MINERS) return false;

    // Si le cache n'est pas valide ou expiré, essayer de rafraîchir
    if (!cached_stats_valid[minerIndex] ||
        (millis() - cached_stats_timestamp[minerIndex]) >= CACHE_TIMEOUT_MS) {

        // Forcer un refresh rapide pour ce mineur
        WifiManager* wifi = WifiManager::getInstance();
        BitaxeDevice* device = wifi->getBitaxe(minerIndex);
        if (device && wifi->isConnected()) {
            BitaxeAPI api;
            api.setDevice(device->ip);

            if (api.getStats(stats)) {
                cached_stats[minerIndex] = stats;
                cached_stats_timestamp[minerIndex] = millis();
                cached_stats_valid[minerIndex] = true;
                device->online = true;
                return true;
            } else {
                cached_stats_valid[minerIndex] = false;
                device->online = false;
                return false;
            }
        }
        return false;
    }

    // Cache valide
    stats = cached_stats[minerIndex];
    return true;
}

// Fonction pour rafraîchir les stats Bitaxe
// Variables pour détecter le clic (used by screen_touch_cb)
static int32_t touch_start_x = 0;
static int32_t touch_start_y = 0;
static uint32_t last_touch_time = 0;
static uint32_t last_click_time = 0;
static int click_count = 0;

// Forward declarations for miner and global callback functions (must be
// declared before refreshBitaxeStats because buttons use them)
static void miner_restart_cb(lv_event_t * e);
static void miner_reboot_cb(lv_event_t * e);
static void miner_config_cb(lv_event_t * e);
static void global_restart_all_cb(lv_event_t * e);
static void global_refresh_all_cb(lv_event_t * e);
static void global_sort_hashrate_cb(lv_event_t * e);
static void global_filter_online_cb(lv_event_t * e);
static void global_filter_issues_cb(lv_event_t * e);

// Forward declarations for screen and touch callbacks
static void screen_touch_cb(lv_event_t * e);

// Forward declarations for carousel functions
static void displayMinerInCarousel(int minerIndex);
static void updateCarouselIndicators(int totalMiners);
static void navigateCarousel(bool next);

// Global variables for carousel navigation

static void refreshBitaxeStats() {
    if (bitaxe_container == nullptr) return;

    // Nettoyer le container
    lv_obj_clean(bitaxe_container);

    WifiManager* wifi = WifiManager::getInstance();
    int bitaxeCount = wifi->getBitaxeCount();

    if (bitaxeCount == 0) {
        // Afficher message par défaut
        lv_obj_t* msg = lv_label_create(bitaxe_container);
        lv_label_set_text(msg, "No Bitaxe configured\n\nUse web portal\nto add miners");
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(msg, lv_color_hex(0x808080), 0);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(msg);
    } else {
        // Rafraîchir le cache des stats si nécessaire (en arrière-plan)
        refreshStatsCache();

        // Assurer que l'index du carousel est valide
        if (current_miner_index >= bitaxeCount) {
            current_miner_index = bitaxeCount - 1;
        }
        if (current_miner_index < 0) {
            current_miner_index = 0;
        }

        // Afficher le mineur actuel dans le carousel (utilise le cache)
        displayMinerInCarousel(current_miner_index);

        // Mettre à jour les indicateurs de carousel
        updateCarouselIndicators(bitaxeCount);
    }

    // Force immediate refresh after loading data
    lv_refr_now(NULL);
    Serial.println("[UI] Bitaxe stats refreshed with carousel display (using cache)");
}

// Callback functions for miner actions
static void miner_restart_cb(lv_event_t * e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    BitaxeDevice* device = (BitaxeDevice*)lv_event_get_user_data(e);
    if (device && device->online) {
        Serial.printf("[UI] Restarting miner: %s\n", device->name.c_str());
        BitaxeAPI api;
        api.setDevice(device->ip);
        if (api.restart()) {
            Serial.println("[UI] Restart command sent successfully");
        } else {
            Serial.println("[UI] Failed to send restart command");
        }
    }
}

static void miner_reboot_cb(lv_event_t * e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    BitaxeDevice* device = (BitaxeDevice*)lv_event_get_user_data(e);
    if (device && device->online) {
        Serial.printf("[UI] Rebooting miner: %s\n", device->name.c_str());
        BitaxeAPI api;
        api.setDevice(device->ip);
        if (api.reboot()) {
            Serial.println("[UI] Reboot command sent successfully");
        } else {
            Serial.println("[UI] Failed to send reboot command");
        }
    }
}

// Open miner config (placeholder)
static void miner_config_cb(lv_event_t * e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    BitaxeDevice* device = (BitaxeDevice*)lv_event_get_user_data(e);
    if (device) {
        Serial.printf("[UI] Opening config for miner: %s at http://%s\n", 
                    device->name.c_str(), device->ip.c_str());
        // TODO: Show a small modal with the IP and actions, or launch webserver
    }
}

// Carousel functions
static void displayMinerInCarousel(int minerIndex) {
    if (bitaxe_container == nullptr) return;

    WifiManager* wifi = WifiManager::getInstance();
    int bitaxeCount = wifi->getBitaxeCount();

    if (minerIndex < 0 || minerIndex >= bitaxeCount) return;

    BitaxeDevice* device = wifi->getBitaxe(minerIndex);
    if (device == nullptr) return;

    // Container principal pour le mineur (centré, style carte) - RÉDUIT
    lv_obj_t* miner_card = lv_obj_create(bitaxe_container);
    lv_obj_set_size(miner_card, 420, 140);  // Dimensions réduites pour rentrer dans l'écran
    lv_obj_set_style_bg_color(miner_card, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(miner_card, 2, 0);
    lv_obj_set_style_border_color(miner_card, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(miner_card, 6, 0); // Radius réduit
    lv_obj_set_style_pad_all(miner_card, 12, 0); // Padding réduit
    lv_obj_center(miner_card);

    // Layout vertical pour le contenu de la carte
    lv_obj_set_flex_flow(miner_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(miner_card, 6, 0); // Padding réduit

    // Récupérer les stats depuis le cache (pas d'appel API bloquant)
    BitaxeStats stats;
    if (getCachedStats(minerIndex, stats)) {
        device->online = true;

        // Obtenir l'historique pour vérifier les alertes
        StatsManager& statsMgr = StatsManager::getInstance();
        MinerHistory* history = statsMgr.getMinerHistory(device->ip);
        
        // Déterminer la couleur selon l'état avec priorité aux alertes
        lv_color_t status_color = lv_color_hex(0x00FF00); // Vert = OK
        lv_color_t border_color = lv_color_hex(0xFF0000); // Bordure rouge par défaut
        
        // Vérifier les alertes actives
        bool hasAlert = false;
        if (history) {
            if (history->highTempAlert) {
                status_color = lv_color_hex(0xFF0000); // Rouge = alerte température
                border_color = lv_color_hex(0xFF0000);
                hasAlert = true;
            } else if (history->lowHashrateAlert) {
                status_color = lv_color_hex(0xFFAA00); // Orange = alerte hashrate
                border_color = lv_color_hex(0xFFAA00);
                hasAlert = true;
            } else if (stats.temp > 70.0) {
                status_color = lv_color_hex(0xFF6600); // Orange-rouge = chaud
                border_color = lv_color_hex(0xFF6600);
            } else if (stats.temp > 60.0) {
                status_color = lv_color_hex(0xFFAA00); // Orange = tiède
                border_color = lv_color_hex(0xFFAA00);
            } else {
                border_color = lv_color_hex(0x00FF00); // Bordure verte si tout va bien
            }
        } else {
            // Fallback sans historique
            if (stats.temp > 70.0) {
                status_color = lv_color_hex(0xFF0000);
                border_color = lv_color_hex(0xFF0000);
            } else if (stats.temp > 60.0) {
                status_color = lv_color_hex(0xFFAA00);
                border_color = lv_color_hex(0xFFAA00);
            } else {
                border_color = lv_color_hex(0x00FF00);
            }
        }
        
        // Mettre à jour la couleur de la bordure de la carte
        lv_obj_set_style_border_color(miner_card, border_color, 0);
        
        // Ajouter un indicateur d'alerte visuel si nécessaire (en haut de la carte)
        if (hasAlert) {
            lv_obj_t* alert_indicator = lv_label_create(miner_card);
            lv_label_set_text(alert_indicator, LV_SYMBOL_WARNING " ALERTE");
            lv_obj_set_style_text_font(alert_indicator, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(alert_indicator, lv_color_hex(0xFF0000), 0);
            lv_obj_set_style_text_align(alert_indicator, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_width(alert_indicator, LV_PCT(100));  // Pleine largeur
            // Positionné automatiquement par le flex layout (sera le premier élément)
        }

        // Nom du device avec statut (en haut, gros)
        lv_obj_t* name_label = lv_label_create(miner_card);
        char name_text[64];
        snprintf(name_text, sizeof(name_text), "%s (%s)", device->name.c_str(), stats.hostname.c_str());
        lv_label_set_text(name_label, name_text);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0); // Police réduite
        lv_obj_set_style_text_color(name_label, status_color, 0);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, 0);

        // Ligne 1: Hashrate + Temp + Power (moyenne)
        lv_obj_t* stats_line1 = lv_label_create(miner_card);
        char line1[128];
        snprintf(line1, sizeof(line1), "%.1f GH/s | %.0f°C | %.0fW", stats.hashrate, stats.temp, stats.power);
        lv_label_set_text(stats_line1, line1);
        lv_obj_set_style_text_font(stats_line1, &lv_font_montserrat_14, 0); // Police réduite
        lv_obj_set_style_text_color(stats_line1, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_align(stats_line1, LV_TEXT_ALIGN_CENTER, 0);

        // Ligne 2: Shares + Pool + Best Diff (petite)
        lv_obj_t* stats_line2 = lv_label_create(miner_card);
        char line2[256];  // Augmenté pour l'adresse pool
        char diff_str[16];
        if (stats.bestDiff >= 1000000000) {
            snprintf(diff_str, sizeof(diff_str), "%.1fG", stats.bestDiff / 1000000000.0);
        } else if (stats.bestDiff >= 1000000) {
            snprintf(diff_str, sizeof(diff_str), "%.1fM", stats.bestDiff / 1000000.0);
        } else if (stats.bestDiff >= 1000) {
            snprintf(diff_str, sizeof(diff_str), "%.1fK", stats.bestDiff / 1000.0);
        } else {
            snprintf(diff_str, sizeof(diff_str), "%u", stats.bestDiff);
        }
        
        // Afficher l'adresse stratum de la pool au lieu de "OK"
        String pool_display = stats.poolConnected ? stats.poolUrl : "ERR";
        // Tronquer si trop long pour l'affichage
        if (pool_display.length() > 18) {  // Réduit pour la police plus petite
            pool_display = pool_display.substring(0, 15) + "...";
        }
        
        snprintf(line2, sizeof(line2), "%u shares | %s | %s", stats.shares, pool_display.c_str(), diff_str);
        lv_label_set_text(stats_line2, line2);
        lv_obj_set_style_text_font(stats_line2, &lv_font_montserrat_16, 0);  // Police plus petite
        lv_color_t pool_color = stats.poolConnected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000);
        lv_obj_set_style_text_color(stats_line2, pool_color, 0);
        lv_obj_set_style_text_align(stats_line2, LV_TEXT_ALIGN_CENTER, 0);

        // Boutons d'action (en bas, centrés) - DESIGN AMÉLIORÉ, TRÈS FINS
        lv_obj_t* btn_row = lv_obj_create(miner_card);
        lv_obj_set_size(btn_row, 220, 16); // Dimensions ajustées pour 4 boutons
        lv_obj_set_style_bg_color(btn_row, lv_color_hex(0x2a2a2a), 0);
        lv_obj_set_style_border_width(btn_row, 0, 0);
        lv_obj_set_style_pad_all(btn_row, 1, 0); // Padding minimal
        lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(btn_row, 2, 0); // Espacement minimal
        lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Bouton Stats - NOUVEAU
        lv_obj_t* btn_stats = lv_button_create(btn_row);
        lv_obj_set_size(btn_stats, 50, 14); // Très petit
        lv_obj_set_style_bg_color(btn_stats, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_radius(btn_stats, 1, 0); // Presque carré
        lv_obj_add_flag(btn_stats, LV_OBJ_FLAG_CLICKABLE);
        // Stocker minerIndex directement comme user_data (cast en void*)
        lv_obj_add_event_cb(btn_stats, [](lv_event_t * e) {
            // Récupérer minerIndex depuis user_data
            intptr_t index = (intptr_t)lv_event_get_user_data(e);
            Serial.printf("[UI] Stats button clicked for miner %d\n", (int)index);
            UI::getInstance().showStatsScreen((int)index);
        }, LV_EVENT_CLICKED, (void*)(intptr_t)minerIndex);  // Cast sans allocation
        lv_obj_t* lbl_stats = lv_label_create(btn_stats);
        lv_label_set_text(lbl_stats, "STS");
        lv_obj_set_style_text_font(lbl_stats, &lv_font_montserrat_16, 0); // Police minuscule
        lv_obj_center(lbl_stats);

        // Bouton Restart - TRÈS FIN
        lv_obj_t* btn_restart = lv_button_create(btn_row);
        lv_obj_set_size(btn_restart, 50, 14); // Très petit
        lv_obj_set_style_bg_color(btn_restart, lv_color_hex(0xFF6600), 0);
        lv_obj_set_style_radius(btn_restart, 1, 0); // Presque carré
        lv_obj_add_flag(btn_restart, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_restart, miner_restart_cb, LV_EVENT_CLICKED, device);
        lv_obj_t* lbl_restart = lv_label_create(btn_restart);
        lv_label_set_text(lbl_restart, "RST");
        lv_obj_set_style_text_font(lbl_restart, &lv_font_montserrat_16, 0); // Police minuscule
        lv_obj_center(lbl_restart);

        // Bouton Reboot - TRÈS FIN
        lv_obj_t* btn_reboot = lv_button_create(btn_row);
        lv_obj_set_size(btn_reboot, 50, 14); // Très petit
        lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_radius(btn_reboot, 1, 0); // Presque carré
        lv_obj_add_flag(btn_reboot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_reboot, miner_reboot_cb, LV_EVENT_CLICKED, device);
        lv_obj_t* lbl_reboot = lv_label_create(btn_reboot);
        lv_label_set_text(lbl_reboot, "RBT");
        lv_obj_set_style_text_font(lbl_reboot, &lv_font_montserrat_16, 0); // Police minuscule
        lv_obj_center(lbl_reboot);

        // Bouton Config - TRÈS FIN
        lv_obj_t* btn_config = lv_button_create(btn_row);
        lv_obj_set_size(btn_config, 50, 14); // Très petit
        lv_obj_set_style_bg_color(btn_config, lv_color_hex(0x0080FF), 0);
        lv_obj_set_style_radius(btn_config, 1, 0); // Presque carré
        lv_obj_add_flag(btn_config, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_config, miner_config_cb, LV_EVENT_CLICKED, device);
        lv_obj_t* lbl_config = lv_label_create(btn_config);
        lv_label_set_text(lbl_config, "CFG");
        lv_obj_set_style_text_font(lbl_config, &lv_font_montserrat_16, 0); // Police minuscule
        lv_obj_center(lbl_config);

    } else {
        device->online = false;

        // Offline status - grande carte centrée
        lv_obj_t* offline_label = lv_label_create(miner_card);
        lv_label_set_text(offline_label, "OFFLINE");
        lv_obj_set_style_text_font(offline_label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(offline_label, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_text_align(offline_label, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t* ip_label = lv_label_create(miner_card);
        lv_label_set_text(ip_label, device->ip.c_str());
        lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(ip_label, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_align(ip_label, LV_TEXT_ALIGN_CENTER, 0);

        // Boutons d'action désactivés (gris)
        lv_obj_t* btn_container = lv_obj_create(miner_card);
        lv_obj_set_size(btn_container, 200, 35);
        lv_obj_set_style_bg_color(btn_container, lv_color_hex(0x2a2a2a), 0);
        lv_obj_set_style_border_width(btn_container, 0, 0);
        lv_obj_set_style_pad_all(btn_container, 2, 0);
        lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(btn_container, 3, 0);
        lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* btn_restart = lv_button_create(btn_container);
        lv_obj_set_size(btn_restart, 60, 30);
        lv_obj_set_style_bg_color(btn_restart, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(btn_restart, 3, 0);

        lv_obj_t* btn_reboot = lv_button_create(btn_container);
        lv_obj_set_size(btn_reboot, 60, 30);
        lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(btn_reboot, 3, 0);

        lv_obj_t* btn_config = lv_button_create(btn_container);
        lv_obj_set_size(btn_config, 60, 30);
        lv_obj_set_style_bg_color(btn_config, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(btn_config, 3, 0);
    }

    Serial.printf("[UI] Displayed miner %d in carousel\n", minerIndex);
}

static void updateCarouselIndicators(int totalMiners) {
    if (page_indicator == nullptr) return;

    // Afficher l'indicateur de position: "Miner X/Y"
    char indicator_text[32];
    if (totalMiners > 0) {
        snprintf(indicator_text, sizeof(indicator_text), "Miner %d/%d", current_miner_index + 1, totalMiners);
    } else {
        snprintf(indicator_text, sizeof(indicator_text), "No miners");
    }
    lv_label_set_text(page_indicator, indicator_text);
    lv_obj_invalidate(page_indicator);

    Serial.printf("[UI] Updated carousel indicators: %s\n", indicator_text);
}

static void navigateCarousel(bool next) {
    WifiManager* wifi = WifiManager::getInstance();
    int bitaxeCount = wifi->getBitaxeCount();

    if (bitaxeCount <= 1) return;  // Pas de navigation si 0 ou 1 mineur

    if (next) {
        current_miner_index = (current_miner_index + 1) % bitaxeCount;
    } else {
        current_miner_index = (current_miner_index - 1 + bitaxeCount) % bitaxeCount;
    }

    Serial.printf("[UI] Navigated carousel to miner %d (instant)\n", current_miner_index);

    // Navigation INSTANTANÉE : juste changer l'affichage sans appel API
    if (bitaxe_container != nullptr) {
        lv_obj_clean(bitaxe_container);
        displayMinerInCarousel(current_miner_index);
        updateCarouselIndicators(bitaxeCount);
        lv_refr_now(NULL);  // Force refresh immédiat
    }
}

static void global_restart_all_cb(lv_event_t * e) {
    Serial.println("[UI] Restarting ALL miners");
    WifiManager* wifi = WifiManager::getInstance();
    for (int i = 0; i < wifi->getBitaxeCount(); i++) {
        BitaxeDevice* device = wifi->getBitaxe(i);
        if (device && device->online) {
            BitaxeAPI api;
            api.setDevice(device->ip);
            if (api.restart()) {
                Serial.printf("[UI] Restarted: %s\n", device->name.c_str());
            } else {
                Serial.printf("[UI] Failed to restart: %s\n", device->name.c_str());
            }
        }
    }
}

static void global_refresh_all_cb(lv_event_t * e) {
    Serial.println("[UI] Refreshing ALL miners");
    refreshBitaxeStats();
}

static void global_sort_hashrate_cb(lv_event_t * e) {
    Serial.println("[UI] Sorting miners by hashrate");
    // TODO: Implement sorting by hashrate
    refreshBitaxeStats();
}

static void global_filter_online_cb(lv_event_t * e) {
    Serial.println("[UI] Filtering online miners only");
    // TODO: Implement online filter
    refreshBitaxeStats();
}

static void global_filter_issues_cb(lv_event_t * e) {
    Serial.println("[UI] Filtering miners with issues");
    // TODO: Implement issues filter (high temp, offline, etc.)
    refreshBitaxeStats();
}

// Timer pour mettre à jour l'heure
static void update_time_cb(lv_timer_t * timer) {
    static int count = 0;
    count++;
    Serial.printf("[UI TIMER] Callback #%d fired, timer=%p, paused=%d\n", 
                  count, timer, lv_timer_get_paused(timer));

    TimeManager* tm = TimeManager::getInstance();
    String timeStr, dateStr;

    if (current_screen != CLOCK_SCREEN || time_label == NULL || date_label == NULL) {
        Serial.println("[UI TIMER] Skipped: Wrong screen or null labels");
        return;
    }

    if (!tm->isTimeInitialized()) {
        Serial.printf("[UI TIMER] Time not initialized yet (callback #%d)\n", count);
        lv_label_set_text(time_label, "Sync...");
        return;
    }

    timeStr = tm->getFormattedTime();  // HH:MM:SS
    dateStr = tm->getFormattedDate();  // DD/MM/YYYY

    Serial.printf("[UI TIMER] Updating to: %s %s\n", timeStr.c_str(), dateStr.c_str());

    lv_label_set_text(time_label, timeStr.c_str());
    lv_label_set_text(date_label, dateStr.c_str());

    // CRITICAL: Force LVGL to redraw the labels
    lv_obj_invalidate(time_label);
    lv_obj_invalidate(date_label);
    
    // Also invalidate parent to ensure partial buffer refresh
    lv_obj_t* scr = lv_screen_active();
    lv_obj_invalidate(scr);
    
    // Force immediate refresh to bypass partial buffer delay
    lv_refr_now(NULL);
    
    // Calculer et mettre à jour le hashrate total
    if (hashrate_total_label != NULL) {
        WifiManager* wifi = WifiManager::getInstance();
        float totalHashrate = 0.0;
        int onlineCount = 0;
        
        for (int i = 0; i < wifi->getBitaxeCount(); i++) {
            BitaxeDevice* device = wifi->getBitaxe(i);
            if (device && device->online) {
                // On pourrait stocker le hashrate dans BitaxeDevice
                // Pour l'instant on affiche juste le nombre de mineurs online
                onlineCount++;
            }
        }
        
        char hashrate_text[64];
        snprintf(hashrate_text, sizeof(hashrate_text), "%d miners online", onlineCount);
        lv_label_set_text(hashrate_total_label, hashrate_text);
        lv_obj_invalidate(hashrate_total_label);
    }
}

// Timer pour rafraîchir le cache des stats des mineurs (30 secondes)
static lv_timer_t* stats_cache_timer = nullptr;
static void refresh_stats_cache_cb(lv_timer_t * timer) {
    if (current_screen == MINERS_SCREEN) {
        Serial.println("[UI] Background cache refresh (30s timer)");
        refreshStatsCache();
    }
}

// Callback d'animation pour la pulsation du titre
static void anim_opa_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}

// UPDATE MANUEL des animations - appelé depuis loop() comme updateClock()
static void updateFallingSquaresInternal() {
    if (!squares_animation_active) return;
    
    // SEULEMENT sur l'écran Clock - pas sur Miners !
    if (current_screen != CLOCK_SCREEN) return;
    
    uint32_t now = millis();
    
    for (int i = 0; i < FALLING_SQUARES_COUNT; i++) {
        if (!falling_squares[i].active || !falling_squares[i].obj) continue;
        
        uint32_t elapsed = now - falling_squares[i].start_time;
        
        if (elapsed >= falling_squares[i].duration) {
            // Animation terminée - repositionner en haut
            int new_x = rand() % 480;
            int new_y = -(rand() % 100);
            lv_obj_set_pos(falling_squares[i].obj, new_x, new_y);
            
            // Redémarrer l'animation
            falling_squares[i].start_y = new_y;
            falling_squares[i].end_y = 300;
            falling_squares[i].start_time = now;
            falling_squares[i].duration = 3000 + (rand() % 2000);  // 3-5 secondes
        } else {
            // Calculer la position actuelle (interpolation linéaire)
            float progress = (float)elapsed / (float)falling_squares[i].duration;
            int current_y = falling_squares[i].start_y + 
                           (int)((falling_squares[i].end_y - falling_squares[i].start_y) * progress);
            lv_obj_set_y(falling_squares[i].obj, current_y);
        }
    }
}

// Fonction pour créer les carrés tombants (effet Matrix/Tetris)
static void createFallingSquares(lv_obj_t* parent) {
    Serial.println("[UI] Creating falling squares effect...");
    
    for (int i = 0; i < FALLING_SQUARES_COUNT; i++) {
        // Créer un carré
        lv_obj_t* square = lv_obj_create(parent);
        
        // IMPORTANT: Déplacer en ARRIÈRE-PLAN
        lv_obj_move_background(square);
        
        // Flags
        lv_obj_remove_flag(square, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(square, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(square, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_add_flag(square, LV_OBJ_FLAG_FLOATING);
        
        // Taille aléatoire légèrement plus petite (3 à 15 pixels)
        int size = 3 + (rand() % 13);
        lv_obj_set_size(square, size, size);
        
        // Style: carré rouge avec opacité variable
        lv_obj_set_style_bg_color(square, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(square, 60 + (rand() % 100), 0);  // 60-160 opacité
        lv_obj_set_style_border_width(square, 1, 0);
        lv_obj_set_style_border_color(square, lv_color_hex(0xFF4040), 0);
        lv_obj_set_style_border_opa(square, 100, 0);
        lv_obj_set_style_radius(square, 2, 0);  // Coins légèrement arrondis
        lv_obj_set_style_pad_all(square, 0, 0);
        
        // Glow subtil
        lv_obj_set_style_shadow_width(square, 6, 0);
        lv_obj_set_style_shadow_color(square, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_shadow_opa(square, 60, 0);
        
        // Position initiale: X aléatoire, Y en haut (hors écran)
        int start_x = rand() % 480;
        int start_y = -(rand() % 200);  // Commence au-dessus de l'écran
        lv_obj_set_pos(square, start_x, start_y);
        
        Serial.printf("[UI] Square %d: size=%dpx, pos=(%d,%d)\n", i, size, start_x, start_y);
        
        // ANIMATION MANUELLE - comme pour l'horloge !
        falling_squares[i].obj = square;
        falling_squares[i].start_y = start_y;
        falling_squares[i].end_y = 300;
        falling_squares[i].start_time = millis();
        falling_squares[i].duration = 3000 + (rand() % 2000);  // 3-5 secondes
        falling_squares[i].active = true;
        
        Serial.printf("[UI] Square %d animation setup (manual millis())\n", i);
    }
    
    Serial.println("[UI] *** Falling squares effect created! ***");
    squares_animation_active = true;  // ACTIVER les updates manuels
}

// Fonction pour nettoyer les carrés
static void cleanupFallingSquares() {
    squares_animation_active = false;
    for (int i = 0; i < FALLING_SQUARES_COUNT; i++) {
        if (falling_squares[i].obj != nullptr) {
            lv_obj_delete(falling_squares[i].obj);
            falling_squares[i].obj = nullptr;
            falling_squares[i].active = false;
        }
    }
}

// Callback pour le bouton "Continuer"
void UI::init() {
    Serial.println("[UI] Initializing UI...");
    showWelcomeScreen();
}

void UI::showWelcomeScreen() {
    Serial.println("[UI] Showing welcome screen...");
    
    // Réinitialiser les pointeurs de labels (car on va nettoyer l'écran)
    time_label = nullptr;
    date_label = nullptr;
    hashrate_total_label = nullptr;
    bitaxe_container = nullptr;
    wifi_status_label = nullptr;
    weather_label = nullptr;
    
    // Nettoyer l'écran actuel au lieu de créer un nouveau
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);  // Fond noir pur
    
    // Titre "TouchAxe" en gros rouge brillant avec police Blackletter
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "TOUCHAXE");
    lv_obj_set_style_text_font(title, &blackletter_48, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_opa(title, LV_OPA_COVER, 0);
    lv_obj_set_style_text_letter_space(title, 8, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -80);
    
    // Animation: Pulsation
    lv_anim_t anim_pulse;
    lv_anim_init(&anim_pulse);
    lv_anim_set_var(&anim_pulse, title);
    lv_anim_set_values(&anim_pulse, LV_OPA_COVER, LV_OPA_40);
    lv_anim_set_time(&anim_pulse, 2000);
    lv_anim_set_playback_time(&anim_pulse, 2000);
    lv_anim_set_repeat_count(&anim_pulse, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_pulse, anim_opa_cb);
    lv_anim_start(&anim_pulse);
    
    Serial.println("[UI] Animation started for title");
    
    // Spinner de chargement
    lv_obj_t* spinner = lv_spinner_create(scr);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    
    // Message de statut WiFi (sera mis à jour par WifiManager)
    wifi_status_label = lv_label_create(scr);
    lv_label_set_text(wifi_status_label, "Connexion WiFi...");
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_CENTER, 0, 80);
    
    // Footer "Powered by Silexperience"
    lv_obj_t* footer = lv_label_create(scr);
    lv_label_set_text(footer, "Powered by Silexperience");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFF9500), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    welcome_screen = scr;
    
    // Forcer le rafraîchissement de l'écran
    lv_obj_invalidate(scr);
    lv_refr_now(NULL);
    
    current_screen = WELCOME_SCREEN;
    
    Serial.println("[UI] Welcome screen created");
}

// Forward declarations for callback functions
static void miner_restart_cb(lv_event_t * e);
static void miner_reboot_cb(lv_event_t * e);
static void miner_config_cb(lv_event_t * e);
static void global_restart_all_cb(lv_event_t * e);
static void global_refresh_all_cb(lv_event_t * e);
static void global_sort_hashrate_cb(lv_event_t * e);
static void global_filter_online_cb(lv_event_t * e);
static void global_filter_issues_cb(lv_event_t * e);

// Gestion du clic gauche/droite pour naviguer entre écrans
static void screen_touch_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_get_act();
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    uint32_t now = millis();

    if (code == LV_EVENT_PRESSED) {
        // Début du toucher : enregistrer position et temps
        touch_start_x = point.x;
        touch_start_y = point.y;
        last_touch_time = now;
        Serial.printf("[TOUCH] Pressed at X=%d Y=%d, current_screen=%d\n", point.x, point.y, current_screen);
    }
    else if (code == LV_EVENT_RELEASED) {
        // Calculer le mouvement
        int32_t dx = point.x - touch_start_x;
        int32_t dy = point.y - touch_start_y;
        uint32_t duration = now - last_touch_time;

        Serial.printf("[TOUCH] Released at X=%d, dx=%d dy=%d, duration=%dms, current_screen=%d\n",
                      point.x, dx, dy, duration, current_screen);

        // Détecter les swipes (mouvement horizontal > 30px, durée < 800ms, mouvement vertical limité)
        if (abs(dx) > 30 && abs(dy) < 80 && duration < 800 && current_screen == MINERS_SCREEN) {
            if (dx > 0) {
                // Swipe droite -> mineur précédent
                Serial.println("[TOUCH] Swipe right detected - previous miner");
                navigateCarousel(false);
                return;
            } else {
                // Swipe gauche -> mineur suivant
                Serial.println("[TOUCH] Swipe left detected - next miner");
                navigateCarousel(true);
                return;
            }
        }

        // Clic rapide sans mouvement (<40px, <600ms)
        if (abs(dx) < 40 && abs(dy) < 40 && duration < 600) {
            // DOUBLE-CLIC : 2 clics rapides (seuils plus permissifs)
            uint32_t time_since_last_click = now - last_click_time;

            if (time_since_last_click < 500 && click_count == 1) {
                // DOUBLE-CLIC détecté - retour immédiat
                Serial.println("[UI] *** DOUBLE-CLICK DETECTED - Returning to clock ***");
                click_count = 0;
                if (current_screen == MINERS_SCREEN) {
                    pending_screen_change = true;
                    pending_screen_target = 1;  // CLOCK
                    return;
                }
            } else {
                // Premier clic
                click_count = 1;
                last_click_time = now;
                Serial.println("[TOUCH] First click registered for double-click");
            }

            // Reset automatique après 500ms (plus permissif)
            static lv_timer_t* double_click_timer = nullptr;
            if (double_click_timer) {
                lv_timer_delete(double_click_timer);
            }
            double_click_timer = lv_timer_create([](lv_timer_t* timer) {
                click_count = 0;
                Serial.println("[TOUCH] Double-click timer expired, resetting counter");
            }, 500, nullptr);
            lv_timer_set_repeat_count(double_click_timer, 1);
        } else {
            Serial.printf("[TOUCH] Ignored: dx=%d dy=%d, duration=%d\n", dx, dy, duration);
        }
    }
}

// Fonction pour animer le slide d'un label avec nouvelle valeur
static void animateLabelSlide(lv_obj_t* label, const char* new_text) {
    if (label == nullptr || new_text == nullptr) return;
    
    // Si une animation est déjà en cours, terminer immédiatement
    if (animating_label != nullptr) {
        lv_anim_delete(animating_label, nullptr);
        lv_obj_set_x(animating_label, original_label_x);
        lv_label_set_text(animating_label, new_label_text);
        animating_label = nullptr;
    }
    
    // Préparer les variables pour l'animation
    animating_label = label;
    new_label_text = new_text;
    original_label_x = lv_obj_get_x(label);
    
    // Animation de slide vers la droite (sortie)
    lv_anim_t slide_out;
    lv_anim_init(&slide_out);
    lv_anim_set_var(&slide_out, label);
    lv_anim_set_values(&slide_out, original_label_x, original_label_x + 40);  // Slide vers la droite
    lv_anim_set_time(&slide_out, 250);  // 250ms
    lv_anim_set_exec_cb(&slide_out, [](void * var, int32_t v) {
        lv_obj_set_x((lv_obj_t*)var, v);
    });
    
    // Callback quand l'animation de sortie est terminée
    lv_anim_set_ready_cb(&slide_out, [](lv_anim_t* a) {
        if (animating_label && new_label_text) {
            // Changer le texte
            lv_label_set_text(animating_label, new_label_text);
            
            // Animation d'entrée depuis la gauche
            lv_anim_t slide_in;
            lv_anim_init(&slide_in);
            lv_anim_set_var(&slide_in, animating_label);
            lv_anim_set_values(&slide_in, original_label_x - 40, original_label_x);  // Depuis la gauche
            lv_anim_set_time(&slide_in, 250);  // 250ms
            lv_anim_set_exec_cb(&slide_in, [](void * var, int32_t v) {
                lv_obj_set_x((lv_obj_t*)var, v);
            });
            
            // Callback de fin d'animation d'entrée
            lv_anim_set_ready_cb(&slide_in, [](lv_anim_t* a) {
                // Nettoyer
                animating_label = nullptr;
                new_label_text = nullptr;
            });
            
            lv_anim_start(&slide_in);
        }
    });
    
    // Démarrer l'animation de sortie
    lv_anim_start(&slide_out);
}

// Fonction pour créer un affichage simple des données de bloc (valeurs verticales dans un carré)
static void createBlockDataDisplay(lv_obj_t* parent) {
    // Créer le conteneur principal pour les données de bloc (rectangle allongé)
    block_data_container = lv_obj_create(parent);
    lv_obj_set_size(block_data_container, 85, 85);  // Carré compact 85x85
    lv_obj_set_style_bg_color(block_data_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(block_data_container, 1, 0);
    lv_obj_set_style_border_color(block_data_container, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_radius(block_data_container, 6, 0);
    lv_obj_set_style_pad_all(block_data_container, 3, 0);  // Padding adapté au carré 85x85
    lv_obj_set_flex_flow(block_data_container, LV_FLEX_FLOW_COLUMN);  // Layout vertical
    lv_obj_set_style_pad_row(block_data_container, 2, 0);  // Espacement vertical réduit

    // Label pour la hauteur du bloc (exacte)
    block_height_label = lv_label_create(block_data_container);
    lv_label_set_text(block_height_label, "---");
    lv_obj_set_style_text_font(block_height_label, &lv_font_montserrat_16, 0);  // Police disponible la plus petite
    lv_obj_set_style_text_color(block_height_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(block_height_label, LV_TEXT_ALIGN_CENTER, 0);  // Centré

    // Label pour les frais (min - high)
    block_fees_label = lv_label_create(block_data_container);
    lv_label_set_text(block_fees_label, "---");
    lv_obj_set_style_text_font(block_fees_label, &lv_font_montserrat_16, 0);  // Police disponible la plus petite
    lv_obj_set_style_text_color(block_fees_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(block_fees_label, LV_TEXT_ALIGN_CENTER, 0);  // Centré

    // Label pour le temps chronométré depuis la création
    block_age_label = lv_label_create(block_data_container);
    lv_label_set_text(block_age_label, "---");
    lv_obj_set_style_text_font(block_age_label, &lv_font_montserrat_16, 0);  // Police disponible la plus petite
    lv_obj_set_style_text_color(block_age_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(block_age_label, LV_TEXT_ALIGN_CENTER, 0);  // Centré

    // Label pour la pool qui a trouvé le bloc
    block_pool_label = lv_label_create(block_data_container);
    lv_label_set_text(block_pool_label, "---");
    lv_obj_set_style_text_font(block_pool_label, &lv_font_montserrat_16, 0);  // Police disponible la plus petite
    lv_obj_set_style_text_color(block_pool_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(block_pool_label, LV_TEXT_ALIGN_CENTER, 0);  // Centré
}

void UI::showClockScreen() {
    Serial.println("[UI] ========== Showing clock screen ==========");
    
    // Arrêter le timer du cache des stats quand on quitte l'écran Miners
    if (stats_cache_timer != nullptr) {
        lv_timer_pause(stats_cache_timer);
        Serial.println("[UI] Stats cache timer paused");
    }
    
    // Nettoyer les carrés tombants de l'écran précédent
    Serial.println("[UI] Cleaning up old falling squares...");
    cleanupFallingSquares();
    
    // Nettoyer l'écran actuel
    lv_obj_t* scr = lv_screen_active();
    Serial.println("[UI] Cleaning screen...");
    lv_obj_clean(scr);
    
    // Réinitialiser les pointeurs
    time_label = nullptr;
    date_label = nullptr;
    bitcoin_price_label = nullptr;
    sats_conversion_label = nullptr;
    weather_label = nullptr;
    weather_icon_label = nullptr;
    block_data_cube = nullptr;
    block_data_container = nullptr;
    block_height_label = nullptr;
    block_fees_label = nullptr;
    block_age_label = nullptr;
    block_pool_label = nullptr;
    best_diff_label = nullptr;
    total_power_label = nullptr;
    hashrate_total_label = nullptr;
    hashrate_sum_label = nullptr;
    bitaxe_container = nullptr;
    btn_scroll_up = nullptr;
    btn_scroll_down = nullptr;
    
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);  // Fond noir
    
    Serial.println("[UI] *** CALLING createFallingSquares() ***");
    // Créer les carrés tombants en arrière-plan (effet Matrix/Tetris)
    createFallingSquares(scr);
    Serial.println("[UI] *** createFallingSquares() RETURNED ***");
    
    // RÉACTIVER l'animation des carrés pour l'écran Clock
    squares_animation_active = true;
    Serial.println("[UI] Falling squares animation RESUMED for Clock screen");
    
    // Récupérer l'heure et la date depuis TimeManager
    TimeManager* tm = TimeManager::getInstance();
    String timeStr = tm->getFormattedTime();  // HH:MM:SS avec secondes
    String dateStr = tm->getFormattedDate();
    
    // Horloge - Heure en TRÈS GROS au centre
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, timeStr.c_str());
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -30);
    
    // Date sous l'heure
    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, dateStr.c_str());
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x808080), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 25);
    
    // Container pour météo (icône + température côte à côte)
    static lv_obj_t* weather_container = nullptr;

    weather_container = lv_obj_create(scr);
    lv_obj_set_size(weather_container, 100, 45);
    lv_obj_align(weather_container, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_opa(weather_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(weather_container, 0, 0);
    lv_obj_clear_flag(weather_container, LV_OBJ_FLAG_SCROLLABLE);

    // Icône météo (Font Awesome)
    weather_icon_label = lv_label_create(weather_container);
    lv_label_set_text(weather_icon_label, "\xEF\x80\x82"); // Soleil par défaut (0xF002)
    lv_obj_set_style_text_font(weather_icon_label, &font_awesome_weather_32, 0);
    lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0xFFA500), 0);
    lv_obj_align(weather_icon_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Température (police normale)
    weather_label = lv_label_create(weather_container);
    lv_label_set_text(weather_label, "--°");
    lv_obj_set_style_text_font(weather_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(weather_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(weather_label, LV_ALIGN_RIGHT_MID, 0, 0);
    
    // Prix Bitcoin en haut à droite - Design futuriste
    bitcoin_price_label = lv_label_create(scr);
    lv_label_set_text(bitcoin_price_label, "BTC\n$--,---");
    lv_obj_set_style_text_font(bitcoin_price_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(bitcoin_price_label, lv_color_hex(0x00FFAA), 0);  // Cyan/turquoise électrique
    lv_obj_set_style_text_align(bitcoin_price_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(bitcoin_price_label, LV_ALIGN_TOP_RIGHT, -15, 8);  // Marge de 15px à droite
    
    // Style futuriste: bordure subtile avec ombre
    lv_obj_set_style_border_width(bitcoin_price_label, 1, 0);
    lv_obj_set_style_border_color(bitcoin_price_label, lv_color_hex(0x00FFAA), 0);
    lv_obj_set_style_border_opa(bitcoin_price_label, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(bitcoin_price_label, 4, 0);
    lv_obj_set_style_radius(bitcoin_price_label, 4, 0);
    lv_obj_set_style_bg_color(bitcoin_price_label, lv_color_hex(0x001a14), 0);  // Fond sombre légèrement vert
    lv_obj_set_style_bg_opa(bitcoin_price_label, LV_OPA_60, 0);
    
    // Conversion 1$ en Sats en haut à gauche - Même style que BTC
    sats_conversion_label = lv_label_create(scr);
    lv_label_set_text(sats_conversion_label, "1$ = --- Sats");
    lv_obj_set_style_text_font(sats_conversion_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sats_conversion_label, lv_color_hex(0x00FFAA), 0);  // Cyan/turquoise électrique
    lv_obj_set_style_text_align(sats_conversion_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(sats_conversion_label, LV_ALIGN_TOP_LEFT, 15, 8);  // Marge de 15px à gauche
    
    // Style futuriste identique: bordure subtile avec ombre
    lv_obj_set_style_border_width(sats_conversion_label, 1, 0);
    lv_obj_set_style_border_color(sats_conversion_label, lv_color_hex(0x00FFAA), 0);
    lv_obj_set_style_border_opa(sats_conversion_label, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(sats_conversion_label, 4, 0);
    lv_obj_set_style_radius(sats_conversion_label, 4, 0);
    lv_obj_set_style_bg_color(sats_conversion_label, lv_color_hex(0x001a14), 0);  // Fond sombre légèrement vert
    lv_obj_set_style_bg_opa(sats_conversion_label, LV_OPA_60, 0);
    
    // Données de bloc - Affichage structuré horizontal
    createBlockDataDisplay(scr);
    
    // Positionner le container de données de bloc centré verticalement contre le côté gauche
    if (block_data_container != nullptr) {
        lv_obj_align(block_data_container, LV_ALIGN_LEFT_MID, 15, -15);  // Remonté de 3mm (15px), 15px du bord gauche
    }
    
    // Best Diff en bas à gauche - Même style que BTC/Sats
    best_diff_label = lv_label_create(scr);
    lv_label_set_text(best_diff_label, "Best Diff\n---");
    lv_obj_set_style_text_font(best_diff_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(best_diff_label, lv_color_hex(0xFFAA00), 0);  // Orange doré pour mining
    lv_obj_set_style_text_align(best_diff_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(best_diff_label, LV_ALIGN_BOTTOM_LEFT, 15, -8);  // Coin bas-gauche, marge 15px
    
    // Style futuriste identique: bordure subtile avec ombre
    lv_obj_set_style_border_width(best_diff_label, 1, 0);
    lv_obj_set_style_border_color(best_diff_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_border_opa(best_diff_label, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(best_diff_label, 4, 0);
    lv_obj_set_style_radius(best_diff_label, 4, 0);
    lv_obj_set_style_bg_color(best_diff_label, lv_color_hex(0x1a1400), 0);  // Fond sombre légèrement orange
    lv_obj_set_style_bg_opa(best_diff_label, LV_OPA_60, 0);
    
    // Consommation totale juste au-dessus de Best Diff
    total_power_label = lv_label_create(scr);
    lv_label_set_text(total_power_label, LV_SYMBOL_CHARGE " ---W");  // Symbole charge LVGL + watts
    lv_obj_set_style_text_font(total_power_label, &lv_font_montserrat_14, 0);  // Police plus petite
    lv_obj_set_style_text_color(total_power_label, lv_color_hex(0xFF3300), 0);  // Rouge-orange vif pour l'électricité
    lv_obj_set_style_text_align(total_power_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(total_power_label, LV_ALIGN_BOTTOM_LEFT, 15, -65);  // 57px au-dessus de Best Diff (remonté de 3mm)
    
    // Style futuriste identique
    lv_obj_set_style_border_width(total_power_label, 1, 0);
    lv_obj_set_style_border_color(total_power_label, lv_color_hex(0xFF3300), 0);
    lv_obj_set_style_border_opa(total_power_label, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(total_power_label, 4, 0);
    lv_obj_set_style_radius(total_power_label, 4, 0);
    lv_obj_set_style_bg_color(total_power_label, lv_color_hex(0x1a0500), 0);  // Fond sombre légèrement rouge
    lv_obj_set_style_bg_opa(total_power_label, LV_OPA_60, 0);
    
    // Hashrate total (somme en GH/s) sous la date
    hashrate_sum_label = lv_label_create(scr);
    lv_label_set_text(hashrate_sum_label, "0.0 GH/s");
    lv_obj_set_style_text_font(hashrate_sum_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(hashrate_sum_label, lv_color_hex(0xFF6600), 0);  // Orange
    lv_obj_align(hashrate_sum_label, LV_ALIGN_CENTER, 0, 55);
    
    // Nombre de miners online sous le hashrate
    hashrate_total_label = lv_label_create(scr);
    
    // Calculer le nombre de miners online
    WifiManager* wifi = WifiManager::getInstance();
    int onlineCount = 0;
    for (int i = 0; i < wifi->getBitaxeCount(); i++) {
        BitaxeDevice* device = wifi->getBitaxe(i);
        if (device && device->online) {
            onlineCount++;
        }
    }
    
    char hashrate_text[64];
    snprintf(hashrate_text, sizeof(hashrate_text), "%d miners online", onlineCount);
    lv_label_set_text(hashrate_total_label, hashrate_text);
    lv_obj_set_style_text_font(hashrate_total_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hashrate_total_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(hashrate_total_label, LV_ALIGN_CENTER, 0, 85);
    
    // Bouton futuriste À DROITE (vertical, style avion de chasse)
    lv_obj_t* click_hint_container = lv_obj_create(scr);
    lv_obj_set_size(click_hint_container, 70, 180);  // Vertical : 70px large, 180px haut
    
    // Style futuriste avec bords ciselés
    lv_obj_set_style_bg_color(click_hint_container, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_bg_opa(click_hint_container, 200, 0);
    
    // Bordures ciselées (pas de radius pour effet anguleux)
    lv_obj_set_style_radius(click_hint_container, 0, 0);
    lv_obj_set_style_border_width(click_hint_container, 2, 0);
    lv_obj_set_style_border_color(click_hint_container, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_opa(click_hint_container, 180, 0);
    
    // Bordures ciselées diagonales (effet avion de chasse)
    lv_obj_set_style_outline_width(click_hint_container, 1, 0);
    lv_obj_set_style_outline_color(click_hint_container, lv_color_hex(0xFF4040), 0);
    lv_obj_set_style_outline_opa(click_hint_container, 120, 0);
    lv_obj_set_style_outline_pad(click_hint_container, 3, 0);
    
    // Ombre rouge intense (glow futuriste)
    lv_obj_set_style_shadow_width(click_hint_container, 20, 0);
    lv_obj_set_style_shadow_color(click_hint_container, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_shadow_opa(click_hint_container, 120, 0);
    lv_obj_set_style_shadow_spread(click_hint_container, 3, 0);
    
    lv_obj_set_style_pad_all(click_hint_container, 8, 0);
    
    // Positionné à DROITE, légèrement en dessous du centre
    lv_obj_align(click_hint_container, LV_ALIGN_RIGHT_MID, -10, 30);
    
    // Texte vertical avec flèche
    lv_obj_t* swipe_hint = lv_label_create(click_hint_container);
    lv_label_set_text(swipe_hint, LV_SYMBOL_RIGHT "\n\nM\nI\nN\nE\nR\nS");  // Texte vertical
    lv_obj_set_style_text_font(swipe_hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(swipe_hint, lv_color_hex(0xFF3030), 0);
    lv_obj_set_style_text_align(swipe_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(swipe_hint);
    
    // Rendre le bouton rouge CLIQUABLE pour aller vers Miners
    lv_obj_add_flag(click_hint_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(click_hint_container, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            Serial.println("[UI] *** RED BUTTON CLICKED - Going to Miners ***");
            UI::getInstance().showMinersScreen();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    // Détecter les clics sur le reste de l'écran (zone gauche pour retour éventuel)
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);  // Nécessaire pour recevoir les events touch
    
    Serial.printf("[UI] Time/date labels created: time_label=%p, date_label=%p\n", time_label, date_label);
    
    clock_screen = scr;
    
    // IMPORTANT: Définir current_screen AVANT d'appeler le callback du timer
    current_screen = CLOCK_SCREEN;
    
    // Créer ou reprendre le timer LVGL pour mettre à jour l'horloge toutes les 15 secondes
    if (time_update_timer == nullptr) {
        Serial.println("[UI] Creating time update timer (15s interval)");
        time_update_timer = lv_timer_create(update_time_cb, 15000, NULL);  // Toutes les 15 secondes
        if (time_update_timer != nullptr) {
            // CRITICAL: Force infinite repeat (LVGL v9 doc)
            lv_timer_set_repeat_count(time_update_timer, -1);
            // Reset timer to ensure it starts (workaround for ESP32)
            lv_timer_reset(time_update_timer);
            Serial.printf("[UI] Time timer created and started, paused=%d\n", lv_timer_get_paused(time_update_timer));
        } else {
            Serial.println("[UI] ERROR: Failed to create timer!");
        }
    } else {
        Serial.printf("[UI] Reusing existing timer, paused=%d\n", lv_timer_get_paused(time_update_timer));
        lv_timer_resume(time_update_timer);
        lv_timer_reset(time_update_timer);
    }
    
    Serial.println("[UI] Clock screen created");
}

void UI::showMinersScreen() {
    Serial.println("[UI] Showing miners screen...");
    
    // Nettoyer les carrés tombants pour éviter les conflits mémoire
    Serial.println("[UI] Cleaning up falling squares for Miners screen...");
    cleanupFallingSquares();
    
    // DÉSACTIVER l'animation des carrés pour meilleures performances
    squares_animation_active = false;
    Serial.println("[UI] Falling squares cleaned up and animation disabled for Miners screen");
    
    // Pause le timer de l'horloge pour économiser CPU
    if (time_update_timer != nullptr) {
        lv_timer_pause(time_update_timer);
        Serial.println("[UI] Time timer paused");
    }
    
    // Nettoyer l'écran actuel
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);

    // Réinitialiser les pointeurs
    time_label = nullptr;
    date_label = nullptr;
    hashrate_total_label = nullptr;
    bitaxe_container = nullptr;
    weather_label = nullptr;

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    // Container principal vertical (flex column) - AJUSTÉ À LA TAILLE ÉCRAN
    lv_obj_t* main_container = lv_obj_create(scr);
    lv_obj_set_size(main_container, 470, 262); // Légèrement plus petit pour laisser une marge visible
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_style_bg_color(main_container, lv_color_hex(0x0a0a0a), 0); // Fond légèrement différent
    lv_obj_set_style_border_width(main_container, 2, 0); // Bordure visible
    lv_obj_set_style_border_color(main_container, lv_color_hex(0x333333), 0); // Bordure grise
    lv_obj_set_style_radius(main_container, 5, 0); // Coins arrondis
    lv_obj_align(main_container, LV_ALIGN_TOP_LEFT, 5, 5); // Décalé vers la droite et vers le bas pour compenser les bordures physiques

    // Titre - RÉDUIT
    lv_obj_t* title = lv_label_create(main_container);
    lv_label_set_text(title, "MINEURS BITAXE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0); // Police réduite
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF0000), 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_obj_set_style_pad_top(title, 5, 0); // Marge réduite
    lv_obj_set_style_pad_bottom(title, 2, 0); // Marge réduite
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    // Container scrollable pour les mineurs - AJUSTÉ
    bitaxe_container = lv_obj_create(main_container);
    lv_obj_set_size(bitaxe_container, 450, 160); // Réduit pour mieux centrer et éviter les bordures
    lv_obj_set_style_bg_color(bitaxe_container, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_border_width(bitaxe_container, 0, 0);
    lv_obj_set_flex_flow(bitaxe_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(bitaxe_container, 5, 0);
    lv_obj_set_style_pad_row(bitaxe_container, 3, 0); // Padding réduit
    // SUPPRIMER les barres de scroll pour un look plus propre
    lv_obj_set_scrollbar_mode(bitaxe_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(bitaxe_container, LV_DIR_VER);
    lv_obj_add_flag(bitaxe_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bitaxe_container, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(bitaxe_container, LV_OBJ_FLAG_CLICKABLE);  // NE PAS intercepter les clics
    lv_obj_set_flex_grow(bitaxe_container, 1); // Permet au container de s'étendre

    // Indicateur de nombre de mineurs - RÉDUIT
    lv_obj_t* status_container = lv_obj_create(main_container);
    lv_obj_set_size(status_container, 450, 25); // Ajusté pour correspondre au bitaxe_container
    lv_obj_set_style_bg_color(status_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(status_container, 1, 0);
    lv_obj_set_style_border_color(status_container, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(status_container, 3, 0); // Radius réduit
    lv_obj_set_style_pad_all(status_container, 3, 0); // Padding réduit
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_ROW); // Layout horizontal
    lv_obj_set_style_pad_column(status_container, 5, 0); // Espacement entre éléments

    // Bouton Prev
    lv_obj_t* btn_prev = lv_button_create(status_container);
    lv_obj_set_size(btn_prev, 60, 20); // Bouton compact
    lv_obj_set_style_bg_color(btn_prev, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(btn_prev, 1, 0);
    lv_obj_set_style_border_color(btn_prev, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(btn_prev, 2, 0);
    lv_obj_add_flag(btn_prev, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_prev, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            static unsigned long last_click = 0;
            unsigned long now = millis();
            if (now - last_click > 200) { // Debounce: 200ms minimum entre clics
                last_click = now;
                Serial.println("[UI] Prev button clicked - navigating to previous miner");
                navigateCarousel(false); // false = previous
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label_prev = lv_label_create(btn_prev);
    lv_label_set_text(label_prev, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(label_prev, &lv_font_montserrat_16, 0);
    lv_obj_center(label_prev);

    // Indicateur de page (maintenant nombre total) - CENTRE
    page_indicator = lv_label_create(status_container);
    lv_obj_set_flex_grow(page_indicator, 1); // Prend l'espace restant au centre
    lv_obj_set_style_text_font(page_indicator, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(page_indicator, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_align(page_indicator, LV_TEXT_ALIGN_CENTER, 0);

    // Bouton Next
    lv_obj_t* btn_next = lv_button_create(status_container);
    lv_obj_set_size(btn_next, 60, 20); // Bouton compact
    lv_obj_set_style_bg_color(btn_next, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(btn_next, 1, 0);
    lv_obj_set_style_border_color(btn_next, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(btn_next, 2, 0);
    lv_obj_add_flag(btn_next, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_next, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            static unsigned long last_click = 0;
            unsigned long now = millis();
            if (now - last_click > 200) { // Debounce: 200ms minimum entre clics
                last_click = now;
                Serial.println("[UI] Next button clicked - navigating to next miner");
                navigateCarousel(true); // true = next
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* label_next = lv_label_create(btn_next);
    lv_label_set_text(label_next, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(label_next, &lv_font_montserrat_16, 0);
    lv_obj_center(label_next);
    
    // SUPPRIMER l'événement GESTURE qui interfère avec les événements tactiles
    // lv_obj_add_event_cb(bitaxe_container, [](lv_event_t * e) {
    //     if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
    //         lv_dir_t dir = lv_indev_get_gesture_dir(lv_event_get_indev(e));
    //         if (dir == LV_DIR_RIGHT) {
    //             Serial.println("[UI] Swipe droite détecté - retour à l'écran principal");
    //             UI::getInstance().showClockScreen();
    //         }
    //     }
    // }, LV_EVENT_GESTURE, nullptr);

    // Message de chargement
    lv_obj_t* loading_msg = lv_label_create(bitaxe_container);
    lv_label_set_text(loading_msg, "Loading data...\n\n" LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(loading_msg, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(loading_msg, lv_color_hex(0xFF6600), 0);
    lv_obj_set_style_text_align(loading_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(loading_msg);

    // Force un refresh pour afficher le message immédiatement
    lv_refr_now(NULL);

    // Marquer qu'on est sur l'écran Miners AVANT de faire les requêtes
    current_screen = MINERS_SCREEN;
    miners_screen = scr;

    // Charger les stats Bitaxe (appel API seulement ici!)
    refreshBitaxeStats();

    // Démarrer le timer de rafraîchissement du cache (30 secondes)
    if (stats_cache_timer == nullptr) {
        Serial.println("[UI] Starting stats cache refresh timer (30s)");
        stats_cache_timer = lv_timer_create(refresh_stats_cache_cb, 30000, NULL);
        lv_timer_set_repeat_count(stats_cache_timer, -1);
    } else {
        Serial.println("[UI] Resuming stats cache refresh timer");
        lv_timer_resume(stats_cache_timer);
        lv_timer_reset(stats_cache_timer);
    }

    // Container pour les boutons globaux (Back/Config) - NON scrollable, placé sur l'écran principal
    static lv_obj_t* nav_container = nullptr;
    if (!nav_container) {
        nav_container = lv_obj_create(scr); // scr = root screen, pas main_container
        lv_obj_set_size(nav_container, 450, 22); // Ajusté pour correspondre aux autres containers
        lv_obj_set_style_bg_color(nav_container, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_width(nav_container, 1, 0);
        lv_obj_set_style_border_color(nav_container, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_radius(nav_container, 2, 0); // Radius réduit
        lv_obj_set_style_pad_all(nav_container, 2, 0); // Padding réduit
        lv_obj_set_flex_flow(nav_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(nav_container, 8, 0); // Espacement réduit
        lv_obj_align(nav_container, LV_ALIGN_TOP_MID, 0, 2); // Plus près du haut

        // Bouton Back - DESIGN AMÉLIORÉ, PLUS FIN
        lv_obj_t* btn_back = lv_button_create(nav_container);
        lv_obj_set_size(btn_back, 100, 18); // Dimensions réduites
        lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2a2a2a), 0); // Fond plus sombre
        lv_obj_set_style_border_width(btn_back, 1, 0);
        lv_obj_set_style_border_color(btn_back, lv_color_hex(0x666666), 0);
        lv_obj_set_style_radius(btn_back, 1, 0); // Très peu de radius
        lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
            if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
                Serial.println("[UI] Back - returning to clock");
                current_miner_index = 0;  // Reset carousel to first miner
                UI::getInstance().showClockScreen();
            }
        }, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* label_back = lv_label_create(btn_back);
        lv_label_set_text(label_back, LV_SYMBOL_LEFT " Back");
        lv_obj_set_style_text_font(label_back, &lv_font_montserrat_16, 0); // Police réduite
        lv_obj_center(label_back);

        // Bouton Config - DESIGN AMÉLIORÉ, PLUS FIN
        lv_obj_t* btn_config = lv_button_create(nav_container);
        lv_obj_set_size(btn_config, 100, 18); // Dimensions réduites
        lv_obj_set_style_bg_color(btn_config, lv_color_hex(0x2a2a2a), 0); // Fond plus sombre
        lv_obj_set_style_border_width(btn_config, 1, 0);
        lv_obj_set_style_border_color(btn_config, lv_color_hex(0x666666), 0);
        lv_obj_set_style_radius(btn_config, 1, 0); // Très peu de radius
        lv_obj_add_flag(btn_config, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_config, [](lv_event_t * e) {
            if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
                Serial.println("[UI] Config - open config screen");
                UI::getInstance().showClockScreen(); // showConfigScreen n'existe pas
            }
        }, LV_EVENT_CLICKED, nullptr);
        lv_obj_t* label_config = lv_label_create(btn_config);
        lv_label_set_text(label_config, LV_SYMBOL_SETTINGS " Config");
        lv_obj_set_style_text_font(label_config, &lv_font_montserrat_16, 0); // Police réduite
        lv_obj_center(label_config);
    }
    
    Serial.println("[UI] Miners screen created with enhanced controls");
    
    // ACTIVER screen_touch_cb pour détecter les double-clics sur Miners screen
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);  // Nécessaire pour recevoir les events touch
    
    // Le bouton Back gère le retour à l'écran horloge
    // Auto-refresh géré par millis() timer dans main.cpp (10s)
    
    // Note: miners_screen et current_screen déjà définis plus haut avant refreshBitaxeStats()
    
    Serial.println("[UI] Miners screen created (auto-refresh every 10s via main loop) - Double-click to return");
}

void UI::showDashboard() {
    Serial.println("[UI] showDashboard() called - redirecting to showClockScreen()");
    showClockScreen();  // Rediriger vers la nouvelle page horloge
}

void UI::createTheme() {
    // Thème sombre par défaut avec bleu comme couleur primaire
    theme = lv_theme_default_init(disp, 
        lv_palette_main(LV_PALETTE_BLUE),   // Couleur primaire
        lv_palette_main(LV_PALETTE_CYAN),   // Couleur secondaire  
        true,                               // Mode sombre
        LV_FONT_DEFAULT);
    
    if (disp) {
        lv_display_set_theme(disp, theme);
        Serial.println("[UI] Theme applied");
    }
}

void UI::showSplashScreen() {
    Serial.println("[UI] Showing splash screen...");
    
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    
    // Logo/Titre
    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "TouchAxe");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);  // Utiliser montserrat_28
    lv_obj_set_style_text_color(label, lv_color_hex(0x1E90FF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);
    
    // Version
    lv_obj_t* version = lv_label_create(scr);
    lv_label_set_text(version, "v1.0.0");
    lv_obj_set_style_text_color(version, lv_color_hex(0x808080), 0);
    lv_obj_align(version, LV_ALIGN_CENTER, 0, 10);
    
    // Message de chargement
    lv_obj_t* loading = lv_label_create(scr);
    lv_label_set_text(loading, "Initialisation...");
    lv_obj_set_style_text_color(loading, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(loading, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    // Spinner
    lv_obj_t* spinner = lv_spinner_create(scr);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_BOTTOM_MID, 0, -80);
    
    Serial.println("[UI] Splash screen created");
}

void UI::update() {
    lv_timer_handler();
}

// Public method to update clock from main loop (bypass LVGL timer issues)
void UI::updateClock() {
    // Handle pending screen change (deferred to avoid crash during event callbacks)
    if (pending_screen_change) {
        pending_screen_change = false;
        Serial.printf("[UI] Executing deferred screen change to %d\n", pending_screen_target);
        
        if (pending_screen_target == 1) {  // CLOCK
            showClockScreen();
        } else if (pending_screen_target == 2) {  // MINERS
            showMinersScreen();
        }
        pending_screen_target = 0;
        return;  // Skip clock update this cycle
    }
    
    if (current_screen != CLOCK_SCREEN || time_label == NULL || date_label == NULL) {
        return;  // Not on clock screen
    }

    TimeManager* tm = TimeManager::getInstance();
    if (!tm->isTimeInitialized()) {
        lv_label_set_text(time_label, "Sync...");
        lv_obj_invalidate(time_label);
        return;
    }

    String timeStr = tm->getFormattedTime();  // HH:MM:SS
    String dateStr = tm->getFormattedDate();  // DD/MM/YYYY

    lv_label_set_text(time_label, timeStr.c_str());
    lv_label_set_text(date_label, dateStr.c_str());

    lv_obj_invalidate(time_label);
    lv_obj_invalidate(date_label);
    lv_obj_invalidate(lv_screen_active());
    
    // CRITICAL: Force immediate redraw (LVGL v9 + ESP32 RGB workaround)
    lv_refr_now(NULL);

    // Update hashrate labels (count + sum)
    WifiManager* wifi = WifiManager::getInstance();
    int onlineCount = 0;
    float totalHashrate = 0.0;
    
    for (int i = 0; i < wifi->getBitaxeCount(); i++) {
        BitaxeDevice* device = wifi->getBitaxe(i);
        if (device && device->online) {
            onlineCount++;
            // Note: device doesn't store hashrate, only checkBitaxeStatus() updates it
            // This just counts online devices, actual hashrate updated every 30s
        }
    }
    
    // Update miner count
    if (hashrate_total_label != NULL) {
        char hashrate_text[64];
        snprintf(hashrate_text, sizeof(hashrate_text), "%d miners online", onlineCount);
        lv_label_set_text(hashrate_total_label, hashrate_text);
        lv_obj_invalidate(hashrate_total_label);
    }
    
    // Note: hashrate_sum_label is updated by checkBitaxeStatus() every 30s
}

// Public method to update Bitcoin price display (called from main loop)
void UI::updateBitcoinPrice() {
    // Only update if on Clock screen and labels exist
    if (current_screen != CLOCK_SCREEN || bitcoin_price_label == NULL || sats_conversion_label == NULL) {
        return;
    }
    
    BitcoinAPI& btc = BitcoinAPI::getInstance();
    
    if (btc.isPriceValid()) {
        float price = btc.getPrice();
        char price_text[32];
        char sats_text[32];
        
        // Format prix BTC avec séparateurs de milliers
        int price_int = (int)price;
        if (price_int >= 1000000) {
            // Format: $1,234,567
            snprintf(price_text, sizeof(price_text), "BTC\n$%d,%03d,%03d", 
                     price_int / 1000000, 
                     (price_int / 1000) % 1000, 
                     price_int % 1000);
        } else if (price_int >= 1000) {
            // Format: $89,234
            snprintf(price_text, sizeof(price_text), "BTC\n$%d,%03d", 
                     price_int / 1000, 
                     price_int % 1000);
        } else {
            // Format: $234
            snprintf(price_text, sizeof(price_text), "BTC\n$%d", price_int);
        }
        
        // Calculer combien de Sats pour 1$ (1 BTC = 100,000,000 Sats)
        // Sats = 100,000,000 / prix_BTC
        int sats_per_dollar = (int)(100000000.0 / price);
        
        // Format Sats avec séparateurs si nécessaire (une seule ligne)
        if (sats_per_dollar >= 1000) {
            snprintf(sats_text, sizeof(sats_text), "1$ = %d,%03d Sats", 
                     sats_per_dollar / 1000, 
                     sats_per_dollar % 1000);
        } else {
            snprintf(sats_text, sizeof(sats_text), "1$ = %d Sats", sats_per_dollar);
        }
        
        lv_label_set_text(bitcoin_price_label, price_text);
        lv_label_set_text(sats_conversion_label, sats_text);
    } else {
        lv_label_set_text(bitcoin_price_label, "BTC\n$--,---");
        lv_label_set_text(sats_conversion_label, "1$ = --- Sats");
    }
    
    // Update block data with slide animation if available
    if (btc.isBlockDataValid() && block_height_label != NULL && block_fees_label != NULL && 
        block_age_label != NULL && block_pool_label != NULL) {
        uint32_t blockHeight = btc.getBlockHeight();
        float minFee = btc.getMinFee();
        float maxFee = btc.getMaxFee();
        uint32_t blockAgeMinutes = btc.getBlockAgeMinutes();
        String poolName = btc.getPoolName();
        
        // Check if values have changed
        static uint32_t lastBlockHeight = 0;
        static float lastMinFee = -1;
        static float lastMaxFee = -1;
        static uint32_t lastBlockAgeMinutes = 0;
        static String lastPoolName = "";
        
        bool heightChanged = (blockHeight != lastBlockHeight);
        bool feesChanged = (minFee != lastMinFee || maxFee != lastMaxFee);
        bool ageChanged = (blockAgeMinutes != lastBlockAgeMinutes);
        bool poolChanged = (poolName != lastPoolName);
        
        // Update last values
        lastBlockHeight = blockHeight;
        lastMinFee = minFee;
        lastMaxFee = maxFee;
        lastBlockAgeMinutes = blockAgeMinutes;
        lastPoolName = poolName;
        
        // Format block height (valeur complète)
        char height_text[16];
        snprintf(height_text, sizeof(height_text), "%u", blockHeight);
        
        // Format fees range (min - high)
        char fees_text[32];
        if (minFee >= 0 && maxFee >= 0) {
            int minFeeSats = (int)minFee;
            int maxFeeSats = (int)maxFee;
            snprintf(fees_text, sizeof(fees_text), "%d-%d sats", minFeeSats, maxFeeSats);
        } else {
            snprintf(fees_text, sizeof(fees_text), "---");
        }
        
        // Format block age (temps exacte chronométré)
        char age_text[32];
        if (blockAgeMinutes < 60) {
            snprintf(age_text, sizeof(age_text), "%um", blockAgeMinutes);
        } else {
            int hours = blockAgeMinutes / 60;
            if (hours < 24) {
                snprintf(age_text, sizeof(age_text), "%uh", hours);
            } else {
                int days = hours / 24;
                snprintf(age_text, sizeof(age_text), "%ud", days);
            }
        }
        
        // Format pool name (pool qui a trouvé)
        char pool_text[32];
        if (poolName.length() > 0 && poolName != "Unknown") {
            if (poolName.length() > 10) {
                String shortName = poolName.substring(0, 9) + "..";
                snprintf(pool_text, sizeof(pool_text), "%s", shortName.c_str());
            } else {
                snprintf(pool_text, sizeof(pool_text), "%s", poolName.c_str());
            }
        } else {
            snprintf(pool_text, sizeof(pool_text), "---");
        }
        
        // Apply slide animation for changed values
        if (heightChanged) {
            animateLabelSlide(block_height_label, height_text);
        } else {
            lv_label_set_text(block_height_label, height_text);
        }
        
        if (feesChanged) {
            animateLabelSlide(block_fees_label, fees_text);
        } else {
            lv_label_set_text(block_fees_label, fees_text);
        }
        
        if (ageChanged) {
            animateLabelSlide(block_age_label, age_text);
        } else {
            lv_label_set_text(block_age_label, age_text);
        }
        
        if (poolChanged) {
            animateLabelSlide(block_pool_label, pool_text);
        } else {
            lv_label_set_text(block_pool_label, pool_text);
        }
    } else {
        // Set default values for display labels
        if (block_height_label != NULL) lv_label_set_text(block_height_label, "---");
        if (block_fees_label != NULL) lv_label_set_text(block_fees_label, "---");
        if (block_age_label != NULL) lv_label_set_text(block_age_label, "---");
        if (block_pool_label != NULL) lv_label_set_text(block_pool_label, "---");
    }
    
    lv_obj_invalidate(bitcoin_price_label);
    lv_obj_invalidate(sats_conversion_label);
    if (block_data_container != NULL) lv_obj_invalidate(block_data_container);
    lv_refr_now(NULL);  // Force immediate redraw
}

// Public method to update falling squares animation (manual, like updateClock)
void UI::updateFallingSquares() {
    updateFallingSquaresInternal();
}

// Public method to check Bitaxe status periodically (called from main loop)
void UI::checkBitaxeStatus() {
    WifiManager* wifi = WifiManager::getInstance();
    
    // Skip if not on Clock screen (avoid conflicts with Miners screen)
    if (current_screen != CLOCK_SCREEN) {
        return;
    }
    
    // Skip if in AP mode (no internet access)
    if (wifi->isAPMode()) {
        return;
    }
    
    // Skip if not connected to WiFi
    if (!wifi->isConnected()) {
        return;
    }
    
    int bitaxeCount = wifi->getBitaxeCount();
    if (bitaxeCount == 0) {
        return;  // No devices configured
    }
    
    Serial.printf("[UI] Checking status of %d Bitaxe device(s)...\n", bitaxeCount);
    
    int onlineCount = 0;
    float totalHashrate = 0.0;
    float totalPower = 0.0;    // Consommation totale en Watts
    uint32_t maxBestDiff = 0;  // Track highest bestDiff across all miners
    
    // Quick check each device
    for (int i = 0; i < bitaxeCount; i++) {
        BitaxeDevice* device = wifi->getBitaxe(i);
        if (device == nullptr) continue;
        
        BitaxeAPI api;
        api.setDevice(device->ip);
        
        BitaxeStats stats;
        bool success = api.getStats(stats);
        
        device->online = success;
        
        if (success) {
            onlineCount++;
            totalHashrate += stats.hashrate;
            totalPower += stats.power;
            
            // Store stats in device structure
            device->bestDiff = stats.bestDiff;
            device->bestSessionDiff = stats.bestDiff;
            device->power = stats.power;
            
            // Track maximum across all miners
            if (stats.bestDiff > maxBestDiff) {
                maxBestDiff = stats.bestDiff;
            }
            
            // Ajouter les données au StatsManager pour l'historique
            StatsManager::getInstance().addDataPoint(device->ip, stats.hashrate, stats.temp, stats.power);
            
            Serial.printf("[UI]   [%d] %s - ONLINE (%.1f GH/s, %.1f°C, %.1fW, bestDiff=%u)\n", 
                i, device->name.c_str(), stats.hashrate, stats.temp, stats.power, stats.bestDiff);
        } else {
            Serial.printf("[UI]   [%d] %s - OFFLINE (%s)\n", 
                i, device->name.c_str(), device->ip.c_str());
            device->bestDiff = 0;
            device->bestSessionDiff = 0;
            device->power = 0;
        }
    }
    
    Serial.printf("[UI] Total: %d online, %.1f GH/s, %.1fW, Best Diff=%u\n", onlineCount, totalHashrate, totalPower, maxBestDiff);
    
    // Update clock screen labels if we're on it
    if (current_screen == CLOCK_SCREEN) {
        // Update hashrate sum
        if (hashrate_sum_label != NULL) {
            char hashrate_sum_text[64];
            snprintf(hashrate_sum_text, sizeof(hashrate_sum_text), "%.1f GH/s", totalHashrate);
            lv_label_set_text(hashrate_sum_label, hashrate_sum_text);
            lv_obj_invalidate(hashrate_sum_label);
        }
        
        // Update miner count
        if (hashrate_total_label != NULL) {
            char hashrate_text[64];
            snprintf(hashrate_text, sizeof(hashrate_text), "%d miners online", onlineCount);
            lv_label_set_text(hashrate_total_label, hashrate_text);
            lv_obj_invalidate(hashrate_total_label);
        }
        
        // Update Best Diff display (format simplifié avec unité)
        if (best_diff_label != NULL) {
            char diff_text[64];
            if (maxBestDiff > 0) {
                // Format simplifié avec unités M/G/T pour lisibilité
                if (maxBestDiff >= 1000000000000ULL) {
                    // Tera (billions): 1.2T
                    float value = maxBestDiff / 1000000000000.0;
                    snprintf(diff_text, sizeof(diff_text), "Best Diff\n%.1fT", value);
                } else if (maxBestDiff >= 1000000000) {
                    // Giga (milliards): 72.6G
                    float value = maxBestDiff / 1000000000.0;
                    snprintf(diff_text, sizeof(diff_text), "Best Diff\n%.1fG", value);
                } else if (maxBestDiff >= 1000000) {
                    // Mega (millions): 997.3M
                    float value = maxBestDiff / 1000000.0;
                    snprintf(diff_text, sizeof(diff_text), "Best Diff\n%.1fM", value);
                } else if (maxBestDiff >= 1000) {
                    // Kilo (milliers): 123.5K
                    float value = maxBestDiff / 1000.0;
                    snprintf(diff_text, sizeof(diff_text), "Best Diff\n%.1fK", value);
                } else {
                    // Moins de 1000
                    snprintf(diff_text, sizeof(diff_text), "Best Diff\n%u", maxBestDiff);
                }
            } else {
                snprintf(diff_text, sizeof(diff_text), "Best Session Diff\n---");
            }
            lv_label_set_text(best_diff_label, diff_text);
            lv_obj_invalidate(best_diff_label);
        }
        
        // Update Total Power display
        if (total_power_label != NULL) {
            char power_text[64];
            if (totalPower > 0) {
                if (totalPower >= 1000) {
                    // Plus de 1kW: afficher en kW
                    snprintf(power_text, sizeof(power_text), LV_SYMBOL_CHARGE " %.2fkW", totalPower / 1000.0);
                } else {
                    // Moins de 1kW: afficher en W
                    snprintf(power_text, sizeof(power_text), LV_SYMBOL_CHARGE " %.0fW", totalPower);
                }
            } else {
                snprintf(power_text, sizeof(power_text), LV_SYMBOL_CHARGE " ---W");
            }
            lv_label_set_text(total_power_label, power_text);
            lv_obj_invalidate(total_power_label);
        }
    }
}

// Public method to refresh miners data only if Miners screen is active (called from main loop)
void UI::refreshMinersIfActive() {
    // Only refresh if on Miners screen to avoid unnecessary API calls
    if (current_screen == MINERS_SCREEN) {
        Serial.println("[UI] Background refresh of miners data (10s timer)");
        refreshBitaxeStats();
    }
}

// Public method to update weather display (called from main loop)
void UI::updateWeatherDisplay() {
    // Only update if on Clock screen and labels exist
    if (current_screen != CLOCK_SCREEN || weather_label == NULL || weather_icon_label == NULL) {
        return;
    }
    
    WeatherManager* weather = WeatherManager::getInstance();
    
    if (weather->isWeatherValid()) {
        // Récupérer les données météo
        WeatherData data = weather->getWeatherData();
        
        // Mettre à jour la température
        char temp_text[16];
        snprintf(temp_text, sizeof(temp_text), "%.0f°", data.temperature);
        lv_label_set_text(weather_label, temp_text);
        
        // Mettre à jour l'icône selon la condition
        if (data.condition == "clear" || data.condition == "soleil") {
            lv_label_set_text(weather_icon_label, "\xEF\x80\x82"); // Soleil (0xF002)
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0xFFA500), 0);
        }
        else if (data.condition == "clouds" || data.condition == "nuage") {
            lv_label_set_text(weather_icon_label, "\xEF\x80\x88"); // Nuages (0xF008)
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0x808080), 0);
        }
        else if (data.condition == "rain" || data.condition == "pluie") {
            lv_label_set_text(weather_icon_label, "\xEF\x80\x8D"); // Pluie (0xF00D)
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0x4682B4), 0);
        }
        else if (data.condition == "thunderstorm" || data.condition == "orage") {
            lv_label_set_text(weather_icon_label, "\xEF\x80\x90"); // Orage (0xF010)
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0xFFD700), 0);
        }
        else if (data.condition == "snow" || data.condition == "neige") {
            lv_label_set_text(weather_icon_label, "\xEF\x80\x9B"); // Neige (0xF01B)
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0x87CEEB), 0);
        }
        else {
            // Par défaut: soleil
            lv_label_set_text(weather_icon_label, "\xEF\x80\x82");
            lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0xFFA500), 0);
        }
    } else {
        lv_label_set_text(weather_label, "--°");
        lv_label_set_text(weather_icon_label, "\xEF\x80\x82"); // Soleil par défaut
        lv_obj_set_style_text_color(weather_icon_label, lv_color_hex(0x808080), 0);
    }
    
    lv_obj_invalidate(weather_label);
    lv_obj_invalidate(weather_icon_label);
    lv_refr_now(NULL);  // Force immediate redraw
}

// Nouvelle fonction: Afficher l'écran de statistiques pour un mineur spécifique
void UI::showStatsScreen(int minerIndex) {
    Serial.printf("[UI] Showing statistics screen for miner %d...\n", minerIndex);
    
    WifiManager* wifi = WifiManager::getInstance();
    if (minerIndex < 0 || minerIndex >= wifi->getBitaxeCount()) {
        Serial.println("[UI] Invalid miner index, returning to miners screen");
        showMinersScreen();
        return;
    }
    
    BitaxeDevice* device = wifi->getBitaxe(minerIndex);
    if (!device) {
        Serial.println("[UI] Miner device not found, returning to miners screen");
        showMinersScreen();
        return;
    }
    
    // Nettoyer les carrés tombants
    cleanupFallingSquares();
    squares_animation_active = false;
    
    // Pause le timer de l'horloge
    if (time_update_timer != nullptr) {
        lv_timer_pause(time_update_timer);
    }
    
    // Nettoyer l'écran actuel
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    
    current_screen = STATS_SCREEN;
    stats_screen = scr;
    
    // Container principal
    lv_obj_t* main_container = lv_obj_create(scr);
    lv_obj_set_size(main_container, 470, 262);
    lv_obj_set_style_bg_color(main_container, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_border_width(main_container, 2, 0);
    lv_obj_set_style_border_color(main_container, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(main_container, 5, 0);
    lv_obj_set_style_pad_all(main_container, 8, 0);
    lv_obj_align(main_container, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(main_container, 4, 0);
    
    // Titre avec nom du mineur
    lv_obj_t* title = lv_label_create(main_container);
    char title_text[64];
    snprintf(title_text, sizeof(title_text), "STATS: %s", device->name.c_str());
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF6600), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, LV_PCT(100));
    
    // Obtenir l'historique du mineur
    StatsManager& statsMgr = StatsManager::getInstance();
    MinerHistory* history = statsMgr.getMinerHistory(device->ip);
    
    if (!history || history->hashrateHistory.empty()) {
        // Pas assez de données
        lv_obj_t* no_data = lv_label_create(main_container);
        lv_label_set_text(no_data, "\n\nPas assez de données\npour afficher les statistiques.\n\n"
                                    "Les données sont collectées\ntoutes les 5 minutes.");
        lv_obj_set_style_text_font(no_data, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(no_data, lv_color_hex(0x808080), 0);
        lv_obj_set_style_text_align(no_data, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(no_data, LV_PCT(100));
    } else {
        // Afficher les statistiques de session
        lv_obj_t* stats_container = lv_obj_create(main_container);
        lv_obj_set_size(stats_container, 450, 40);
        lv_obj_set_style_bg_color(stats_container, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_width(stats_container, 1, 0);
        lv_obj_set_style_border_color(stats_container, lv_color_hex(0x444444), 0);
        lv_obj_set_style_radius(stats_container, 3, 0);
        lv_obj_set_style_pad_all(stats_container, 4, 0);
        lv_obj_set_flex_flow(stats_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(stats_container, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
        // Stats: Avg / Min / Max
        lv_obj_t* avg_label = lv_label_create(stats_container);
        char avg_text[32];
        snprintf(avg_text, sizeof(avg_text), "Avg\n%.1f", history->avgHashrate);
        lv_label_set_text(avg_label, avg_text);
        lv_obj_set_style_text_font(avg_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(avg_label, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_align(avg_label, LV_TEXT_ALIGN_CENTER, 0);
        
        lv_obj_t* min_label = lv_label_create(stats_container);
        char min_text[32];
        snprintf(min_text, sizeof(min_text), "Min\n%.1f", history->minHashrate);
        lv_label_set_text(min_label, min_text);
        lv_obj_set_style_text_font(min_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(min_label, lv_color_hex(0xFFAA00), 0);
        lv_obj_set_style_text_align(min_label, LV_TEXT_ALIGN_CENTER, 0);
        
        lv_obj_t* max_label = lv_label_create(stats_container);
        char max_text[32];
        snprintf(max_text, sizeof(max_text), "Max\n%.1f", history->maxHashrate);
        lv_label_set_text(max_label, max_text);
        lv_obj_set_style_text_font(max_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(max_label, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_text_align(max_label, LV_TEXT_ALIGN_CENTER, 0);
        
        // Afficher les alertes actives
        if (history->highTempAlert || history->lowHashrateAlert) {
            lv_obj_t* alert_box = lv_obj_create(main_container);
            lv_obj_set_size(alert_box, 450, 25);
            lv_obj_set_style_bg_color(alert_box, lv_color_hex(0xFF0000), 0);
            lv_obj_set_style_bg_opa(alert_box, LV_OPA_30, 0);
            lv_obj_set_style_border_width(alert_box, 1, 0);
            lv_obj_set_style_border_color(alert_box, lv_color_hex(0xFF0000), 0);
            lv_obj_set_style_radius(alert_box, 3, 0);
            lv_obj_set_style_pad_all(alert_box, 3, 0);
            
            lv_obj_t* alert_text = lv_label_create(alert_box);
            char alert_msg[128];
            if (history->highTempAlert && history->lowHashrateAlert) {
                snprintf(alert_msg, sizeof(alert_msg), LV_SYMBOL_WARNING " Temp élevée + Hashrate faible");
            } else if (history->highTempAlert) {
                snprintf(alert_msg, sizeof(alert_msg), LV_SYMBOL_WARNING " Alerte: Température élevée (>%.0f°C)", 
                         statsMgr.getTempThreshold());
            } else {
                snprintf(alert_msg, sizeof(alert_msg), LV_SYMBOL_WARNING " Alerte: Hashrate faible (<%.0f GH/s)", 
                         statsMgr.getHashrateThreshold());
            }
            lv_label_set_text(alert_text, alert_msg);
            lv_obj_set_style_text_font(alert_text, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(alert_text, lv_color_hex(0xFF0000), 0);
            lv_obj_center(alert_text);
        }
        
        // Afficher l'efficacité si disponible
        if (!history->efficiencyHistory.empty()) {
            // Calculer l'efficacité moyenne
            float avgEfficiency = 0;
            for (const auto& point : history->efficiencyHistory) {
                avgEfficiency += point.value;
            }
            avgEfficiency /= history->efficiencyHistory.size();
            
            lv_obj_t* efficiency_label = lv_label_create(main_container);
            char eff_text[64];
            snprintf(eff_text, sizeof(eff_text), "Efficacité moyenne: %.1f J/TH", avgEfficiency);
            lv_label_set_text(efficiency_label, eff_text);
            lv_obj_set_style_text_font(efficiency_label, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(efficiency_label, lv_color_hex(0x00FFAA), 0);
            lv_obj_set_style_text_align(efficiency_label, LV_TEXT_ALIGN_CENTER, 0);
        }
        
        // Graphique Hashrate
        lv_obj_t* chart_hashrate = lv_chart_create(main_container);
        lv_obj_set_size(chart_hashrate, 450, 60); // Légèrement réduit pour faire de la place
        lv_obj_set_style_bg_color(chart_hashrate, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(chart_hashrate, lv_color_hex(0xFF6600), 0);
        lv_obj_set_style_border_width(chart_hashrate, 1, 0);
        lv_chart_set_type(chart_hashrate, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(chart_hashrate, history->hashrateHistory.size());
        lv_chart_set_div_line_count(chart_hashrate, 3, 5);
        
        // Série pour hashrate
        lv_chart_series_t* ser_hashrate = lv_chart_add_series(chart_hashrate, 
                                                               lv_color_hex(0x00FF00), 
                                                               LV_CHART_AXIS_PRIMARY_Y);
        
        // Remplir les données
        for (size_t i = 0; i < history->hashrateHistory.size(); i++) {
            lv_chart_set_next_value(chart_hashrate, ser_hashrate, (int32_t)history->hashrateHistory[i].value);
        }
        
        // Label pour le graphique hashrate
        lv_obj_t* chart_label1 = lv_label_create(main_container);
        lv_label_set_text(chart_label1, "Hashrate (GH/s) - 24h");
        lv_obj_set_style_text_font(chart_label1, &lv_font_montserrat_10, 0); // Police plus petite
        lv_obj_set_style_text_color(chart_label1, lv_color_hex(0x00FF00), 0);
        
        // Graphique Température
        lv_obj_t* chart_temp = lv_chart_create(main_container);
        lv_obj_set_size(chart_temp, 450, 60); // Légèrement réduit
        lv_obj_set_style_bg_color(chart_temp, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(chart_temp, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_border_width(chart_temp, 1, 0);
        lv_chart_set_type(chart_temp, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(chart_temp, history->temperatureHistory.size());
        lv_chart_set_div_line_count(chart_temp, 3, 5);
        
        // Série pour température
        lv_chart_series_t* ser_temp = lv_chart_add_series(chart_temp, 
                                                           lv_color_hex(0xFF6600), 
                                                           LV_CHART_AXIS_PRIMARY_Y);
        
        // Remplir les données de température
        for (size_t i = 0; i < history->temperatureHistory.size(); i++) {
            lv_chart_set_next_value(chart_temp, ser_temp, (int32_t)history->temperatureHistory[i].value);
        }
        
        // Label pour le graphique température
        lv_obj_t* chart_label2 = lv_label_create(main_container);
        lv_label_set_text(chart_label2, "Température (°C) - 24h");
        lv_obj_set_style_text_font(chart_label2, &lv_font_montserrat_10, 0); // Police plus petite
        lv_obj_set_style_text_color(chart_label2, lv_color_hex(0xFF6600), 0);
    }
    
    // Bouton Back en bas
    lv_obj_t* btn_back = lv_button_create(main_container);
    lv_obj_set_size(btn_back, 100, 25);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(btn_back, 3, 0);
    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            Serial.println("[UI] Back from stats - returning to miners screen");
            UI::getInstance().showMinersScreen();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, LV_SYMBOL_LEFT " Retour");
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_14, 0);
    lv_obj_center(label_back);
    
    // Force refresh
    lv_refr_now(NULL);
    
    Serial.println("[UI] Statistics screen created");
}
}

