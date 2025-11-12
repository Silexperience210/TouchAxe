#include <Arduino.h>
#include "lvgl.h"
#include "display_config.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include <Wire.h>
#include <cstring>
#include "TAMC_GT911.h"
#include "ui.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "bitcoin_api.h"
#include "config.h"

static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_display_t *disp = NULL;
static lv_color_t *draw_buf1 = NULL;
static lv_color_t *draw_buf2 = NULL;

// Touch configuration (Sunton ESP32-S3 mapping pour GT911 capacitif)
#define GT911_SDA        19
#define GT911_SCL        20
#define GT911_INT        18
#define GT911_RST        38

// Déclaration du pointeur touch (sera initialisé dans setup())
TAMC_GT911 *touch = nullptr;

// Pointeur vers le périphérique d'entrée LVGL
static lv_indev_t *indev_touchpad = NULL;

// Mutex pour LVGL thread safety
static SemaphoreHandle_t lvgl_mutex = xSemaphoreCreateMutex();

// LVGL v9 flush callback: called when LVGL has rendered a tile. We forward the pixel buffer
// to the ESP LCD driver and notify LVGL that flushing is ready.
static void lvgl_flush_cb(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    (void) disp_drv;
    
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    // IMPORTANT: En LVGL v9, px_map est déjà le pointeur direct vers les pixels RGB565
    // Il faut le caster en uint16_t* pour esp_lcd_panel_draw_bitmap
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, (uint16_t*)px_map);
    
    // Notify LVGL that flushing is finished
    lv_display_flush_ready(disp_drv);
}

// LVGL v9 touchpad read callback: called by LVGL to read touch coordinates
static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void) indev;
    
    static uint32_t last_read_time = 0;
    static lv_indev_data_t last_data;
    uint32_t now = millis();
    
    // Limiter la lecture du GT911 à 50Hz (20ms) pour un suivi plus fluide du mouvement
    if (now - last_read_time < 20) {
        // Retourner le dernier état lu
        *data = last_data;
        return;
    }
    last_read_time = now;
    
    if (touch != nullptr) {
        touch->read();
        if (touch->isTouched && touch->touches > 0) {
            // Retourner les coordonnées du premier point tactile
            // INVERSION X et Y: l'écran est monté à l'envers (rotation 180°)
            data->point.x = LCD_H_RES - touch->points[0].x;  // Inverser l'axe X
            data->point.y = LCD_V_RES - touch->points[0].y;  // Inverser l'axe Y
            data->state = LV_INDEV_STATE_PRESSED;
            Serial.printf("[Touch] X=%d Y=%d\n", data->point.x, data->point.y);  // Réactivé pour debug
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
        
        // Sauvegarder pour le prochain appel rapproché
        last_data = *data;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Attendre que le port USB CDC soit prêt
    
    // Force flush to ensure USB CDC is working
    Serial.println("\n\n\n\n\n");
    Serial.flush();
    delay(100);
    
    Serial.println("=== TouchAxe Starting ===");
    Serial.println("Starting display initialization...");
    Serial.flush();
    
    // Initialize I2C for touch
    Serial.println("Initializing I2C...");
    Serial.flush();
    Wire.begin(GT911_SDA, GT911_SCL);
    Serial.println("I2C initialized");
    Serial.flush();
    
    Serial.println("Initializing touch controller...");
    Serial.flush();
    touch = new TAMC_GT911(GT911_SDA, GT911_SCL, GT911_INT, GT911_RST, LCD_H_RES, LCD_V_RES);
    touch->begin();
    Serial.println("Touch controller initialized");
    Serial.flush();

    // RGB display configuration (RGB565 / 16-bit)
    Serial.println("Configuring RGB panel structure...");
    esp_lcd_rgb_panel_config_t panel_config;
    memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));
    
    Serial.println("Setting clock source and timings...");
    panel_config.clk_src = LCD_CLK_SRC_PLL160M;
    panel_config.timings.pclk_hz = LCD_FREQUENCY;
    panel_config.timings.h_res = LCD_H_RES;
    panel_config.timings.v_res = LCD_V_RES;
    panel_config.timings.hsync_pulse_width = LCD_HSYNC_PULSE;
    panel_config.timings.hsync_back_porch = LCD_HSYNC_BACK;
    panel_config.timings.hsync_front_porch = LCD_HSYNC_FRONT;
    panel_config.timings.vsync_pulse_width = LCD_VSYNC_PULSE;
    panel_config.timings.vsync_back_porch = LCD_VSYNC_BACK;
    panel_config.timings.vsync_front_porch = LCD_VSYNC_FRONT;
    panel_config.timings.flags.hsync_idle_low = 1;
    panel_config.timings.flags.vsync_idle_low = 1;
    panel_config.timings.flags.de_idle_high = 0;
    panel_config.timings.flags.pclk_active_neg = 1;
    panel_config.timings.flags.pclk_idle_high = 1;
    
    Serial.println("Setting data width and GPIO pins...");
    panel_config.data_width = 16; // RGB565
    panel_config.psram_trans_align = 64;
    panel_config.hsync_gpio_num = (gpio_num_t)LCD_PIN_HSYNC;
    panel_config.vsync_gpio_num = (gpio_num_t)LCD_PIN_VSYNC;
    panel_config.de_gpio_num = GPIO_NUM_NC;
    panel_config.pclk_gpio_num = (gpio_num_t)LCD_PIN_PCLK;
    panel_config.disp_gpio_num = GPIO_NUM_NC;
    
    Serial.println("Setting data GPIO numbers...");
    // Data GPIO numbers
    panel_config.data_gpio_nums[0] = (gpio_num_t)LCD_PIN_DATA0;
    panel_config.data_gpio_nums[1] = (gpio_num_t)LCD_PIN_DATA1;
    panel_config.data_gpio_nums[2] = (gpio_num_t)LCD_PIN_DATA2;
    panel_config.data_gpio_nums[3] = (gpio_num_t)LCD_PIN_DATA3;
    panel_config.data_gpio_nums[4] = (gpio_num_t)LCD_PIN_DATA4;
    panel_config.data_gpio_nums[5] = (gpio_num_t)LCD_PIN_DATA5;
    panel_config.data_gpio_nums[6] = (gpio_num_t)LCD_PIN_DATA6;
    panel_config.data_gpio_nums[7] = (gpio_num_t)LCD_PIN_DATA7;
    panel_config.data_gpio_nums[8] = (gpio_num_t)LCD_PIN_DATA8;
    panel_config.data_gpio_nums[9] = (gpio_num_t)LCD_PIN_DATA9;
    panel_config.data_gpio_nums[10] = (gpio_num_t)LCD_PIN_DATA10;
    panel_config.data_gpio_nums[11] = (gpio_num_t)LCD_PIN_DATA11;
    panel_config.data_gpio_nums[12] = (gpio_num_t)LCD_PIN_DATA12;
    panel_config.data_gpio_nums[13] = (gpio_num_t)LCD_PIN_DATA13;
    panel_config.data_gpio_nums[14] = (gpio_num_t)LCD_PIN_DATA14;
    panel_config.data_gpio_nums[15] = (gpio_num_t)LCD_PIN_DATA15;
    
    panel_config.flags.fb_in_psram = 1;
    panel_config.flags.relax_on_idle = 0;  // keep panel refreshing continuously
    
    // Initialize LCD panel
    Serial.println("Creating RGB panel...");
    esp_err_t ret = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to create RGB panel: 0x%x\n", ret);
        while (1) delay(1000);
    }
    Serial.println("RGB panel created");
    
    Serial.println("Resetting panel...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    
    Serial.println("Initializing panel...");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Some panel drivers do not implement disp_on_off; log but keep running instead of aborting.
    esp_err_t disp_err = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (disp_err == ESP_ERR_NOT_SUPPORTED) {
        Serial.println("Panel driver does not support disp_on_off, continuing");
    } else if (disp_err != ESP_OK) {
        Serial.printf("WARNING: Failed to turn panel on (0x%x)\n", disp_err);
    }
    Serial.println("RGB panel initialized");
  
    // Turn on LCD backlight
    Serial.printf("Turning on backlight (GPIO %d, level %d)...\n", PIN_LCD_BL, 1);
    if (PIN_LCD_BL >= 0) {
        pinMode(PIN_LCD_BL, OUTPUT);
        digitalWrite(PIN_LCD_BL, 1);
        Serial.println("Backlight ON");
    } else {
        Serial.println("No backlight pin configured");
    }

    // Initialize LVGL
    Serial.println("Initializing LVGL...");
    lv_init();
    Serial.println("LVGL initialized");

    // Create LVGL display and draw buffer (LVGL v9 API)
    Serial.printf("Creating LVGL display (%dx%d)...\n", LCD_H_RES, LCD_V_RES);
    disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (!disp) {
        Serial.println("ERROR: Failed to create LVGL display object");
        while (1) delay(1000);
    }
    Serial.println("LVGL display created");
    lv_display_set_default(disp);

    // Create LVGL draw buffers (40-line stripes) in PSRAM for partial rendering
    size_t buf_line_count = 40;
    size_t buf_size_bytes = LCD_H_RES * buf_line_count * sizeof(lv_color_t);
    Serial.printf("Allocating LVGL draw buffers (%u lines, %u bytes each)...\n", (unsigned)buf_line_count, (unsigned)buf_size_bytes);

    draw_buf1 = (lv_color_t *)heap_caps_malloc(buf_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    draw_buf2 = (lv_color_t *)heap_caps_malloc(buf_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!draw_buf1) {
        Serial.println("CRITICAL ERROR: Failed to allocate LVGL buffer 1");
        while (1) delay(1000);
    }

    if (!draw_buf2) {
        Serial.println("WARNING: Failed to allocate LVGL buffer 2, falling back to single buffer");
    }

    lv_display_set_buffers(disp, draw_buf1, draw_buf2, buf_size_bytes,
                           draw_buf2 ? LV_DISPLAY_RENDER_MODE_PARTIAL : LV_DISPLAY_RENDER_MODE_PARTIAL);  // partial even with single buffer
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    Serial.printf("LVGL buffers ready (%s buffering)\n", draw_buf2 ? "double" : "single");

    // Créer le périphérique d'entrée tactile LVGL (DOIT être fait AVANT de créer les widgets)
    Serial.println("Creating LVGL touchpad input device...");
    indev_touchpad = lv_indev_create();
    if (!indev_touchpad) {
        Serial.println("ERROR: Failed to create LVGL input device");
        while (1) delay(1000);
    }
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read_cb);
    lv_indev_set_display(indev_touchpad, disp);  // IMPORTANT: Associer l'input au display !
    Serial.println("Touchpad input device created");
    Serial.flush();

    // Initialiser l'interface utilisateur
    Serial.println("Initializing UI...");
    UI& ui = UI::getInstance();
    ui.init();
    
    // Initialiser le WiFi Manager (gère AP mode et web portal)
    Serial.println("Initializing WiFi Manager...");
    WifiManager::getInstance()->init();
    
    // Enregistrer le callback pour transition auto vers Clock quand WiFi connecté
    WifiManager::getInstance()->setWifiConnectedCallback([]() {
        Serial.println("[main] WiFi connected callback - showing clock screen");
        UI::getInstance().showClockScreen();
    });
    
    // Initialiser le TimeManager (NTP) - WiFiManager s'occupera de la connexion
    Serial.println("Initializing TimeManager...");
    TimeManager::getInstance()->init();
    
    // Initialiser le BitcoinAPI
    Serial.println("Initializing BitcoinAPI...");
    BitcoinAPI::getInstance().begin();
    
    Serial.println("\n=== SETUP COMPLETE ===\n");
}

