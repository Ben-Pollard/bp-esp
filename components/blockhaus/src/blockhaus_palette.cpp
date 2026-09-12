#include "blockhaus_palette.h"

#define RGB(r, g, b)   (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

static const blockhaus_color_t s_resting[BLOCKHAUS_HUE_COUNT] = {
    RGB(0x5C, 0x2E, 0x2E),
    RGB(0x2D, 0x4A, 0x2D),
    RGB(0x1A, 0x2A, 0x4A),
    RGB(0x7A, 0x6B, 0x2E),
    RGB(0x3A, 0x3A, 0x3A),
};

static const blockhaus_color_t s_active[BLOCKHAUS_HUE_COUNT] = {
    RGB(0xCC, 0x3A, 0x3A),
    RGB(0x4C, 0xAF, 0x50),
    RGB(0x3A, 0x7B, 0xDB),
    RGB(0xE6, 0xC4, 0x4A),
    RGB(0xAA, 0xAA, 0xAA),
};

blockhaus_color_t blockhaus_resting(int hue)
{
    if (hue < 0 || hue >= BLOCKHAUS_HUE_COUNT) return s_resting[BLOCKHAUS_HUE_NEUTRAL];
    return s_resting[hue];
}

blockhaus_color_t blockhaus_active(int hue)
{
    if (hue < 0 || hue >= BLOCKHAUS_HUE_COUNT) return s_active[BLOCKHAUS_HUE_NEUTRAL];
    return s_active[hue];
}

static const blockhaus_color_t s_spectrum_resting[5] = {
    RGB(0x5C, 0x2E, 0x2E),
    RGB(0x8A, 0x3E, 0x1E),
    RGB(0x7A, 0x6B, 0x2E),
    RGB(0x2D, 0x4A, 0x2D),
    RGB(0x1A, 0x2A, 0x4A),
};

static const blockhaus_color_t s_spectrum_active[5] = {
    RGB(0xCC, 0x3A, 0x3A),
    RGB(0xE6, 0x78, 0x1E),
    RGB(0xE6, 0xC4, 0x4A),
    RGB(0x4C, 0xAF, 0x50),
    RGB(0x3A, 0x7B, 0xDB),
};

blockhaus_color_t blockhaus_spectrum_color(int index)
{
    if (index < 0 || index > 4) return 0;
    return s_spectrum_resting[index];
}

blockhaus_color_t blockhaus_spectrum_active(int index)
{
    if (index < 0 || index > 4) return 0;
    return s_spectrum_active[index];
}

int blockhaus_spectrum_count(void)
{
    return 5;
}

static uint32_t mix_color(uint32_t a, uint32_t b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    uint32_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint32_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint8_t r = (uint8_t)(ar + (br - ar) * t);
    uint8_t g = (uint8_t)(ag + (bg - ag) * t);
    uint8_t bl = (uint8_t)(ab + (bb - ab) * t);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
}

blockhaus_color_t blockhaus_spectrum_ramp(float t)
{
    int n = blockhaus_spectrum_count();
    float scaled = t * (float)n;
    int i = (int)scaled;
    float frac = scaled - (float)i;
    i %= n;
    int j = (i + 1) % n;
    return mix_color(s_spectrum_active[i], s_spectrum_active[j], frac);
}

blockhaus_color_t blockhaus_bg(void)
{
    return RGB(0x11, 0x15, 0x1A);
}

blockhaus_color_t blockhaus_surface(void)
{
    return RGB(0x04, 0x08, 0x10);
}