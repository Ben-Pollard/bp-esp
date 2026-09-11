#include "blockhaus_slider.h"
#include "blockhaus_shapes.h"
#include "blockhaus_palette.h"
#include <cstdlib>

struct blockhaus_slider_t {
    blockhaus_bar_handle_t bar;
    lv_obj_t *bar_obj;
    int n_blocks;
    int min;
    int max;
    int value;
    blockhaus_slider_cb_t cb;
    void *user_data;
};

static void slider_input_cb(lv_event_t *e)
{
    blockhaus_slider_t *s = (blockhaus_slider_t *)lv_event_get_user_data(e);
    if (!s || !s->bar_obj) return;

    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    lv_area_t a;
    lv_obj_get_coords(s->bar_obj, &a);
    int w = a.x2 - a.x1;
    int rel = p.x - a.x1;
    if (rel < 0) rel = 0;
    if (rel > w) rel = w;

    int v = s->min + (w > 0 ? (int)(((long)rel * (s->max - s->min)) / w) : s->min);

    if (v != s->value) {
        s->value = v;
        blockhaus_bar_set_fill(s->bar, ((v - s->min) * s->n_blocks) / (s->max - s->min));
        if (s->cb) s->cb(v, s->user_data);
    }
}

blockhaus_slider_handle_t blockhaus_slider_create(lv_obj_t *parent, int n_blocks,
                                                  lv_coord_t block_w, lv_coord_t block_h,
                                                  lv_coord_t gap, int hue)
{
    if (n_blocks <= 0) return NULL;

    blockhaus_slider_t *s = (blockhaus_slider_t *)malloc(sizeof(blockhaus_slider_t));
    if (!s) return NULL;
    s->bar = blockhaus_bar_create(parent, n_blocks, block_w, block_h, gap, hue);
    if (!s->bar) { free(s); return NULL; }
    s->bar_obj = blockhaus_bar_obj(s->bar);
    s->n_blocks = n_blocks;
    s->min = 0;
    s->max = 100;
    s->value = 0;
    s->cb = NULL;
    s->user_data = NULL;

    lv_obj_add_flag(s->bar_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(s->bar_obj, 10);

    lv_obj_add_event_cb(s->bar_obj, slider_input_cb, LV_EVENT_CLICKED, s);
    lv_obj_add_event_cb(s->bar_obj, slider_input_cb, LV_EVENT_PRESSING, s);

    return s;
}

void blockhaus_slider_set_range(blockhaus_slider_handle_t slider, int min, int max)
{
    if (!slider || max <= min) return;
    slider->min = min;
    slider->max = max;
    blockhaus_slider_set_value(slider, slider->value);
}

void blockhaus_slider_set_value(blockhaus_slider_handle_t slider, int value)
{
    if (!slider) return;
    if (value < slider->min) value = slider->min;
    if (value > slider->max) value = slider->max;
    slider->value = value;
    blockhaus_bar_set_fill(slider->bar, ((value - slider->min) * slider->n_blocks) / (slider->max - slider->min));
}

int blockhaus_slider_get_value(blockhaus_slider_handle_t slider)
{
    return slider ? slider->value : 0;
}

void blockhaus_slider_set_callback(blockhaus_slider_handle_t slider,
                                   blockhaus_slider_cb_t cb, void *user_data)
{
    if (!slider) return;
    slider->cb = cb;
    slider->user_data = user_data;
}

lv_obj_t *blockhaus_slider_obj(blockhaus_slider_handle_t slider)
{
    return slider ? slider->bar_obj : NULL;
}