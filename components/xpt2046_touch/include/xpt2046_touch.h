#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int clk_pin;
    int mosi_pin;
    int miso_pin;
    int cs_pin;
    int irq_pin;
} xpt2046_touch_config_t;

typedef struct {
    xpt2046_touch_config_t cfg;
    bool initialized;
} xpt2046_touch_handle_t;

esp_err_t xpt2046_touch_init(xpt2046_touch_handle_t *handle, const xpt2046_touch_config_t *cfg);
esp_err_t xpt2046_touch_deinit(xpt2046_touch_handle_t *handle);
bool xpt2046_touch_read(xpt2046_touch_handle_t *handle, uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif