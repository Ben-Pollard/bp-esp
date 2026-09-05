#include "xpt2046_touch.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "XPT2046";

#define CMD_READ_X   0x91
#define CMD_READ_Y   0xD1
#define CMD_READ_Z1  0xB1
#define CMD_READ_Z2  0xC1

#define TOUCH_Z_THRESHOLD  100
#define SAMPLE_DELAY_US    5

static int64_t s_last_dbg = 0;

static void cs_select(const xpt2046_touch_handle_t *h)
{
    gpio_set_level(h->cfg.cs_pin, 0);
}

static void cs_deselect(const xpt2046_touch_handle_t *h)
{
    gpio_set_level(h->cfg.cs_pin, 1);
}

static void spi_write_byte(const xpt2046_touch_handle_t *h, uint8_t data)
{
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(h->cfg.mosi_pin, (data >> i) & 1);
        gpio_set_level(h->cfg.clk_pin, 0);
        esp_rom_delay_us(SAMPLE_DELAY_US);
        gpio_set_level(h->cfg.clk_pin, 1);
        esp_rom_delay_us(SAMPLE_DELAY_US);
    }
    gpio_set_level(h->cfg.mosi_pin, 0);
    gpio_set_level(h->cfg.clk_pin, 0);
}

static uint16_t spi_read_16bits(const xpt2046_touch_handle_t *h)
{
    uint16_t result = 0;
    for (int i = 15; i >= 0; i--) {
        gpio_set_level(h->cfg.clk_pin, 1);
        esp_rom_delay_us(SAMPLE_DELAY_US);
        gpio_set_level(h->cfg.clk_pin, 0);
        esp_rom_delay_us(SAMPLE_DELAY_US);
        result |= (gpio_get_level(h->cfg.miso_pin) << i);
    }
    return result >> 4;
}

static uint16_t read_channel(const xpt2046_touch_handle_t *h, uint8_t cmd)
{
    spi_write_byte(h, cmd);
    return spi_read_16bits(h);
}

esp_err_t xpt2046_touch_init(xpt2046_touch_handle_t *handle, const xpt2046_touch_config_t *cfg)
{
    if (!handle || !cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->cfg = *cfg;
    handle->initialized = false;

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(cfg->clk_pin) | BIT64(cfg->mosi_pin) | BIT64(cfg->cs_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_config_t in_conf = {
        .pin_bit_mask = BIT64(cfg->miso_pin) | BIT64(cfg->irq_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);

    gpio_set_level(cfg->cs_pin, 1);
    gpio_set_level(cfg->clk_pin, 0);

    handle->initialized = true;
    ESP_LOGI(TAG, "XPT2046 initialized (CLK=%d, MOSI=%d, MISO=%d, CS=%d, IRQ=%d)",
             cfg->clk_pin, cfg->mosi_pin, cfg->miso_pin, cfg->cs_pin, cfg->irq_pin);
    return ESP_OK;
}

esp_err_t xpt2046_touch_deinit(xpt2046_touch_handle_t *handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_reset_pin(handle->cfg.clk_pin);
    gpio_reset_pin(handle->cfg.mosi_pin);
    gpio_reset_pin(handle->cfg.cs_pin);
    gpio_reset_pin(handle->cfg.miso_pin);
    gpio_reset_pin(handle->cfg.irq_pin);
    handle->initialized = false;
    ESP_LOGI(TAG, "XPT2046 deinitialized");
    return ESP_OK;
}

bool xpt2046_touch_read(xpt2046_touch_handle_t *handle, uint16_t *x, uint16_t *y)
{
    if (!handle || !handle->initialized || !x || !y) {
        return false;
    }

    int irq_level = gpio_get_level(handle->cfg.irq_pin);

    cs_select(handle);

    uint16_t z1 = read_channel(handle, CMD_READ_Z1);
    uint16_t z = z1 + 4095;
    uint16_t z2 = read_channel(handle, CMD_READ_Z2);
    z -= z2;

    bool pressed = (z >= TOUCH_Z_THRESHOLD);

    uint16_t x_raw = 0;
    uint16_t y_raw = 0;
    if (pressed) {
        x_raw = read_channel(handle, CMD_READ_X);
        y_raw = read_channel(handle, CMD_READ_Y & ~0x01);
    }

    cs_deselect(handle);

    int64_t now = esp_timer_get_time();
    if (now - s_last_dbg >= 500000) {
        s_last_dbg = now;
        ESP_LOGI(TAG, "irq=%d z1=%u z2=%u z=%u pressed=%d x=%u y=%u",
                 irq_level, z1, z2, z, pressed, x_raw, y_raw);
    }

    if (!pressed) {
        return false;
    }

    *x = x_raw;
    *y = y_raw;
    return true;
}