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
    RGB(0x7A, 0x5A, 0x2E),
    RGB(0x7A, 0x6B, 0x2E),
    RGB(0x2D, 0x4A, 0x2D),
    RGB(0x1A, 0x2A, 0x4A),
};

static const blockhaus_color_t s_spectrum_active[5] = {
    RGB(0xCC, 0x3A, 0x3A),
    RGB(0xE6, 0x8A, 0x2E),
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

blockhaus_color_t blockhaus_bg(void)
{
    return RGB(0x11, 0x15, 0x1A);
}