#pragma once

#include <cstdint>

uint8_t brightness_factor(uint16_t raw, uint16_t raw_min, uint16_t raw_max);
uint8_t effective_backlight(uint8_t target_pct, uint8_t factor_pct);