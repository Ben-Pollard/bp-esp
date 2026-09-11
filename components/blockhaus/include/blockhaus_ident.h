#pragma once

#include <lvgl.h>
#include "blockhaus_palette.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*blockhaus_ident_done_cb_t)(void *user_data);

void blockhaus_ident_show(lv_obj_t *scr, const char *name, blockhaus_ident_done_cb_t done, void *user_data);

typedef struct blockhaus_signal_strip_t *blockhaus_signal_strip_handle_t;

blockhaus_signal_strip_handle_t blockhaus_signal_strip_create(lv_obj_t *parent, int n_blocks, int hue);
void blockhaus_signal_strip_set_signal(blockhaus_signal_strip_handle_t strip, int sig);
void blockhaus_signal_strip_set_blocks(blockhaus_signal_strip_handle_t strip, int n);
void blockhaus_signal_strip_destroy(blockhaus_signal_strip_handle_t strip);

#ifdef __cplusplus
}
#endif