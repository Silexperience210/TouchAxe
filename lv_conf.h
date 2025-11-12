#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_CONF_INCLUDE_SIMPLE 1  // Use simple include style for PlatformIO

/* Color depth */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Display resolution */
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 272
#define LV_DISP_DEF_REFR_PERIOD 30

/* Tick source (0 = default) */
#define LV_TICK_CUSTOM 0

/* Software renderer */
#define LV_USE_DRAW_SW 1
#define LV_DRAW_COMPLEX 1
#define LV_DRAW_SW_ROTATE 1
#define LV_USE_DRAW_SW_HELIUM 0  // Helium assembly off for ESP32-S3

/* Monitors */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/* Fonts (enable the ones you use) */
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_16

/* Widgets */
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_ARC 1
#define LV_USE_DROPDOWN 1

#endif /*LV_CONF_H*/