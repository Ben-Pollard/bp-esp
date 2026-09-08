#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_board_manager.h"
#include "dev_display_lcd.h"
#include "periph_ledc.h"
#include "periph_adc.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "xpt2046_touch.h"
#include "src/debugging/sysmon/lv_sysmon.h"

static const char *TAG = "bp-esp";

/* ---- Touch constants ---- */
#define TOUCH_RAW_X_MIN  167
#define TOUCH_RAW_X_MAX  3725
#define TOUCH_RAW_Y_MIN  242
#define TOUCH_RAW_Y_MAX  3776

/* ---- LEDC ---- */
#define LEDC_RES  (LEDC_TIMER_13_BIT)
#define LEDC_MAX  ((1 << 13) - 1)

/* ---- Hardware handles ---- */
static xpt2046_touch_handle_t *s_touch = NULL;
static periph_ledc_handle_t *s_led_red   = NULL;
static periph_ledc_handle_t *s_led_green = NULL;
static periph_ledc_handle_t *s_led_blue  = NULL;
static periph_ledc_handle_t *s_backlight = NULL;
static adc_oneshot_unit_handle_t s_adc   = NULL;

/* ---- Debug overlay ---- */
static lv_obj_t *s_dbg_label = NULL;
static int s_ldr_raw = 0;
static int s_ldr_factor_pct = 50;
static int s_bl_target_pct = 100;
static int s_last_touch_raw_x = 0;
static int s_last_touch_raw_y = 0;
static int s_last_touch_x = 0;
static int s_last_touch_y = 0;
static bool s_touching = false;
static lv_point_t s_touch_point = {0, 0};

/* ---- Particle system ---- */
#define PARTICLE_COUNT 28
typedef struct {
    lv_obj_t *obj;
    float x, y;
    float vx, vy;
    float hue;
} particle_t;
static particle_t s_particles[PARTICLE_COUNT];
static lv_obj_t *s_vis_cont = NULL;

/* ---- LED slider state ---- */
static lv_obj_t *s_rgb_preview = NULL;
static int s_led_pct[3] = {0, 0, 0};

/* ---- Touch helpers ---- */
static void calibrate_touch(uint16_t raw_x, uint16_t raw_y, int *out_x, int *out_y)
{
    int nx = (int)(((int32_t)raw_x - TOUCH_RAW_X_MIN) * 319L / (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN));
    int ny = (int)(((int32_t)raw_y - TOUCH_RAW_Y_MIN) * 239L / (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN));
    int x = 319 - nx;
    int y = 239 - ny;
    *out_x = x < 0 ? 0 : (x > 319 ? 319 : x);
    *out_y = y < 0 ? 0 : (y > 239 ? 239 : y);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t raw_x = 0, raw_y = 0;
    if (s_touch && xpt2046_touch_read(s_touch, &raw_x, &raw_y)) {
        int x, y;
        calibrate_touch(raw_x, raw_y, &x, &y);
        s_last_touch_raw_x = raw_x;
        s_last_touch_raw_y = raw_y;
        s_last_touch_x = x;
        s_last_touch_y = y;
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
        s_touching = true;
        s_touch_point = data->point;
    } else {
        data->state = LV_INDEV_STATE_REL;
        s_touching = false;
    }
}

/* ---- LED helpers ---- */
static void led_set_pct(periph_ledc_handle_t *h, int pct)
{
    if (!h) return;
    uint32_t duty = (uint32_t)pct * LEDC_MAX / 100;
    ledc_set_duty(h->speed_mode, h->channel, duty);
    ledc_update_duty(h->speed_mode, h->channel);
}

static void update_rgb_preview(void)
{
    if (!s_rgb_preview) return;
    lv_color_t c = lv_color_make(
        s_led_pct[0] * 255 / 100,
        s_led_pct[1] * 255 / 100,
        s_led_pct[2] * 255 / 100);
    lv_obj_set_style_bg_color(s_rgb_preview, c, 0);
}

/* ---- LED slider callbacks ---- */
static void led_slider_cb_r(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    s_led_pct[0] = lv_slider_get_value(sl);
    led_set_pct(s_led_red, s_led_pct[0]);
    update_rgb_preview();
}

static void led_slider_cb_g(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    s_led_pct[1] = lv_slider_get_value(sl);
    led_set_pct(s_led_green, s_led_pct[1]);
    update_rgb_preview();
}

static void led_slider_cb_b(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    s_led_pct[2] = lv_slider_get_value(sl);
    led_set_pct(s_led_blue, s_led_pct[2]);
    update_rgb_preview();
}

