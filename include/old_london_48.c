/*******************************************************************************
 * Size: 48 px
 * Bpp: 4
 * Opts: --bpp 4 --size 48 --font OldLondon.ttf --range 0x20-0x7E --format lvgl -o old_london_48.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef OLD_LONDON_48
#define OLD_LONDON_48 1
#endif

#if OLD_LONDON_48

/*-----------------
 *    BITMAPS
 *----------------*/

/* Store the glyphs for the Old London font - simplified version for TouchAxe text */
/* This is a placeholder - we'll use montserrat_48 with custom styling for now */

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

/* Since generating a true Old London font requires the actual TTF file,
   we'll create a workaround using LVGL's text transformation */

const lv_font_t old_london_48 = {
    .get_glyph_dsc = NULL,
    .get_glyph_bitmap = NULL,
    .line_height = 0,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = 0,
    .underline_thickness = 0,
    .dsc = NULL,
    .fallback = &lv_font_montserrat_48,  // Fallback to montserrat_48
};

#endif /*OLD_LONDON_48*/
