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

/* Pulse: a wave travelling across a row of blocks. A crest block is fully
 * lit; neighbours shade off toward the rest colour over an envelope window,
 * so the transition is a continuous wave rather than a discrete hop.
 *
 * LIGHTNESS: the crest is the active hue and shades toward the rest hue.
 * COLOUR:    the crest slides along the spectrum array as it travels, so the
 *            wave rolls through the palette (the spectrum motif). */
typedef enum {
    BLOCKHAUS_PULSE_LIGHTNESS = 0,
    BLOCKHAUS_PULSE_COLOR,
} blockhaus_pulse_mode_t;

typedef struct blockhaus_pulse_t *blockhaus_pulse_handle_t;
blockhaus_pulse_handle_t blockhaus_pulse_start(lv_obj_t **blocks, int count, int hue,
                                               int period_ms, blockhaus_pulse_mode_t mode);
void blockhaus_pulse_stop(blockhaus_pulse_handle_t *handle);

/* Blink: a single block oscillating its fill between two colours on a sine
 * wave (rest ↔ active). This is the single-block equivalent of the pulse:
 * where the pulse travels across a strip, the blink "breathes" in place so
 * a lone tally light can still convey a temporal state. */
typedef struct blockhaus_blink_t *blockhaus_blink_handle_t;
blockhaus_blink_handle_t blockhaus_blink_start(lv_obj_t *obj,
                                               blockhaus_color_t rest_color,
                                               blockhaus_color_t active_color,
                                               int period_ms);
void blockhaus_blink_stop(blockhaus_blink_handle_t *handle);

void blockhaus_cascade(lv_obj_t **blocks, int count, blockhaus_color_t color, int delay_ms, blockhaus_motion_done_cb_t done, void *user_data);
void blockhaus_dissolve(lv_obj_t *obj, int duration_ms, blockhaus_motion_done_cb_t done, void *user_data);

#ifdef __cplusplus
}
#endif