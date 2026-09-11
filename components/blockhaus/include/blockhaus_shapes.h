#pragma once

#include <lvgl.h>
#include "blockhaus_palette.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *blockhaus_block_create(lv_obj_t *parent, lv_coord_t size);
lv_obj_t *blockhaus_bar_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
lv_obj_t *blockhaus_tile_create(lv_obj_t *parent, lv_coord_t size, lv_coord_t radius);
lv_obj_t *blockhaus_dot_create(lv_obj_t *parent, lv_coord_t diameter);
lv_obj_t *blockhaus_frame_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
lv_obj_t *blockhaus_strip_create(lv_obj_t *parent, int n_blocks, lv_coord_t block_size, lv_coord_t gap);
lv_obj_t *blockhaus_spectrum_create(lv_obj_t *parent, lv_coord_t block_size, lv_coord_t gap);

void blockhaus_shape_set_color(lv_obj_t *obj, blockhaus_color_t color);
void blockhaus_shape_set_hue_signal(lv_obj_t *obj, int hue, int sig);

#ifdef __cplusplus
}
#endif