static void backlight_slider_cb(lv_event_t *e)
{
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    s_bl_target_pct = lv_slider_get_value(sl);
    int duty = (s_bl_target_pct * s_ldr_factor_pct / 100) * LEDC_MAX / 100;
    if (s_backlight) {
        ledc_set_duty(s_backlight->speed_mode, s_backlight->channel, duty);
        ledc_update_duty(s_backlight->speed_mode, s_backlight->channel);
    }
}

/* ---- Particle system ---- */
static void particle_timer_cb(lv_timer_t *tm)
{
    bool touched = s_touching;
    float tx = 0, ty = 0;
    if (touched && s_vis_cont) {
        lv_area_t ca;
        lv_obj_get_coords(s_vis_cont, &ca);
        tx = (float)(s_touch_point.x - ca.x1);
        ty = (float)(s_touch_point.y - ca.y1);
    }

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!s_particles[i].obj) continue;
        if (touched) {
            float dx = s_particles[i].x - tx;
            float dy = s_particles[i].y - ty;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 100.0f && d > 0.5f) {
                float force = (100.0f - d) * 0.04f;
                s_particles[i].vx += dx / d * force;
                s_particles[i].vy += dy / d * force;
            }
        }
        s_particles[i].vx *= 0.992f;
        s_particles[i].vy *= 0.992f;
        s_particles[i].vy += 0.015f;
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        if (s_particles[i].x < -10) s_particles[i].x = 330;
        if (s_particles[i].x > 330) s_particles[i].x = -10;
        if (s_particles[i].y < -10) s_particles[i].y = 210;
        if (s_particles[i].y > 210) s_particles[i].y = -10;
        lv_obj_set_pos(s_particles[i].obj, (int)s_particles[i].x, (int)s_particles[i].y);
        float h = s_particles[i].hue;
        if (h >= 360.0f) h -= 360.0f;
        lv_color_t c = lv_color_hsv_to_rgb((int)h, 180, 255);
        lv_obj_set_style_bg_color(s_particles[i].obj, c, 0);
        lv_obj_set_style_shadow_color(s_particles[i].obj, c, 0);
        s_particles[i].hue += 0.3f + (float)(i % 3) * 0.1f;
    }
}

static void create_particles(lv_obj_t *parent)
{
    s_vis_cont = parent;
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        lv_obj_t *o = lv_obj_create(parent);
        lv_obj_remove_style_all(o);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_width(o, 18, 0);
        lv_obj_set_style_shadow_opa(o, LV_OPA_40, 0);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_set_size(o, 8, 8);
        s_particles[i].obj = o;
        s_particles[i].x  = (float)(rand() % 310);
        s_particles[i].y  = (float)(rand() % 180 + 10);
        s_particles[i].vx = (float)(rand() % 200 - 100) * 0.02f;
        s_particles[i].vy = (float)(rand() % 200 - 100) * 0.02f;
        s_particles[i].hue = (float)(rand() % 360);
    }
    lv_timer_create(particle_timer_cb, 33, NULL);
}

/* ---- LDR auto-brightness task ---- */
static void ldr_task(void *arg)
{
    (void)arg;
    int raw_min = 4095, raw_max = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(200));
        if (!s_adc) continue;
        int raw = 0;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_6, &raw) != ESP_OK) continue;
        if (raw < raw_min) raw_min = raw;
        if (raw > raw_max) raw_max = raw;
        s_ldr_raw = raw;
        int range = raw_max - raw_min;
        /* Invert: bright→low raw, dark→high raw */
        float factor = (range > 10) ? 1.0f - (float)(raw - raw_min) / (float)range : 0.5f;
        if (factor < 0.05f) factor = 0.05f;
        if (factor > 1.0f) factor = 1.0f;
        s_ldr_factor_pct = (int)(factor * 100.0f);

        int duty = s_bl_target_pct * s_ldr_factor_pct / 100;
        duty = duty * LEDC_MAX / 100;
        if (s_backlight) {
            ledc_set_duty(s_backlight->speed_mode, s_backlight->channel, (uint32_t)duty);
            ledc_update_duty(s_backlight->speed_mode, s_backlight->channel);
        }

        lvgl_port_lock(0);
        if (s_dbg_label) {
            lv_label_set_text_fmt(s_dbg_label,
                "raw %d,%d  scr %d,%d\nLDR %u  adj %d%%  BL %d%%",
                s_last_touch_raw_x, s_last_touch_raw_y,
                s_last_touch_x, s_last_touch_y,
                (unsigned)s_ldr_raw,
                s_ldr_factor_pct,
                s_bl_target_pct * s_ldr_factor_pct / 100);
        }
        lvgl_port_unlock();
    }
}

