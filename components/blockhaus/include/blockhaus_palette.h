#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

enum blockhaus_hue_t {
    BLOCKHAUS_HUE_MAROON = 0,
    BLOCKHAUS_HUE_FOREST,
    BLOCKHAUS_HUE_NAVY,
    BLOCKHAUS_HUE_MUSTARD,
    BLOCKHAUS_HUE_NEUTRAL,
    BLOCKHAUS_HUE_COUNT
};

typedef uint32_t blockhaus_color_t;

blockhaus_color_t blockhaus_resting(int hue);
blockhaus_color_t blockhaus_active(int hue);

blockhaus_color_t blockhaus_spectrum_color(int index);
blockhaus_color_t blockhaus_spectrum_active(int index);
int         blockhaus_spectrum_count(void);

blockhaus_color_t blockhaus_bg(void);

#ifdef __cplusplus
}
#endif