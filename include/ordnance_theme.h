#pragma once
/*
 * ordnance_theme.h — TouchAxe design tokens (ORDNANCE system, rev A).
 *
 * Colour, spacing and type live HERE and nowhere else. Before this file the
 * UI carried 14 separate literal 0xFF0000 values and 25 hand-placed offsets;
 * changing the look meant editing 2000 lines and hoping.
 *
 * The law of the system: BONE and GREY carry text, WIRE carries structure,
 * and a signal colour may appear ONLY when it encodes a machine state.
 * Decoration in colour is not permitted.
 *
 * Full rationale: docs/design/ORDNANCE_DESIGN_SYSTEM.md
 */

#include "lvgl.h"
#include "telemetry.h"

/* ── chromatic register ─────────────────────────────────────────────── */
#define OX_VOID    lv_color_hex(0x05070A)   /* 565 0x0021 — canvas / negative */
#define OX_STEEL   lv_color_hex(0x0D1318)   /* 565 0x0883 — panel ground */
#define OX_WIRE    lv_color_hex(0x1E2A32)   /* 565 0x1946 — structure / rules */
#define OX_GREY    lv_color_hex(0x55666F)   /* 565 0x532D — secondary type */
#define OX_BONE    lv_color_hex(0xDCE4E8)   /* 565 0xDF3D — primary type */
#define OX_AMBER   lv_color_hex(0xFFB020)   /* 565 0xFD84 — signal / focus */
#define OX_ORANGE  lv_color_hex(0xF7931A)   /* 565 0xF483 — bitcoin accent */
#define OX_EMBER   lv_color_hex(0xFF6B1A)   /* 565 0xFB43 — thermal load */
#define OX_PHOS    lv_color_hex(0x46E0A0)   /* 565 0x4714 — nominal state */
#define OX_ALERT   lv_color_hex(0xFF3B30)   /* 565 0xF9C6 — fault state */

/* Raw hex, for places that need a uint32_t rather than lv_color_t. */
#define OX_HEX_VOID    0x05070AU
#define OX_HEX_STEEL   0x0D1318U
#define OX_HEX_WIRE    0x1E2A32U
#define OX_HEX_GREY    0x55666FU
#define OX_HEX_BONE    0xDCE4E8U
#define OX_HEX_AMBER   0xFFB020U
#define OX_HEX_ORANGE  0xF7931AU
#define OX_HEX_EMBER   0xFF6B1AU
#define OX_HEX_PHOS    0x46E0A0U
#define OX_HEX_ALERT   0xFF3B30U

/* ── spatial grid — native panel is 480 x 272 ───────────────────────── */
#define OX_UNIT          4    /* every offset is a multiple of this      */
#define OX_GUTTER        6
#define OX_RAIL_H       20    /* status rail height                      */
#define OX_FOOT_H       14    /* footer rail height                      */
#define OX_PANEL_R       0    /* corners are square. always.             */
#define OX_HAIRLINE      1
#define OX_ACCENT_W      2    /* left stripe on every tile — the state   */

/* ── type roles ─────────────────────────────────────────────────────── */
/* Built-in Montserrat for now. To get the condensed grotesque of the
 * design plate, convert BigShoulders-Bold with lv_font_conv and swap
 * OX_F_DISPLAY / OX_F_SUBHEAD below — nothing else changes. */
#define OX_F_DISPLAY    &lv_font_montserrat_48   /* one value per screen */
#define OX_F_SUBHEAD    &lv_font_montserrat_24
#define OX_F_DATA       &lv_font_montserrat_16
#define OX_F_LABEL      &lv_font_montserrat_12
#define OX_F_MICRO      &lv_font_montserrat_10

/* ── state → colour: the only sanctioned mapping ─────────────────────── */
static inline lv_color_t ox_state_color(RigState s) {
    switch (s) {
        case RIG_NOMINAL: return OX_PHOS;
        case RIG_LOAD:    return OX_AMBER;
        case RIG_THERMAL: return OX_EMBER;
        case RIG_FAULT:   return OX_ALERT;
        default:          return OX_GREY;
    }
}

/* ── shared styles, initialised once by ox_theme_init() ─────────────── */
extern lv_style_t ox_st_screen;   /* VOID ground, square                 */
extern lv_style_t ox_st_panel;    /* STEEL tile, square, no border       */
extern lv_style_t ox_st_rule;     /* WIRE hairline                       */
extern lv_style_t ox_st_label;    /* GREY caption                        */
extern lv_style_t ox_st_value;    /* BONE measured value                 */

void ox_theme_init(void);         /* call once, after lv_init()          */

/* Build a state-striped tile: the two-pixel left edge is the entire
 * ornamental budget of the system, and it always means something. */
lv_obj_t* ox_tile(lv_obj_t* parent, lv_coord_t x, lv_coord_t y,
                  lv_coord_t w, lv_coord_t h, lv_color_t accent);
