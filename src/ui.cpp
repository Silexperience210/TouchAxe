#include "ui.h"
#include <Arduino.h>
#include "blackletter_48.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "bitaxe_api.h"
#include "bitcoin_api.h"

// Variables globales pour les écrans
static lv_obj_t *welcome_screen = nullptr;
static lv_obj_t *clock_screen = nullptr;
static lv_obj_t *miners_screen = nullptr;
static lv_obj_t *dashboard_screen = nullptr;  // DEPRECATED
static lv_obj_t *time_label = nullptr;
static lv_obj_t *date_label = nullptr;
static lv_obj_t *hashrate_total_label = nullptr;
static lv_obj_t *hashrate_sum_label = nullptr;  // Somme des hashrates en GH/s
static lv_obj_t *bitcoin_price_label = nullptr;  // Prix du Bitcoin en USD
static lv_obj_t *sats_conversion_label = nullptr;  // Conversion 1$ en Sats
static lv_obj_t *bitaxe_container = nullptr;
static lv_obj_t *btn_scroll_up = nullptr;  // Bouton scroll UP sur Miners screen
static lv_obj_t *btn_scroll_down = nullptr;  // Bouton scroll DOWN sur Miners screen

// Index pour la pagination des mineurs (afficher 2 mineurs à la fois)
static int miners_display_offset = 0;

// Flag pour différer le changement d'écran (évite crash pendant event callbacks)
static bool pending_screen_change = false;
static int pending_screen_target = 0;  // 0=none, 1=CLOCK, 2=MINERS
static lv_obj_t *wifi_status_label = nullptr;  // Pour le welcome screen
static lv_timer_t *time_update_timer = nullptr;  // Timer global pour l'heure
static lv_timer_t *miners_refresh_timer = nullptr;  // Timer auto-refresh pour l'écran Miners

// État actuel de l'écran
enum ScreenState { WELCOME_SCREEN, CLOCK_SCREEN, MINERS_SCREEN, DASHBOARD_SCREEN };
static ScreenState current_screen = WELCOME_SCREEN;

