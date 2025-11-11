#include "display.h"
#include <driver/gpio.h>

void Display::init() {
    // Initialize backlight pin
    pinMode(PIN_LCD_BL, OUTPUT);
    setBrightness(current_brightness);
    
    initPanel();
}

void Display::setBrightness(uint8_t brightness) {
    current_brightness = brightness;
    analogWrite(PIN_LCD_BL, brightness);
}

void Display::initPanel() {
    esp_lcd_rgb_panel_config_t panel_config = {};
    
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
    
    panel_config.flags.fb_in_psram = true;
    panel_config.flags.relax_on_idle = false;  // Désactiver pour forcer le refresh
    panel_config.timings.flags.hsync_idle_low = false;
    panel_config.timings.flags.vsync_idle_low = false;
    panel_config.timings.flags.de_idle_high = true;
    panel_config.timings.flags.pclk_active_neg = false;
    panel_config.timings.flags.pclk_idle_high = true;
    
    panel_config.data_width = 16;
    panel_config.sram_trans_align = 64;
    panel_config.psram_trans_align = 64;
    
    panel_config.hsync_gpio_num = (gpio_num_t)LCD_PIN_HSYNC;
    panel_config.vsync_gpio_num = (gpio_num_t)LCD_PIN_VSYNC;
    panel_config.de_gpio_num = LCD_PIN_DE;
    panel_config.pclk_gpio_num = (gpio_num_t)LCD_PIN_PCLK;
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
    
    panel_config.disp_gpio_num = GPIO_NUM_NC;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
}