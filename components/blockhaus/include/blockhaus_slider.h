#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Interactive slider built on top of blockhaus_bar. Maps touch position
 * along a segmented bar to a value in [min, max]. The bar itself is the
 * touch target; its click area is extended so a thin bar stays grabbable.
 * The component owns its underlying bar and its blocks. */
typedef struct blockhaus_slider_t *blockhaus_slider_handle_t;
typedef void (*blockhaus_slider_cb_t)(int value, void *user_data);

blockhaus_slider_handle_t blockhaus_slider_create(lv_obj_t *parent, int n_blocks,
                                                  lv_coord_t block_w, lv_coord_t block_h,
                                                  lv_coord_t gap, int hue);
void blockhaus_slider_set_range(blockhaus_slider_handle_t slider, int min, int max);
void blockhaus_slider_set_value(blockhaus_slider_handle_t slider, int value);
int  blockhaus_slider_get_value(blockhaus_slider_handle_t slider);
void blockhaus_slider_set_callback(blockhaus_slider_handle_t slider,
                                   blockhaus_slider_cb_t cb, void *user_data);
lv_obj_t *blockhaus_slider_obj(blockhaus_slider_handle_t slider);

#ifdef __cplusplus
}
#endif