/* ---- Debug overlay setup ---- */
static void setup_debug_overlay(lv_display_t *disp)
{
    lvgl_port_lock(0);
    s_dbg_label = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_color(s_dbg_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(s_dbg_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(s_dbg_label, "raw -,-  scr -,-\nLDR ---  adj --%  BL --%");
    lv_obj_align(s_dbg_label, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_sysmon_show_performance(disp);
    lv_sysmon_show_memory(disp);
    lvgl_port_unlock();
}

/* ---- Splash (from earlier design) ---- */
static void splash_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

static void show_splash(lv_obj_t *scr)
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

/* ---- Controls tab ---- */
static lv_obj_t *create_slider_row(lv_obj_t *parent, const char *label_text,
                                   lv_color_t color, int initial,
                                   lv_event_cb_t cb)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 38);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *sl = lv_slider_create(row);
    lv_obj_set_width(sl, 160);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, initial, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_darken(color, 100), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, color, LV_PART_INDICATOR);
    lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *val_lbl = lv_label_create(row);
    lv_label_set_text_fmt(val_lbl, "%d%%", initial);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(val_lbl, color, 0);

    lv_obj_add_event_cb(sl, [](lv_event_t *e) {
        lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
        lv_obj_t *parent_row = lv_obj_get_parent(target);
        lv_obj_t *val_lbl = lv_obj_get_child(parent_row, 2);
        lv_label_set_text_fmt(val_lbl, "%d%%", (int)lv_slider_get_value(target));
    }, LV_EVENT_VALUE_CHANGED, NULL);

    return row;
}

static void create_controls_tab(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, 4, 0);
    lv_obj_set_style_pad_all(parent, 8, 0);

    lv_obj_t *section = lv_label_create(parent);
    lv_label_set_text(section, "\xe2\x97\x86 RGB LED");
    lv_obj_set_style_text_font(section, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(section, lv_color_hex(0x888888), 0);

    s_rgb_preview = lv_obj_create(parent);
    lv_obj_remove_style_all(s_rgb_preview);
    lv_obj_set_size(s_rgb_preview, lv_pct(100), 20);
    lv_obj_set_style_radius(s_rgb_preview, 6, 0);
    lv_obj_set_style_bg_color(s_rgb_preview, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_rgb_preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_rgb_preview, 0, 0);

    create_slider_row(parent, "R", lv_color_hex(0xff3333), 0, led_slider_cb_r);
    create_slider_row(parent, "G", lv_color_hex(0x33ff33), 0, led_slider_cb_g);
    create_slider_row(parent, "B", lv_color_hex(0x3333ff), 0, led_slider_cb_b);

    section = lv_label_create(parent);
    lv_label_set_text(section, "\xe2\x98\x80 Backlight");
    lv_obj_set_style_text_font(section, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(section, lv_color_hex(0x888888), 0);

    create_slider_row(parent, "BL", lv_color_hex(0xffaa00), 100, backlight_slider_cb);

    section = lv_label_create(parent);
    lv_label_set_text(section, "\xf0\x9f\x8c\x99 Light Sensor");
    lv_obj_set_style_text_font(section, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(section, lv_color_hex(0x888888), 0);

    lv_obj_t *ldr_row = lv_obj_create(parent);
    lv_obj_remove_style_all(ldr_row);
    lv_obj_set_size(ldr_row, lv_pct(100), 28);
    lv_obj_t *ldr_lbl = lv_label_create(ldr_row);
    lv_label_set_text(ldr_lbl, "Raw: ---  Adj: --%");
    lv_obj_set_style_text_font(ldr_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ldr_lbl, lv_color_hex(0x88aacc), 0);

    /* Task to update LDR label in controls tab */
    xTaskCreate([](void *arg) {
        lv_obj_t *label = (lv_obj_t *)arg;
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(500));
            lvgl_port_lock(0);
            if (label) {
                lv_label_set_text_fmt(label, "Raw: %d  Adj: %d%%",
                    s_ldr_raw, s_ldr_factor_pct);
            }
            lvgl_port_unlock();
        }
    }, "ldr_ui", 2048, ldr_lbl, 1, NULL);
}

