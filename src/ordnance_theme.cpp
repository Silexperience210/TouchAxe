#include "ordnance_theme.h"

lv_style_t ox_st_screen;
lv_style_t ox_st_panel;
lv_style_t ox_st_rule;
lv_style_t ox_st_label;
lv_style_t ox_st_value;

static bool s_initialised = false;

void ox_theme_init(void) {
    if (s_initialised) return;
    s_initialised = true;

    /* Ground. Near-black, never pure black: pure black on an IPS panel shows
     * every backlight defect, and the 5-7-10 value gives the panels something
     * to sit on. */
    lv_style_init(&ox_st_screen);
    lv_style_set_bg_color(&ox_st_screen, OX_VOID);
    lv_style_set_bg_opa(&ox_st_screen, LV_OPA_COVER);
    lv_style_set_radius(&ox_st_screen, OX_PANEL_R);
    lv_style_set_pad_all(&ox_st_screen, 0);
    lv_style_set_border_width(&ox_st_screen, 0);

    /* Panel. Depth comes from one step of value, not from a shadow. */
    lv_style_init(&ox_st_panel);
    lv_style_set_bg_color(&ox_st_panel, OX_STEEL);
    lv_style_set_bg_opa(&ox_st_panel, LV_OPA_COVER);
    lv_style_set_radius(&ox_st_panel, OX_PANEL_R);
    lv_style_set_border_width(&ox_st_panel, 0);
    lv_style_set_pad_all(&ox_st_panel, OX_GUTTER);

    lv_style_init(&ox_st_rule);
    lv_style_set_bg_color(&ox_st_rule, OX_WIRE);
    lv_style_set_bg_opa(&ox_st_rule, LV_OPA_COVER);
    lv_style_set_radius(&ox_st_rule, 0);
    lv_style_set_border_width(&ox_st_rule, 0);

    /* Captions are quiet, tracked and uppercase. They are labels on an
     * instrument, not headlines. */
    lv_style_init(&ox_st_label);
    lv_style_set_text_color(&ox_st_label, OX_GREY);
    lv_style_set_text_font(&ox_st_label, OX_F_LABEL);
    lv_style_set_text_letter_space(&ox_st_label, 1);

    lv_style_init(&ox_st_value);
    lv_style_set_text_color(&ox_st_value, OX_BONE);
    lv_style_set_text_font(&ox_st_value, OX_F_DISPLAY);
}

lv_obj_t* ox_tile(lv_obj_t* parent, lv_coord_t x, lv_coord_t y,
                  lv_coord_t w, lv_coord_t h, lv_color_t accent) {
    ox_theme_init();

    lv_obj_t* tile = lv_obj_create(parent);
    lv_obj_remove_style_all(tile);
    lv_obj_add_style(tile, &ox_st_panel, 0);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_size(tile, w, h);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    /* The accent stripe. Two pixels, left edge, state-coloured. This is the
     * whole ornamental vocabulary of the system. */
    lv_obj_t* stripe = lv_obj_create(tile);
    lv_obj_remove_style_all(stripe);
    lv_obj_set_style_bg_color(stripe, accent, 0);
    lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);
    lv_obj_set_size(stripe, OX_ACCENT_W, h);
    lv_obj_set_pos(stripe, -OX_GUTTER, -OX_GUTTER);
    lv_obj_clear_flag(stripe, LV_OBJ_FLAG_SCROLLABLE);

    return tile;
}
