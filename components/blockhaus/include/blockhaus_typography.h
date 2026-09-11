#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_BLOCKHAUS_FONT_MONO
LV_FONT_DECLARE(lv_font_jetbrains_mono_14);
LV_FONT_DECLARE(lv_font_jetbrains_mono_24);
LV_FONT_DECLARE(lv_font_jetbrains_mono_40);
#endif

const lv_font_t *blockhaus_font_mono(int size_px);

#ifdef __cplusplus
}
#endif