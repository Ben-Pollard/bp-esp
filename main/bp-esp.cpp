#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_board_manager.h"
#include "dev_display_lcd.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "demos/lv_demos.h"
#include "xpt2046_touch.h"
#include "src/debugging/sysmon/lv_sysmon.h"

static const char *TAG = "bp-esp";

static void splash_anim_cb(void *var, int32_t v)
{
    (void)var;
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

static void show_splash(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a1628), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "bp-esp");
    lv_obj_set_style_text_color(label, lv_color_hex(0x00d4ff), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_48, LV_STATE_DEFAULT);
    lv_obj_center(label);
    lv_obj_set_style_opa(label, LV_OPA_0, LV_STATE_DEFAULT);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_exec_cb(&a, splash_anim_cb);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_delay(&a, 200);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_start(&a);

    lv_obj_t *sub = lv_label_create(scr);
    lv_label_set_text(sub, "ESP32  ·  LVGL 9.5");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x5a7d9a), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_align(sub, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_opa(sub, LV_OPA_0, LV_STATE_DEFAULT);

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
    lv_obj_set_style_opa(arc, LV_OPA_0, LV_STATE_DEFAULT);

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

// Touch calibration: XPT2046 raw ADC range -> 320x240 screen coordinates.
// Both axes are inverted on this panel: raw X is max at the left edge and
// raw Y is max at the top edge, so each axis is mirrored before clamping.
#define TOUCH_RAW_X_MIN  750
#define TOUCH_RAW_X_MAX  2850
#define TOUCH_RAW_Y_MIN  1000
#define TOUCH_RAW_Y_MAX  3500

static xpt2046_touch_handle_t *s_touch = NULL;

#if CONFIG_ESP_DEBUG_TOUCH_OVERLAY
static lv_obj_t *s_touch_dbg = NULL;
#endif

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
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    if (s_touch && xpt2046_touch_read(s_touch, &raw_x, &raw_y)) {
        int x, y;
        calibrate_touch(raw_x, raw_y, &x, &y);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
#if CONFIG_ESP_DEBUG_TOUCH_OVERLAY
        if (s_touch_dbg) {
            lv_label_set_text_fmt(s_touch_dbg, "raw %u,%u\nscr %d,%d",
                                  (unsigned)raw_x, (unsigned)raw_y, x, y);
        }
#endif
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void start_lvgl_demo(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(3000));

    lvgl_port_lock(0);
    lv_obj_clean(lv_screen_active());
    lv_demo_widgets();
    lvgl_port_unlock();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "bp-esp starting...");

    esp_err_t ret = esp_board_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board manager init failed: %d", ret);
        return;
    }
    esp_board_manager_print_board_info();

    void *raw_handle = NULL;
    ret = esp_board_manager_get_device_handle("display_lcd", &raw_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get display handle: %d", ret);
        return;
    }
    dev_display_lcd_handles_t *lcd_handles = (dev_display_lcd_handles_t *)raw_handle;

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack = 8192;
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port init failed: %d", ret);
        return;
    }

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle   = lcd_handles->io_handle;
    disp_cfg.panel_handle = lcd_handles->panel_handle;
    disp_cfg.buffer_size = 320 * 32;
    disp_cfg.double_buffer = true;
    disp_cfg.hres        = 320;
    disp_cfg.vres        = 240;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.flags.buff_dma   = true;
    disp_cfg.flags.swap_bytes = true;
    disp_cfg.rotation = {
        .swap_xy = true,
        .mirror_x = true,
        .mirror_y = true,
    };

    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to add display");
        return;
    }

    void *touch_raw = NULL;
    ret = esp_board_manager_get_device_handle("xpt2046_touch", &touch_raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get touch handle: %d", ret);
        return;
    }
    s_touch = (xpt2046_touch_handle_t *)touch_raw;

    lvgl_port_lock(0);
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    lv_indev_set_display(touch_indev, disp);
    lvgl_port_unlock();

    lvgl_port_lock(0);
    lv_obj_t *active_screen = lv_screen_active();
    show_splash(active_screen);
    lvgl_port_unlock();

#if CONFIG_ESP_DEBUG_TOUCH_OVERLAY
    lvgl_port_lock(0);
    s_touch_dbg = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_color(s_touch_dbg, lv_color_hex(0x00ff00), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_touch_dbg, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_label_set_text(s_touch_dbg, "raw -,-\nscr -,-");
    lv_obj_align(s_touch_dbg, LV_ALIGN_TOP_LEFT, 4, 4);
    lv_sysmon_show_performance(disp);
    lv_sysmon_show_memory(disp);
    lvgl_port_unlock();
#endif

    xTaskCreate(start_lvgl_demo, "lvgl_demo", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Running LVGL...");
}