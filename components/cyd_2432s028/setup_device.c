#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_ili9341.h"
#include "esp_board_manager_includes.h"

static const char *TAG = "CYD_SETUP";

static const ili9341_lcd_init_cmd_t cyd_vcom_cmds[] = {
    {0xC5, (uint8_t []){0x3E, 0x28}, 2, 0},
    {0xC7, (uint8_t []){0x86}, 1, 0},
};

__attribute__((weak)) esp_err_t lcd_panel_factory_entry_t(esp_lcd_panel_io_handle_t io,
                                                          const esp_lcd_panel_dev_config_t *panel_dev_config,
                                                          esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(panel_dev_config, ESP_ERR_INVALID_ARG, TAG, "panel_dev_config is NULL");

    ili9341_vendor_config_t vendor_cfg = {
        .init_cmds = cyd_vcom_cmds,
        .init_cmds_size = sizeof(cyd_vcom_cmds) / sizeof(ili9341_lcd_init_cmd_t),
    };

    esp_lcd_panel_dev_config_t local_config = *panel_dev_config;
    local_config.vendor_config = &vendor_cfg;

    esp_err_t ret = esp_lcd_new_panel_ili9341(io, &local_config, ret_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ILI9341 panel: %s", esp_err_to_name(ret));
    }
    return ret;
}