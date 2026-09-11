#pragma once

#include <lvgl.h>
#include "blockhaus_palette.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Single source of truth for the blockhaus corner radius.
 * Every filled shape (block, bar segments) uses this so the geometry
 * stays consistent across the language. The dot is the sole outlier
 * (a circle). */
#define BLOCKHAUS_CORNER 2

lv_obj_t *blockhaus_block_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
lv_obj_t *blockhaus_dot_create(lv_obj_t *parent, lv_coord_t diameter);
lv_obj_t *blockhaus_frame_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);
lv_obj_t *blockhaus_spectrum_create(lv_obj_t *parent, lv_coord_t block_size, lv_coord_t gap);

/* Segmented fill meter: N blocks laid out in a row, the first `fill`
 * lit in the active hue and the rest resting. Owns a group of blocks,
 * driven as a whole rather than a solid bar. */
typedef struct blockhaus_bar_t *blockhaus_bar_handle_t;

blockhaus_bar_handle_t blockhaus_bar_create(lv_obj_t *parent, int n_blocks,
                                            lv_coord_t block_w, lv_coord_t block_h,
                                            lv_coord_t gap, int hue);
void blockhaus_bar_set_fill(blockhaus_bar_handle_t bar, int filled);
void blockhaus_bar_set_hue(blockhaus_bar_handle_t bar, int hue);
lv_obj_t *blockhaus_bar_obj(blockhaus_bar_handle_t bar);

void blockhaus_shape_set_color(lv_obj_t *obj, blockhaus_color_t color);
void blockhaus_shape_set_hue_signal(lv_obj_t *obj, int hue, int sig);

#ifdef __cplusplus
}
#endif