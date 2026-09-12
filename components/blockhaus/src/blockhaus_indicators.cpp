#include "blockhaus_indicators.h"
#include "blockhaus_typography.h"
#include "blockhaus_shapes.h"
#include "blockhaus_signals.h"
#include "blockhaus_motion.h"
#include <cstdlib>
#include <cstddef>

/* An indicator is a single block whose colour and motion encode a signal,
 * derived entirely from the shared signal table (blockhaus_signals):
 *   idle    -> resting hue, static
 *   active  -> blink resting<->active of the object hue
 *   warning -> blink mustard
 *   success -> hold forest (no motion; "done")
 *   fault   -> blink maroon, fast
 * The colour for a single block and for a strip come from the same hue and
 * tempo, so a block and a strip showing the same signal always agree. */
struct blockhaus_indicator_t {
    lv_obj_t *obj;
    lv_obj_t *label;
    int hue;
    int signal;
    blockhaus_blink_handle_t blink;
};

blockhaus_indicator_handle_t blockhaus_indicator_create(lv_obj_t *parent, int hue)
{
    blockhaus_indicator_t *ind = (blockhaus_indicator_t *)malloc(sizeof(blockhaus_indicator_t));
    if (!ind) return NULL;
    ind->hue = hue;
    ind->signal = BLOCKHAUS_SIGNAL_IDLE;
    ind->blink = NULL;

    ind->obj = blockhaus_block_create(parent, 14, 14);

    ind->label = lv_label_create(parent);
    lv_obj_set_style_text_font(ind->label, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(ind->label, lv_color_hex(0x888888), 0);
    lv_label_set_text(ind->label, "");

    lv_obj_set_style_bg_color(ind->obj, lv_color_hex(blockhaus_color_of(hue, BLOCKHAUS_SIGNAL_IDLE)), 0);
    return ind;
}

void blockhaus_indicator_set_signal(blockhaus_indicator_handle_t ind, int sig)
{
    if (!ind) return;
    ind->signal = sig;

    if (ind->blink) { blockhaus_blink_stop(&ind->blink); }

    int hue = blockhaus_signal_hue(ind->hue, sig);
    int period = blockhaus_signal_blink_period(sig);

    if (period > 0) {
        ind->blink = blockhaus_blink_start(ind->obj,
                                           blockhaus_resting(hue),
                                           blockhaus_active(hue),
                                           period);
    } else {
        lv_obj_set_style_bg_color(ind->obj, lv_color_hex(blockhaus_color_of(ind->hue, sig)), 0);
    }
}

void blockhaus_indicator_set_label(blockhaus_indicator_handle_t ind, const char *text)
{
    if (!ind || !ind->label) return;
    lv_label_set_text(ind->label, text);
}

int blockhaus_indicator_hue(blockhaus_indicator_handle_t ind)
{
    return ind ? ind->hue : 0;
}

lv_obj_t *blockhaus_indicator_obj(blockhaus_indicator_handle_t ind)
{
    return ind ? ind->obj : NULL;
}