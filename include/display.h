#pragma once

#include <Arduino.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include "config.h"

class Display {
public:
    static Display& getInstance() {
        static Display instance;
        return instance;
    }

    void init();
    void setBrightness(uint8_t brightness);
    esp_lcd_panel_handle_t getPanelHandle() { return panel_handle; }

private:
    Display() {} // Private constructor
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    void initPanel();
    
    esp_lcd_panel_handle_t panel_handle = nullptr;
    uint8_t current_brightness = 255;
};