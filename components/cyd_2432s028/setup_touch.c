#include <stdlib.h>
#include "esp_log.h"
#include "xpt2046_touch.h"
#include "dev_custom.h"
#include "esp_board_entry.h"
#include "gen_board_device_custom.h"

static const char *TAG = "CYD_TOUCH";

static int cyd_xpt2046_touch_init(void *cfg, int cfg_size, void **device_handle)
{
    dev_custom_xpt2046_touch_config_t *touch_cfg = (dev_custom_xpt2046_touch_config_t *)cfg;
    if (!touch_cfg || cfg_size != sizeof(dev_custom_xpt2046_touch_config_t) || !device_handle) {
        ESP_LOGE(TAG, "Invalid config");
        return -1;
    }

    xpt2046_touch_handle_t *handle = calloc(1, sizeof(xpt2046_touch_handle_t));
    if (!handle) {
        ESP_LOGE(TAG, "Failed to allocate touch handle");
        return -1;
    }

    xpt2046_touch_config_t config = {
        .clk_pin = touch_cfg->clk_pin,
        .mosi_pin = touch_cfg->mosi_pin,
        .miso_pin = touch_cfg->miso_pin,
        .cs_pin = touch_cfg->cs_pin,
        .irq_pin = touch_cfg->irq_pin,
    };

    if (xpt2046_touch_init(handle, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init XPT2046 driver");
        free(handle);
        return -1;
    }

    *device_handle = handle;
    return 0;
}

static int cyd_xpt2046_touch_deinit(void *device_handle)
{
    xpt2046_touch_handle_t *handle = (xpt2046_touch_handle_t *)device_handle;
    if (!handle) {
        return -1;
    }
    xpt2046_touch_deinit(handle);
    free(handle);
    return 0;
}

CUSTOM_DEVICE_IMPLEMENT(xpt2046_touch, cyd_xpt2046_touch_init, cyd_xpt2046_touch_deinit);