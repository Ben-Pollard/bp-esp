#pragma once
#include "hardware/hardware.h"
#include "periph_ledc.h"
#include "periph_adc.h"

class BoardAdapter : public IHardware {
public:
    bool init() override;
    void set_rgb(uint8_t r, uint8_t g, uint8_t b) override;
    void set_backlight(uint8_t pct) override;
    bool read_ldr(uint16_t *raw) override;

private:
    periph_ledc_handle_t *m_led_red   = nullptr;
    periph_ledc_handle_t *m_led_green = nullptr;
    periph_ledc_handle_t *m_led_blue  = nullptr;
    periph_ledc_handle_t *m_backlight = nullptr;
    adc_oneshot_unit_handle_t m_adc   = nullptr;
};