#include "brightness.h"

uint8_t brightness_factor(uint16_t raw, uint16_t raw_min, uint16_t raw_max)
{
    int range = (int)raw_max - (int)raw_min;
    if (range < 10) return 50;
    float f = 1.0f - (float)(raw - raw_min) / (float)range;
    if (f < 0.05f) f = 0.05f;
    if (f > 1.0f) f = 1.0f;
    return (uint8_t)(f * 100.0f);
}

uint8_t effective_backlight(uint8_t target_pct, uint8_t factor_pct)
{
    return (uint8_t)((uint16_t)target_pct * (uint16_t)factor_pct / 100);
}