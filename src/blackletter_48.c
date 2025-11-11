/*******************************************************************************
 * Size: 48 px
 * Bpp: 4
 * Opts: Blackletter style font for TouchAxe logo
 ******************************************************************************/

#include "lvgl.h"

#ifndef BLACKLETTER_48
#define BLACKLETTER_48 1
#endif

#if BLACKLETTER_48

/*-----------------
 *    BITMAPS
 *----------------*/

/* U+0020 " " */
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap_0x20[] = {
    /* Empty space */
};

/* U+0041 "A" - Enhanced for blackletter style */
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap_0x41[] = {
    0x00, 0x00, 0x00, 0x07, 0xff, 0x70, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x7f, 0xff, 0xf7, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00,
    0x00, 0x00, 0x07, 0xf7, 0x00, 0x7f, 0x70, 0x00,
    0x00, 0x00, 0x0f, 0xf0, 0x00, 0x0f, 0xf0, 0x00,
    0x00, 0x00, 0x7f, 0x70, 0x00, 0x07, 0xf7, 0x00,
    0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x07, 0xf7, 0x00, 0x00, 0x00, 0x7f, 0x70,
    0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x0f, 0xf0,
    0x00, 0x7f, 0x70, 0x00, 0x00, 0x00, 0x07, 0xf7,
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x07, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
    0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0,
    0x7f, 0x70, 0x00, 0x00, 0x00, 0x00, 0x07, 0xf7,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
};

/* Similar bitmaps for C, E, H, O, T, U, X would go here */
/* For now, using montserrat as fallback */

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 192, .box_w = 12, .box_h = 1, .ofs_x = 0, .ofs_y = 0},  /* space */
    {.bitmap_index = 0, .adv_w = 384, .box_w = 24, .box_h = 48, .ofs_x = 0, .ofs_y = 0}, /* A */
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x21
};

static const lv_font_fmt_txt_cmap_t cmaps[] = {
    {
        .range_start = 32, .range_length = 33, .glyph_id_start = 0,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

static const lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = NULL,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
};

/*-----------------
 *  PUBLIC FONT
 *----------------*/

const lv_font_t blackletter_48 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 48,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -6,
    .underline_thickness = 3,
    .dsc = &font_dsc,
    .fallback = &lv_font_montserrat_48,  /* Fallback important pour les caractères manquants */
};

#endif /*BLACKLETTER_48*/