/* ---- Visual tab ---- */
static void create_visual_tab(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x040810), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    create_particles(parent);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "touch the void");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x1a2a3a), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}

/* ---- Main UI transition ---- */
static void start_main_ui(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(3000));

    lvgl_port_lock(0);
    lv_obj_clean(lv_screen_active());

    lv_obj_t *tv = lv_tabview_create(lv_screen_active());
    lv_obj_set_size(tv, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(lv_tabview_get_content(tv), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tab_ctrl = lv_tabview_add_tab(tv, "Controls");
    lv_obj_t *tab_vis  = lv_tabview_add_tab(tv, "Visual");

    create_controls_tab(tab_ctrl);
    create_visual_tab(tab_vis);

    lvgl_port_unlock();

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}

/* ---- Entry point ---- */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "bp-esp starting...");

    esp_err_t ret = esp_board_manager_init();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Board manager init failed: %d", ret); return; }
    esp_board_manager_print_board_info();

    /* Display */
    void *raw_handle = NULL;
    ret = esp_board_manager_get_device_handle("display_lcd", &raw_handle);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Display handle: %d", ret); return; }
    dev_display_lcd_handles_t *lcd_handles = (dev_display_lcd_handles_t *)raw_handle;

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack = 8192;
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "LVGL port: %d", ret); return; }

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle    = lcd_handles->io_handle;
    disp_cfg.panel_handle = lcd_handles->panel_handle;
    disp_cfg.buffer_size  = 320 * 32;
    disp_cfg.double_buffer = true;
    disp_cfg.hres         = 320;
    disp_cfg.vres         = 240;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.flags.buff_dma  = true;
    disp_cfg.flags.swap_bytes = true;
    disp_cfg.rotation = { .swap_xy = true, .mirror_x = true, .mirror_y = true };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) { ESP_LOGE(TAG, "Display add failed"); return; }

    /* Touch */
    void *touch_raw = NULL;
    ret = esp_board_manager_get_device_handle("xpt2046_touch", &touch_raw);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Touch handle: %d", ret); return; }
    s_touch = (xpt2046_touch_handle_t *)touch_raw;

    lvgl_port_lock(0);
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    lv_indev_set_display(touch_indev, disp);
    lvgl_port_unlock();

    /* Backlight handle */
    void *bl_h = NULL;
    if (esp_board_manager_get_device_handle("lcd_brightness", &bl_h) == ESP_OK)
        s_backlight = (periph_ledc_handle_t *)bl_h;

    /* RGB LED handles */
    void *h = NULL;
    if (esp_board_manager_get_device_handle("led_red", &h) == ESP_OK) {
        s_led_red = (periph_ledc_handle_t *)h;
        /* ILI9341 reset sequence overwrites GPIO4 routing; re-route LEDC signal */
        ledc_channel_config_t red_re = {};
        red_re.gpio_num = 4;
        red_re.channel = s_led_red->channel;
        red_re.timer_sel = LEDC_TIMER_0;
        red_re.speed_mode = s_led_red->speed_mode;
        red_re.duty = 0;
        red_re.hpoint = 0;
        red_re.flags.output_invert = 1;
        ledc_channel_config(&red_re);
    }
    if (esp_board_manager_get_device_handle("led_green", &h) == ESP_OK)
        s_led_green = (periph_ledc_handle_t *)h;
    if (esp_board_manager_get_device_handle("led_blue", &h) == ESP_OK)
        s_led_blue = (periph_ledc_handle_t *)h;

    /* ADC (LDR) handle */
    void *adc_p = NULL;
    if (esp_board_manager_get_periph_handle("adc_light", &adc_p) == ESP_OK) {
        periph_adc_handle_t *ah = (periph_adc_handle_t *)adc_p;
        s_adc = ah->oneshot;
    }

    /* Kconfig flags not available yet — use default atten if ADC init fails */
    if (!s_adc) ESP_LOGW(TAG, "ADC handle unavailable");

    /* Splash */
    lvgl_port_lock(0);
    lv_obj_t *active_screen = lv_screen_active();
    show_splash(active_screen);
    lvgl_port_unlock();

    /* Debug overlay (touch coords + sysmon + LDR in one label) */
    setup_debug_overlay(disp);

    /* LDR auto-brightness task */
    if (s_adc && s_backlight)
        xTaskCreate(ldr_task, "ldr_auto", 3072, NULL, 1, NULL);

    /* Transition to main UI */
    xTaskCreate(start_main_ui, "main_ui", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Running");
}