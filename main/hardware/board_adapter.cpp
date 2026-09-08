#include "board_adapter.h"
#include <cstdio>
#include "esp_log.h"
#include "esp_board_manager.h"
#include "dev_display_lcd.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "BOARD";

#define LEDC_MAX ((1 << 13) - 1)

bool BoardAdapter::init()
{
    esp_err_t ret = esp_board_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Board manager init failed: %d", ret);
        return false;
    }
    esp_board_manager_print_board_info();

    void *h = nullptr;
    if (esp_board_manager_get_device_handle("led_red", &h) == ESP_OK) {
        m_led_red = (periph_ledc_handle_t *)h;
        ledc_channel_config_t red_re = {};
        red_re.gpio_num = 4;
        red_re.channel = m_led_red->channel;
        red_re.timer_sel = LEDC_TIMER_0;
        red_re.speed_mode = m_led_red->speed_mode;
        red_re.duty = 0;
        red_re.hpoint = 0;
        red_re.flags.output_invert = 1;
        ledc_channel_config(&red_re);
    }
    if (esp_board_manager_get_device_handle("led_green", &h) == ESP_OK)
        m_led_green = (periph_ledc_handle_t *)h;
    if (esp_board_manager_get_device_handle("led_blue", &h) == ESP_OK)
        m_led_blue = (periph_ledc_handle_t *)h;
    if (esp_board_manager_get_device_handle("lcd_brightness", &h) == ESP_OK)
        m_backlight = (periph_ledc_handle_t *)h;

    void *adc_p = nullptr;
    if (esp_board_manager_get_periph_handle("adc_light", &adc_p) == ESP_OK) {
        periph_adc_handle_t *ah = (periph_adc_handle_t *)adc_p;
        m_adc = ah->oneshot;
    }

    if (!m_adc) ESP_LOGW(TAG, "ADC handle unavailable");

    return true;
}

void BoardAdapter::set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    auto set = [](periph_ledc_handle_t *h, uint8_t v) {
        if (!h) return;
        uint32_t duty = (uint32_t)v * LEDC_MAX / 100;
        ledc_set_duty(h->speed_mode, h->channel, duty);
        ledc_update_duty(h->speed_mode, h->channel);
    };
    set(m_led_red, r);
    set(m_led_green, g);
    set(m_led_blue, b);
}

void BoardAdapter::set_backlight(uint8_t pct)
{
    if (!m_backlight) return;
    uint32_t duty = (uint32_t)pct * LEDC_MAX / 100;
    ledc_set_duty(m_backlight->speed_mode, m_backlight->channel, duty);
    ledc_update_duty(m_backlight->speed_mode, m_backlight->channel);
}

bool BoardAdapter::read_ldr(uint16_t *raw)
{
    if (!m_adc || !raw) return false;
    int val = 0;
    if (adc_oneshot_read(m_adc, ADC_CHANNEL_6, &val) != ESP_OK) return false;
    *raw = (uint16_t)val;
    return true;
}