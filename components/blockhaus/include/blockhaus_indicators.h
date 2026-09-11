#pragma once

#include <lvgl.h>
#include "blockhaus_palette.h"
#include "blockhaus_signals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blockhaus_indicator_t *blockhaus_indicator_handle_t;

blockhaus_indicator_handle_t blockhaus_indicator_create(lv_obj_t *parent, int hue);
void blockhaus_indicator_set_signal(blockhaus_indicator_handle_t ind, int sig);
void blockhaus_indicator_set_label(blockhaus_indicator_handle_t ind, const char *text);
int  blockhaus_indicator_hue(blockhaus_indicator_handle_t ind);
lv_obj_t *blockhaus_indicator_obj(blockhaus_indicator_handle_t ind);

#ifdef __cplusplus
}
#endif