#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_ili9341.h"
#include "esp_board_manager_includes.h"

static const char *TAG = "CYD_SETUP";

static const ili9341_lcd_init_cmd_t cyd_init_cmds[] = {
    /* Power control B */
    {0xCF, (uint8_t []){0x00, 0xc1, 0x30}, 3, 0},
    /* Power on sequence control */
    {0xED, (uint8_t []){0x64, 0x03, 0x12, 0x81}, 4, 0},
    /* Driver timing control A */
    {0xE8, (uint8_t []){0x85, 0x10, 0x7a}, 3, 0},
    /* Power control A */
    {0xCB, (uint8_t []){0x39, 0x2C, 0x00, 0x34, 0x02}, 5, 0},
    /* Driver timing control */
    {0xEA, (uint8_t []){0x00, 0x00}, 2, 0},
    /* Power control 1 */
    {0xC0, (uint8_t []){0x1b}, 1, 0},
    /* Power control 2 */
    {0xC1, (uint8_t []){0x00}, 1, 0},
    /* Power control 3 */
    {0xC2, (uint8_t []){0x11}, 1, 0},
    /* VCOM control 1 */
    {0xC5, (uint8_t []){0x30, 0x30}, 2, 0},
    /* VCOM control 2 */
    {0xC7, (uint8_t []){0xb7}, 1, 0},
    /* Frame rate control */
    {0xB1, (uint8_t []){0x00, 0x1a}, 2, 0},
    /* Display function control */
    {0xB6, (uint8_t []){0x0A, 0xe6, 0x27, 0x02}, 4, 0},
    /* 3G control */
    {0xF2, (uint8_t []){0x00}, 1, 0},
    /* Pump ratio control */
    {0xF7, (uint8_t []){0x20}, 1, 0},
    /* Timing */
    {0xF1, (uint8_t []){0x01, 0x31}, 2, 0},
    /* Gamma set */
    {0x26, (uint8_t []){0x01}, 1, 0},
    /* Positive gamma correction */
    {0xE0, (uint8_t []){0x0f, 0x2a, 0x28, 0x08, 0x0e, 0x08, 0x54, 0xa9, 0x43, 0x0a, 0x0f, 0x00, 0x00, 0x00, 0x00}, 15, 0},
    /* Negative gamma correction */
    {0xE1, (uint8_t []){0x00, 0x15, 0x17, 0x07, 0x11, 0x06, 0x2b, 0x56, 0x3c, 0x05, 0x10, 0x0f, 0x3f, 0x3f, 0x0F}, 15, 0},
};

__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(panel_dev_config, ESP_ERR_INVALID_ARG, TAG, "panel_dev_config is NULL");

    ili9341_vendor_config_t vendor_cfg = {
        .init_cmds = cyd_init_cmds,
        .init_cmds_size = sizeof(cyd_init_cmds) / sizeof(ili9341_lcd_init_cmd_t),
    };

    esp_lcd_panel_dev_config_t local_config = *panel_dev_config;
    local_config.vendor_config = &vendor_cfg;

    esp_err_t ret = esp_lcd_new_panel_ili9341(io, &local_config, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ILI9341 panel: %s", esp_err_to_name(ret));
    }
    return ret;
}