void loop() {
    // Check for serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "status") {
            Serial.println("\n=== TouchAxe Status ===");
            Serial.printf("WiFi Mode: %s\n", WifiManager::getInstance()->isAPMode() ? "AP Mode" : "Station Mode");
            Serial.printf("WiFi SSID: %s\n", WifiManager::getInstance()->getSSID().c_str());
            Serial.printf("IP Address: %s\n", WifiManager::getInstance()->getIP().c_str());
            Serial.printf("Web Server: %s\n", WifiManager::getInstance()->isWebServerRunning() ? "Running" : "Stopped");
            WifiManager::getInstance()->printBitaxeConfig();
        }
        else if (cmd == "clear") {
            WifiManager::getInstance()->clearAllBitaxes();
        }
        else if (cmd == "reset") {
            Serial.println("Resetting WiFi config...");
            WifiManager::getInstance()->resetConfig();
        }
        else if (cmd == "webstop") {
            WifiManager::getInstance()->stopWebServer();
        }
        else if (cmd == "webstart") {
            WifiManager::getInstance()->startWebServer();
        }
        else if (cmd == "help") {
            Serial.println("\n=== TouchAxe Serial Commands ===");
            Serial.println("status   - Show WiFi and Bitaxe status");
            Serial.println("clear    - Clear all Bitaxe devices");
            Serial.println("reset    - Reset WiFi config and restart in AP mode");
            Serial.println("webstop  - Stop web server to save CPU/RAM");
            Serial.println("webstart - Start web server");
            Serial.println("help     - Show this help message");
            Serial.println("================================\n");
        }
        else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
    
    // Update WiFi Manager
    WifiManager::getInstance()->update();
    
    // Update TimeManager
    TimeManager::getInstance()->update();
    
    // Simple timer for clock update (bypass LVGL timer issues)
    static uint32_t last_clock_update = 0;
    if (millis() - last_clock_update > 1000) {
        last_clock_update = millis();
        UI::getInstance().updateClock();
    }
    
    // ANIMATION MANUELLE des carrés - SEULEMENT sur Clock screen !
    // Désactivée automatiquement sur Miners pour meilleures performances
    UI::getInstance().updateFallingSquares();
    
    // Check Bitaxe status every 15 seconds - SEULEMENT sur Clock screen !
    // (skip sur Miners pour éviter conflits HTTP)
    static uint32_t last_bitaxe_check = 0;
    if (!WifiManager::getInstance()->isAPMode() && millis() - last_bitaxe_check > 15000) {
        last_bitaxe_check = millis();
        UI::getInstance().checkBitaxeStatus();  // Cette fonction vérifie déjà current_screen
    }
    
    // Refresh Miners screen data every 10 seconds (only when on MINERS_SCREEN)
    static uint32_t last_miners_refresh = 0;
    if (!WifiManager::getInstance()->isAPMode() && millis() - last_miners_refresh > 10000) {
        last_miners_refresh = millis();
        UI::getInstance().refreshMinersIfActive();
    }
    
    // Fetch Bitcoin price every 30 seconds - CoinGecko free tier allows 10-50 req/min
    static uint32_t last_bitcoin_fetch = 0;
    static bool bitcoin_first_fetch = true;
    if (!WifiManager::getInstance()->isAPMode()) {
        // First fetch immediately after WiFi connection
        if (bitcoin_first_fetch && WifiManager::getInstance()->isConnected()) {
            bitcoin_first_fetch = false;
            last_bitcoin_fetch = millis();
            BitcoinAPI::getInstance().fetchPrice();
            UI::getInstance().updateBitcoinPrice();
        }
        // Then fetch every 30 seconds
        else if (millis() - last_bitcoin_fetch > 30000) {
            last_bitcoin_fetch = millis();
            BitcoinAPI::getInstance().fetchPrice();
            UI::getInstance().updateBitcoinPrice();
        }
    }
    
    // *** OPTIMIZED: Call lv_timer_handler() every 10ms instead of every millisecond ***
    // LVGL recommends 5-10ms intervals for smooth operation with much better CPU efficiency
    static uint32_t last_lvgl_call = 0;
    uint32_t now = millis();
    if (now - last_lvgl_call >= 10) {
        last_lvgl_call = now;
        lv_timer_handler();
    }
    
    // Touch input lu à CHAQUE loop pour maximum de réactivité (<1ms latence)
    if (indev_touchpad != nullptr) {
        lv_indev_read(indev_touchpad);
    }
    
    // Animation update appelée à chaque loop pour fluidité
    UI::getInstance().updateFallingSquares();
    
    // NO DELAY - ESP32-S3 @ 240MHz handles this easily
}

