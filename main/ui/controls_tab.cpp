#include "controls_tab.h"
#include "supervisor.h"

static lv_obj_t *s_rgb_preview = NULL;
static lv_obj_t *s_ldr_label = NULL;
static uint8_t s_rgb_val[3] = {0, 0, 0};

static void update_rgb_preview(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_rgb_preview) return;
    lv_color_t c = lv_color_make(r * 255 / 100, g * 255 / 100, b * 255 / 100);
    lv_obj_set_style_bg_color(s_rgb_preview, c, 0);
}

static void led_slider_r(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    Supervisor *sup = (Supervisor *)lv_event_get_user_data(e);
    s_rgb_val[0] = (uint8_t)lv_slider_get_value(sl);
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_rgb_preview(s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]);
}

static void led_slider_g(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    Supervisor *sup = (Supervisor *)lv_event_get_user_data(e);
    s_rgb_val[1] = (uint8_t)lv_slider_get_value(sl);
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_rgb_preview(s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]);
}

static void led_slider_b(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    Supervisor *sup = (Supervisor *)lv_event_get_user_data(e);
    s_rgb_val[2] = (uint8_t)lv_slider_get_value(sl);
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_rgb_preview(s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]);
}

static void bl_slider(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    Supervisor *sup = (Supervisor *)lv_event_get_user_data(e);
    uint8_t v = (uint8_t)lv_slider_get_value(sl);
    sup->submit({CommandKind::SetBacklight, v, 0, 0});
}

static lv_obj_t *create_slider_row(lv_obj_t *parent, const char *label_text,
                                    lv_color_t color, int initial,
                                    lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 28);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 6, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);

    lv_obj_t *sl = lv_slider_create(row);
    lv_obj_set_width(sl, 180);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, initial, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_darken(color, 100), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, color, LV_PART_INDICATOR);
    lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, user_data);

    lv_obj_t *val_lbl = lv_label_create(row);
    lv_label_set_text_fmt(val_lbl, "%d%%", initial);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(val_lbl, color, 0);

    lv_obj_add_event_cb(sl, [](lv_event_t *ev) {
        lv_obj_t *target = (lv_obj_t *)lv_event_get_target(ev);
        lv_obj_t *parent_row = lv_obj_get_parent(target);
        lv_obj_t *val_lbl = lv_obj_get_child(parent_row, 2);
        lv_label_set_text_fmt(val_lbl, "%d%%", (int)lv_slider_get_value(target));
    }, LV_EVENT_VALUE_CHANGED, NULL);

    return row;
}

static void poll_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    AppState s = sup->snapshot();
    if (s_ldr_label) {
        lv_label_set_text_fmt(s_ldr_label, "LDR: %d  adj %d%%  BL %d%%",
            s.ldr_raw, s.ldr_factor, s.backlight_eff);
    }
    update_rgb_preview(s.rgb[0], s.rgb[1], s.rgb[2]);
}

void create_controls_tab(lv_obj_t *parent, Supervisor *sup)
{
    s_rgb_preview = lv_obj_create(parent);
    lv_obj_remove_style_all(s_rgb_preview);
    lv_obj_set_size(s_rgb_preview, lv_pct(100), 10);
    lv_obj_set_style_radius(s_rgb_preview, 4, 0);
    lv_obj_set_style_bg_color(s_rgb_preview, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_rgb_preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_rgb_preview, 0, 0);

    create_slider_row(parent, "R", lv_color_hex(0xff3333), 0, led_slider_r, sup);
    create_slider_row(parent, "G", lv_color_hex(0x33ff33), 0, led_slider_g, sup);
    create_slider_row(parent, "B", lv_color_hex(0x3333ff), 0, led_slider_b, sup);
    create_slider_row(parent, "BL", lv_color_hex(0xffaa00), 100, bl_slider, sup);

    s_ldr_label = lv_label_create(parent);
    lv_label_set_text(s_ldr_label, "LDR: ---  adj ---  BL ---%");
    lv_obj_set_style_text_font(s_ldr_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ldr_label, lv_color_hex(0x88aacc), 0);

    lv_timer_create(poll_timer_cb, 250, sup);
}