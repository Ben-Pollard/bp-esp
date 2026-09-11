#include "blockhaus_indicators.h"
#include "blockhaus_typography.h"
#include "blockhaus_shapes.h"
#include <cstdlib>
#include <cstddef>

struct blockhaus_indicator_t {
    lv_obj_t *obj;
    lv_obj_t *label;
    int hue;
    int signal;
    lv_timer_t *pulse_timer;
    int pulse_count;
};

static void indicator_pulse_cb(lv_timer_t *tm)
{
    blockhaus_indicator_t *ind = (blockhaus_indicator_t *)lv_timer_get_user_data(tm);
    ind->pulse_count++;
    bool on = (ind->pulse_count % 2) == 0;

    if (ind->signal == BLOCKHAUS_SIGNAL_WARNING) {
        blockhaus_color_t c = on ? blockhaus_active(BLOCKHAUS_HUE_MUSTARD) : blockhaus_resting(ind->hue);
        lv_obj_set_style_bg_color(ind->obj, lv_color_hex(c), 0);
    } else if (ind->signal == BLOCKHAUS_SIGNAL_CRITICAL) {
        blockhaus_color_t c = on ? blockhaus_active(BLOCKHAUS_HUE_MAROON) : blockhaus_resting(ind->hue);
        lv_obj_set_style_bg_color(ind->obj, lv_color_hex(c), 0);
    } else {
        blockhaus_color_t c = on ? blockhaus_active(ind->hue) : blockhaus_resting(ind->hue);
        lv_obj_set_style_bg_color(ind->obj, lv_color_hex(c), 0);
    }
}

blockhaus_indicator_handle_t blockhaus_indicator_create(lv_obj_t *parent, int hue)
{
    blockhaus_indicator_t *ind = (blockhaus_indicator_t *)malloc(sizeof(blockhaus_indicator_t));
    if (!ind) return NULL;
    ind->hue = hue;
    ind->signal = BLOCKHAUS_SIGNAL_IDLE;
    ind->pulse_timer = NULL;
    ind->pulse_count = 0;

    ind->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(ind->obj);
    lv_obj_set_style_radius(ind->obj, BLOCKHAUS_CORNER, 0);
    lv_obj_set_style_border_width(ind->obj, 0, 0);
    lv_obj_set_style_bg_opa(ind->obj, LV_OPA_COVER, 0);
    lv_obj_set_size(ind->obj, 14, 14);

    ind->label = lv_label_create(parent);
    lv_obj_set_style_text_font(ind->label, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(ind->label, lv_color_hex(0x888888), 0);
    lv_label_set_text(ind->label, "");

    lv_obj_set_style_bg_color(ind->obj, lv_color_hex(blockhaus_resting(hue)), 0);
    return ind;
}

void blockhaus_indicator_set_signal(blockhaus_indicator_handle_t ind, int sig)
{
    if (!ind) return;

    if (ind->pulse_timer) {
        lv_timer_del(ind->pulse_timer);
        ind->pulse_timer = NULL;
    }
    ind->pulse_count = 0;
    ind->signal = sig;

    blockhaus_color_t color = blockhaus_color_of(ind->hue, sig);
    lv_obj_set_style_bg_color(ind->obj, lv_color_hex(color), 0);

    if (sig == BLOCKHAUS_SIGNAL_ACTIVE || sig == BLOCKHAUS_SIGNAL_ATTENTION) {
        ind->pulse_timer = lv_timer_create(indicator_pulse_cb, 600, ind);
    } else if (sig == BLOCKHAUS_SIGNAL_WARNING || sig == BLOCKHAUS_SIGNAL_CRITICAL) {
        ind->pulse_timer = lv_timer_create(indicator_pulse_cb, 300, ind);
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