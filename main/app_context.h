#pragma once

#include <cstdint>

struct AppState {
    uint8_t  rgb[3] = {0, 0, 0};
    uint8_t  backlight = 100;
    uint16_t ldr_raw = 0;
    uint8_t  ldr_factor = 100;
    uint8_t  backlight_eff = 100;
    uint32_t version = 0;
};

enum class CommandKind : uint8_t {
    SetRgb,
    SetBacklight,
};

struct Command {
    CommandKind kind;
    uint8_t a;
    uint8_t b;
    uint8_t c;
};