// Fonction pour rafraîchir les stats Bitaxe
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
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(msg, lv_color_hex(0x808080), 0);
        lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(msg);
    } else {
        // Afficher seulement 2 mineurs à la fois (pagination)
        int startIdx = miners_display_offset;
        int endIdx = min(startIdx + 2, bitaxeCount);
        
        Serial.printf("[UI] Displaying miners %d-%d of %d (offset=%d)\n", 
                      startIdx+1, endIdx, bitaxeCount, miners_display_offset);
        
        for (int i = startIdx; i < endIdx; i++) {
            BitaxeDevice* device = wifi->getBitaxe(i);
            if (device == nullptr) continue;
            
            // Créer une carte compacte pour ce Bitaxe
            lv_obj_t* card = lv_obj_create(bitaxe_container);
            lv_obj_set_size(card, 440, 80);
            lv_obj_set_style_bg_color(card, lv_color_hex(0x1a1a1a), 0);
            lv_obj_set_style_border_color(card, lv_color_hex(0xFF0000), 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_radius(card, 5, 0);
            lv_obj_set_style_pad_all(card, 8, 0);
            
            // Nom du device
            lv_obj_t* name_label = lv_label_create(card);
            lv_label_set_text(name_label, device->name.c_str());
            lv_obj_set_style_text_font(name_label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(name_label, lv_color_hex(0xFF0000), 0);
            lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);
            
            // Récupérer les stats via API (si WiFi connecté)
            if (wifi->isConnected()) {
                BitaxeAPI api;
                api.setDevice(device->ip);
                
                BitaxeStats stats;
                if (api.getStats(stats)) {
                    device->online = true;
                    
                    // Ligne 1: Hashrate + Temp
                    lv_obj_t* stats_line1 = lv_label_create(card);
                    char line1[64];
                    snprintf(line1, sizeof(line1), "%.1f GH/s | %.1f°C", stats.hashrate, stats.temp);
                    lv_label_set_text(stats_line1, line1);
                    lv_obj_set_style_text_font(stats_line1, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(stats_line1, lv_color_hex(0x00FF00), 0);
                    lv_obj_align(stats_line1, LV_ALIGN_TOP_LEFT, 0, 22);
                    
                    // Ligne 2: Power seulement (pas de voltage dans BitaxeStats)
                    lv_obj_t* stats_line2 = lv_label_create(card);
                    char line2[64];
                    snprintf(line2, sizeof(line2), "%.1fW", stats.power);
                    lv_label_set_text(stats_line2, line2);
                    lv_obj_set_style_text_font(stats_line2, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(stats_line2, lv_color_hex(0x00AAFF), 0);
                    lv_obj_align(stats_line2, LV_ALIGN_TOP_LEFT, 0, 44);
                    
                    // Status online
                    lv_obj_t* status_icon = lv_label_create(card);
                    lv_label_set_text(status_icon, LV_SYMBOL_OK);
                    lv_obj_set_style_text_font(status_icon, &lv_font_montserrat_28, 0);
                    lv_obj_set_style_text_color(status_icon, lv_color_hex(0x00FF00), 0);
                    lv_obj_align(status_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
                } else {
                    device->online = false;
                    
                    // Offline status
                    lv_obj_t* offline_label = lv_label_create(card);
                    lv_label_set_text(offline_label, "OFFLINE");
                    lv_obj_set_style_text_font(offline_label, &lv_font_montserrat_16, 0);
                    lv_obj_set_style_text_color(offline_label, lv_color_hex(0xFF0000), 0);
                    lv_obj_align(offline_label, LV_ALIGN_CENTER, 0, 0);
                    
                    lv_obj_t* ip_label = lv_label_create(card);
                    lv_label_set_text(ip_label, device->ip.c_str());
                    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_14, 0);
                    lv_obj_set_style_text_color(ip_label, lv_color_hex(0x666666), 0);
                    lv_obj_align(ip_label, LV_ALIGN_BOTTOM_MID, 0, 0);
                }
            } else {
                // WiFi non connecté
                lv_obj_t* waiting_label = lv_label_create(card);
                lv_label_set_text(waiting_label, "Waiting WiFi...");
                lv_obj_set_style_text_font(waiting_label, &lv_font_montserrat_14, 0);
                lv_obj_set_style_text_color(waiting_label, lv_color_hex(0x888888), 0);
                lv_obj_align(waiting_label, LV_ALIGN_CENTER, 0, 0);
            }
        }
    }
    
    // Remettre les boutons scroll au premier plan s'ils existent
    if (btn_scroll_up != nullptr) {
        lv_obj_move_foreground(btn_scroll_up);
    }
    if (btn_scroll_down != nullptr) {
        lv_obj_move_foreground(btn_scroll_down);
    }
    
    // Force immediate refresh after loading data
    lv_refr_now(NULL);
    Serial.println("[UI] Bitaxe stats refreshed");
}

// Timer callback pour auto-refresh de l'écran Miners (toutes les 20s)
static void miners_refresh_cb(lv_timer_t * timer) {
    if (current_screen == MINERS_SCREEN) {
        Serial.println("[UI TIMER] Auto-refreshing miners screen...");
        refreshBitaxeStats();
    }
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

// Callback d'animation pour la pulsation du titre
static void anim_opa_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
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
    
    welcome_screen = scr;
    
    // Forcer le rafraîchissement de l'écran
    lv_obj_invalidate(scr);
    lv_refr_now(NULL);
    
    current_screen = WELCOME_SCREEN;
    
    Serial.println("[UI] Welcome screen created");
}

// Variables pour détecter le clic
static int32_t touch_start_x = 0;
static uint32_t last_touch_time = 0;

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
        last_touch_time = now;
        Serial.printf("[TOUCH] Pressed at X=%d, current_screen=%d\n", point.x, current_screen);
    }
    else if (code == LV_EVENT_RELEASED) {
        // Clic court : changer d'écran selon la moitié gauche/droite
        int32_t dx = abs(point.x - touch_start_x);
        uint32_t duration = now - last_touch_time;
        
        Serial.printf("[TOUCH] Released at X=%d, dx=%d, duration=%dms, current_screen=%d\n", 
                      point.x, dx, duration, current_screen);
        
        // Clic rapide sans mouvement (<30px, <500ms)
        if (dx < 30 && duration < 500) {
            if (point.x < 240 && current_screen == MINERS_SCREEN) {
                // Clic gauche sur écran Miners → retour Clock
                Serial.println("[UI] *** CLICK LEFT - SCHEDULING CLOCK ***");
                pending_screen_change = true;
                pending_screen_target = 1;  // CLOCK
            }
            else if (point.x > 240 && current_screen == CLOCK_SCREEN) {
                // Clic droit sur écran Clock → afficher Miners
                Serial.println("[UI] *** CLICK RIGHT - SCHEDULING MINERS ***");
                pending_screen_change = true;
                pending_screen_target = 2;  // MINERS
            } else {
                Serial.printf("[TOUCH] No action: X=%d, screen=%d\n", point.x, current_screen);
            }
        } else {
            Serial.printf("[TOUCH] Ignored: dx=%d, duration=%d\n", dx, duration);
        }
    }
}

