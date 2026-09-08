#include "splash.h"
#include <lvgl.h>

static void splash_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

void show_splash(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a1628), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "bp-esp");
    lv_obj_set_style_text_color(label, lv_color_hex(0x00d4ff), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
    lv_obj_center(label);
    lv_obj_set_style_opa(label, LV_OPA_0, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_exec_cb(&a, splash_anim_cb);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_delay(&a, 200);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_obj_t *sub = lv_label_create(scr);
    lv_label_set_text(sub, "ESP32  \xc2\xb7  LVGL 9.5");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x5a7d9a), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, 0);
    lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_obj_set_style_opa(sub, LV_OPA_0, 0);
    lv_anim_init(&a);
    lv_anim_set_var(&a, sub);
    lv_anim_set_exec_cb(&a, splash_anim_cb);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_delay(&a, 500);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_obj_t *arc = lv_arc_create(scr);
    lv_obj_set_size(arc, 80, 80);
    lv_arc_set_range(arc, 0, 360);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1a2a3a), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00d4ff), LV_PART_INDICATOR);
    lv_obj_center(arc);
    lv_obj_set_style_opa(arc, LV_OPA_0, 0);
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, splash_anim_cb);
    lv_anim_set_duration(&a, 400);
    lv_anim_set_delay(&a, 800);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_value);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_delay(&a, 800);
    lv_anim_set_values(&a, 0, 360);
    lv_anim_start(&a);
}