#pragma once

// Pins configuration
#define PIN_LCD_BL          2   // Backlight control
#define PIN_TOUCH_SDA       11  // Touch I2C SDA
#define PIN_TOUCH_SCL       10  // Touch I2C SCL
#define TOUCH_I2C_ADDR      0x38

// Display configuration
#define LCD_FREQUENCY       10000000  // 10MHz
#define LCD_H_RES          480
#define LCD_V_RES          272

// Display timings
#define LCD_HSYNC_PULSE    8
#define LCD_HSYNC_BACK     32
#define LCD_HSYNC_FRONT    8
#define LCD_VSYNC_PULSE    8
#define LCD_VSYNC_BACK     12
#define LCD_VSYNC_FRONT    8

// GPIO mapping for LCD
#define LCD_PIN_HSYNC      39
#define LCD_PIN_VSYNC      41
#define LCD_PIN_DE         40
#define LCD_PIN_PCLK       42
#define LCD_PIN_DATA0      8   // R0
#define LCD_PIN_DATA1      3   // R1
#define LCD_PIN_DATA2      46  // R2
#define LCD_PIN_DATA3      9   // R3
#define LCD_PIN_DATA4      1   // R4
#define LCD_PIN_DATA5      5   // G0
#define LCD_PIN_DATA6      6   // G1
#define LCD_PIN_DATA7      7   // G2
#define LCD_PIN_DATA8      15  // G3
#define LCD_PIN_DATA9      16  // G4
#define LCD_PIN_DATA10     4   // G5
#define LCD_PIN_DATA11     45  // B0
#define LCD_PIN_DATA12     48  // B1
#define LCD_PIN_DATA13     47  // B2
#define LCD_PIN_DATA14     21  // B3
#define LCD_PIN_DATA15     14  // B4

// UI configuration
#define UI_ANIMATION_SPEED 700  // Animation duration in ms
#define UI_UPDATE_PERIOD   6    // UI update period in ms (165 FPS)

// Theme colors
#define COLOR_PRIMARY      0x1E90FF  // Dodger Blue
#define COLOR_SECONDARY    0x003060  // Dark Blue
#define COLOR_ACCENT      0xFFFFFF  // White