void UI::showClockScreen() {
    Serial.println("[UI] Showing clock screen...");
    
    // Nettoyer l'écran actuel
    lv_obj_t* scr = lv_screen_active();
    lv_obj_clean(scr);
    
    // Réinitialiser les pointeurs
    time_label = nullptr;
    date_label = nullptr;
    bitcoin_price_label = nullptr;
    sats_conversion_label = nullptr;
    hashrate_total_label = nullptr;
    hashrate_sum_label = nullptr;
    bitaxe_container = nullptr;
    btn_scroll_up = nullptr;
    btn_scroll_down = nullptr;
    
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);  // Fond noir
    
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
    
    // Prix Bitcoin en haut à droite - Design futuriste
    bitcoin_price_label = lv_label_create(scr);
    lv_label_set_text(bitcoin_price_label, "BTC\n$--,---");
    lv_obj_set_style_text_font(bitcoin_price_label, &lv_font_montserrat_14, 0);
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
    lv_obj_set_style_text_font(sats_conversion_label, &lv_font_montserrat_14, 0);
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
    lv_obj_set_style_text_font(hashrate_total_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hashrate_total_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(hashrate_total_label, LV_ALIGN_CENTER, 0, 85);
    
    // Indicateur de swipe en bas
    lv_obj_t* swipe_hint = lv_label_create(scr);
    lv_label_set_text(swipe_hint, LV_SYMBOL_LEFT " Click for miners details");
    lv_obj_set_style_text_font(swipe_hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(swipe_hint, lv_color_hex(0x404040), 0);
    lv_obj_align(swipe_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Détecter les clics pour navigation
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(scr, screen_touch_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);  // Nécessaire pour recevoir les events touch
    
    Serial.printf("[UI] Time/date labels created: time_label=%p, date_label=%p\n", time_label, date_label);
    
    clock_screen = scr;
    
    // IMPORTANT: Définir current_screen AVANT d'appeler le callback du timer
    current_screen = CLOCK_SCREEN;
    
    // Créer ou reprendre le timer LVGL pour mettre à jour l'horloge toutes les secondes
    if (time_update_timer == nullptr) {
        Serial.println("[UI] Creating time update timer (1s interval)");
        time_update_timer = lv_timer_create(update_time_cb, 1000, NULL);  // Toutes les secondes
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
    
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    
    // Titre
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "MINEURS BITAXE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF0000), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Container scrollable pour les mineurs
    bitaxe_container = lv_obj_create(scr);
    lv_obj_set_size(bitaxe_container, 460, 190);
    lv_obj_align(bitaxe_container, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_bg_color(bitaxe_container, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_border_width(bitaxe_container, 0, 0);
    lv_obj_set_flex_flow(bitaxe_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(bitaxe_container, 5, 0);
    lv_obj_set_style_pad_row(bitaxe_container, 5, 0);
    
    // Activer le scroll vertical
    lv_obj_set_scrollbar_mode(bitaxe_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(bitaxe_container, LV_DIR_VER);  // Vertical seulement
    lv_obj_add_flag(bitaxe_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bitaxe_container, LV_OBJ_FLAG_SCROLL_MOMENTUM);  // Désactiver momentum pour meilleure réactivité
    
    // Afficher message de chargement temporaire
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
    
    // Bouton UP (scroll vers le haut) - côté droit en haut
    btn_scroll_up = lv_button_create(scr);
    lv_obj_set_size(btn_scroll_up, 35, 35);
    lv_obj_align(btn_scroll_up, LV_ALIGN_RIGHT_MID, -10, -50);
    lv_obj_set_style_bg_color(btn_scroll_up, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(btn_scroll_up, 18, 0);  // Arrondi
    
    // Ajouter un halo lumineux pulsant
    lv_obj_set_style_shadow_width(btn_scroll_up, 15, 0);
    lv_obj_set_style_shadow_color(btn_scroll_up, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_shadow_opa(btn_scroll_up, LV_OPA_80, 0);
    lv_obj_set_style_shadow_spread(btn_scroll_up, 3, 0);
    
    lv_obj_add_flag(btn_scroll_up, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn_scroll_up, LV_OBJ_FLAG_SCROLLABLE);  // Désactiver scroll sur le bouton
    lv_obj_add_event_cb(btn_scroll_up, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            Serial.println("[UI] UP - showing previous miners");
            WifiManager* wifi = WifiManager::getInstance();
            int bitaxeCount = wifi->getBitaxeCount();
            
            if (miners_display_offset > 0) {
                miners_display_offset--;
                Serial.printf("[UI] Page: %d/%d\n", miners_display_offset + 1, (bitaxeCount + 1) / 2);
                refreshBitaxeStats();
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label_up = lv_label_create(btn_scroll_up);
    lv_label_set_text(label_up, LV_SYMBOL_UP);
    lv_obj_set_style_text_font(label_up, &lv_font_montserrat_16, 0);
    lv_obj_center(label_up);
    
    // Bouton DOWN (scroll vers le bas) - côté droit en bas
    btn_scroll_down = lv_button_create(scr);
    lv_obj_set_size(btn_scroll_down, 35, 35);
    lv_obj_align(btn_scroll_down, LV_ALIGN_RIGHT_MID, -10, 50);
    lv_obj_set_style_bg_color(btn_scroll_down, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(btn_scroll_down, 18, 0);  // Arrondi
    
    // Ajouter un halo lumineux pulsant
    lv_obj_set_style_shadow_width(btn_scroll_down, 15, 0);
    lv_obj_set_style_shadow_color(btn_scroll_down, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_shadow_opa(btn_scroll_down, LV_OPA_80, 0);
    lv_obj_set_style_shadow_spread(btn_scroll_down, 3, 0);
    
    lv_obj_add_flag(btn_scroll_down, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn_scroll_down, LV_OBJ_FLAG_SCROLLABLE);  // Désactiver scroll sur le bouton
    lv_obj_add_event_cb(btn_scroll_down, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            Serial.println("[UI] DOWN - showing next miners");
            WifiManager* wifi = WifiManager::getInstance();
            int bitaxeCount = wifi->getBitaxeCount();
            
            if (miners_display_offset + 2 < bitaxeCount) {
                miners_display_offset++;
                Serial.printf("[UI] Page: %d/%d\n", miners_display_offset + 1, (bitaxeCount + 1) / 2);
                refreshBitaxeStats();
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label_down = lv_label_create(btn_scroll_down);
    lv_label_set_text(label_down, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(label_down, &lv_font_montserrat_16, 0);
    lv_obj_center(label_down);
    
    // Bouton Back centré en bas (refresh automatique toutes les 10s)
    lv_obj_t* btn_back = lv_button_create(scr);
    lv_obj_set_size(btn_back, 140, 40);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);  // Centré en bas
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x333333), 0);
    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
        if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
            Serial.println("[UI] Back - returning to clock");
            miners_display_offset = 0;  // Reset offset
            UI::getInstance().showClockScreen();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, LV_SYMBOL_LEFT " Back");
    lv_obj_center(label_back);
    
    // Ensure buttons are on top of all other elements
    lv_obj_move_foreground(btn_scroll_up);
    lv_obj_move_foreground(btn_scroll_down);
    lv_obj_move_foreground(btn_back);
    
    // Animation de pulsation pour le halo des boutons UP/DOWN
    lv_anim_t anim_up, anim_down;
    lv_anim_init(&anim_up);
    lv_anim_set_var(&anim_up, btn_scroll_up);
    lv_anim_set_values(&anim_up, LV_OPA_30, LV_OPA_80);
    lv_anim_set_time(&anim_up, 1500);  // 1.5 secondes
    lv_anim_set_playback_time(&anim_up, 1500);
    lv_anim_set_repeat_count(&anim_up, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_up, [](void * obj, int32_t value) {
        lv_obj_set_style_shadow_opa((lv_obj_t*)obj, value, 0);
    });
    lv_anim_start(&anim_up);
    
    lv_anim_init(&anim_down);
    lv_anim_set_var(&anim_down, btn_scroll_down);
    lv_anim_set_values(&anim_down, LV_OPA_30, LV_OPA_80);
    lv_anim_set_time(&anim_down, 1500);  // 1.5 secondes
    lv_anim_set_playback_time(&anim_down, 1500);
    lv_anim_set_repeat_count(&anim_down, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&anim_down, [](void * obj, int32_t value) {
        lv_obj_set_style_shadow_opa((lv_obj_t*)obj, value, 0);
    });
    lv_anim_start(&anim_down);
    
    Serial.println("[UI] Scroll buttons and Back button created with pulsing glow");
    
    // SUPPRIMÉ: Ne pas ajouter screen_touch_cb sur Miners screen (interfère avec les boutons)
    // Le bouton Back gère le retour à l'écran horloge
    // Auto-refresh géré par millis() timer dans main.cpp (10s)
    
    // Note: miners_screen et current_screen déjà définis plus haut avant refreshBitaxeStats()
    
    Serial.println("[UI] Miners screen created (auto-refresh every 10s via main loop)");
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
    
    lv_obj_invalidate(bitcoin_price_label);
    lv_obj_invalidate(sats_conversion_label);
    lv_refr_now(NULL);  // Force immediate redraw
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
            Serial.printf("[UI]   [%d] %s - ONLINE (%.1f GH/s, %.1f°C)\n", 
                i, device->name.c_str(), stats.hashrate, stats.temp);
        } else {
            Serial.printf("[UI]   [%d] %s - OFFLINE (%s)\n", 
                i, device->name.c_str(), device->ip.c_str());
        }
    }
    
    Serial.printf("[UI] Total: %d online, %.1f GH/s\n", onlineCount, totalHashrate);
    
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
    }
}

// Refresh Miners screen data if currently on that screen (called from main loop every 20s)
void UI::refreshMinersIfActive() {
    // Only refresh if we're on the Miners screen
    if (current_screen != MINERS_SCREEN) {
        return;
    }
    
    // Skip if in AP mode
    if (WifiManager::getInstance()->isAPMode()) {
        return;
    }
    
    // Skip if not connected to WiFi
    if (!WifiManager::getInstance()->isConnected()) {
        return;
    }
    
    Serial.println("[UI] Auto-refreshing Miners screen data (10s timer)...");
    refreshBitaxeStats();
}

