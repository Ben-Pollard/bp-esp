#pragma once

#include <lvgl.h>
#include "blockhaus_palette.h"
#include "blockhaus_signals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*blockhaus_motion_done_cb_t)(void *user_data);

lv_anim_t *blockhaus_sweep_start(lv_obj_t *container, int dir);
void blockhaus_sweep_stop(lv_anim_t **anim);

typedef struct blockhaus_propagate_t *blockhaus_propagate_handle_t;
blockhaus_propagate_handle_t blockhaus_propagate_start(lv_obj_t **blocks, int count, blockhaus_color_t active_color, blockhaus_color_t rest_color, int period_ms);
void blockhaus_propagate_stop(blockhaus_propagate_handle_t *handle);

void blockhaus_cascade(lv_obj_t **blocks, int count, blockhaus_color_t color, int delay_ms, blockhaus_motion_done_cb_t done, void *user_data);
void blockhaus_dissolve(lv_obj_t *obj, int duration_ms, blockhaus_motion_done_cb_t done, void *user_data);

#ifdef __cplusplus
}
#endif