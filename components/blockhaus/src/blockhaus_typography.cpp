#include "blockhaus_typography.h"

const lv_font_t *blockhaus_font_mono(int size_px)
{
#ifdef CONFIG_BLOCKHAUS_FONT_MONO
    if (size_px <= 14) return &lv_font_jetbrains_mono_14;
    if (size_px <= 24) return &lv_font_jetbrains_mono_24;
    return &lv_font_jetbrains_mono_40;
#else
    if (size_px <= 16) return &lv_font_montserrat_16;
    return &lv_font_montserrat_48;
#endif
}