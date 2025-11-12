#pragma once

#include <lvgl.h>
#include "config.h"
#include "display.h"

class UI {
public:
    static UI& getInstance() {
        static UI instance;
        return instance;
    }

    void init();
    void update();
    void createTheme();
    void showSplashScreen();
    void showWelcomeScreen();
    void showClockScreen();     // Page principale : horloge + hashrate total
    void showMinersScreen();    // Page détails des mineurs
    void showDashboard();       // DEPRECATED - kept for compatibility
    void showMainMenu();
    void updateClock();         // Update clock display (called from loop)
    void checkBitaxeStatus();   // Check Bitaxe device status periodically
    void refreshMinersIfActive(); // Refresh Miners screen data if currently on that screen
    void updateBitcoinPrice();  // Update Bitcoin price display (called from loop)
    void updateFallingSquares(); // Update falling squares animation (manual, like updateClock)

private:
    UI() {} // Private constructor
    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;

    static void flushDisplay(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
    static void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data);

    lv_display_t *disp = nullptr;
    lv_theme_t *theme = nullptr;
    lv_color_t *buf1 = nullptr;
    lv_color_t *buf2 = nullptr;
    uint32_t last_tick = 0;
};