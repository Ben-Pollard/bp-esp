#include <cstdio>
#include "esp_log.h"
#include "esp_board_manager.h"
#include "dev_display_lcd.h"
#include "esp_lvgl_port.h"
#include "esp_task_wdt.h"
#include "xpt2046_touch.h"
#include "hardware/board_adapter.h"
#include "supervisor.h"
#include "ui/screens.h"
#include "ui/debug_overlay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bp-esp";

#define RAW_X_MIN  167
#define RAW_X_MAX  3725
#define RAW_Y_MIN  242
#define RAW_Y_MAX  3776

static xpt2046_touch_handle_t *s_touch = NULL;

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t raw_x = 0, raw_y = 0;
    if (s_touch && xpt2046_touch_read(s_touch, &raw_x, &raw_y)) {
        int nx = (int)(((int32_t)raw_x - RAW_X_MIN) * 319L / (RAW_X_MAX - RAW_X_MIN));
        int ny = (int)(((int32_t)raw_y - RAW_Y_MIN) * 239L / (RAW_Y_MAX - RAW_Y_MIN));
        data->point.x = nx < 0 ? 0 : (nx > 319 ? 319 : 319 - nx);
        data->point.y = ny < 0 ? 0 : (ny > 239 ? 239 : 239 - ny);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "bp-esp starting...");
    esp_log_level_set("spi_master", ESP_LOG_WARN);
    esp_log_level_set("esp_lcd", ESP_LOG_WARN);
    esp_log_level_set("ledc", ESP_LOG_WARN);

    BoardAdapter board;
    if (!board.init()) return;

    void *raw_handle = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle("display_lcd", &raw_handle);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Display handle: %d", ret); return; }
    dev_display_lcd_handles_t *lcd = (dev_display_lcd_handles_t *)raw_handle;

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack = 8192;
    lvgl_cfg.task_affinity = 0;
    ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "LVGL port: %d", ret); return; }

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle    = lcd->io_handle;
    disp_cfg.panel_handle = lcd->panel_handle;
    disp_cfg.buffer_size  = 320 * 32;
    disp_cfg.double_buffer = true;
    disp_cfg.hres         = 320;
    disp_cfg.vres         = 240;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.flags.buff_dma  = true;
    disp_cfg.flags.swap_bytes = true;
    disp_cfg.rotation     = { .swap_xy = true, .mirror_x = true, .mirror_y = true };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (!disp) { ESP_LOGE(TAG, "Display add failed"); return; }

    void *touch_raw = NULL;
    ret = esp_board_manager_get_device_handle("xpt2046_touch", &touch_raw);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Touch handle: %d", ret); return; }
    s_touch = (xpt2046_touch_handle_t *)touch_raw;

    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
    lv_indev_set_display(touch_indev, disp);

    Supervisor supervisor;
    if (!supervisor.start(board)) { ESP_LOGE(TAG, "Supervisor start failed"); return; }

    lvgl_port_lock(0);
    show_splash(lv_screen_active());
    lvgl_port_unlock();

    lvgl_port_lock(0);
    setup_debug_overlay(disp, &supervisor);
    start_main_ui(&supervisor);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Running");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}