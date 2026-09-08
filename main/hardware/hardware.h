#pragma once

#include <cstdint>

class IHardware {
public:
    virtual ~IHardware() = default;

    virtual bool init() = 0;
    virtual void set_rgb(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void set_backlight(uint8_t pct) = 0;
    virtual bool read_ldr(uint16_t *raw) = 0;
};