#include "blockhaus_shapes.h"
#include "blockhaus_signals.h"
#include <cstddef>
#include <cstdlib>

static void strip_style(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, BLOCKHAUS_CORNER, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

lv_obj_t *blockhaus_block_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    strip_style(obj);
    lv_obj_set_size(obj, w, h);
    return obj;
}

lv_obj_t *blockhaus_dot_create(lv_obj_t *parent, lv_coord_t diameter)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_size(obj, diameter, diameter);
    return obj;
}

lv_obj_t *blockhaus_frame_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(blockhaus_surface()), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, BLOCKHAUS_CORNER, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_size(obj, w, h);
    return obj;
}

lv_obj_t *blockhaus_spectrum_create(lv_obj_t *parent, lv_coord_t block_size, lv_coord_t gap)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, gap, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);

    for (int i = 0; i < blockhaus_spectrum_count(); i++) {
        lv_obj_t *tile = blockhaus_block_create(cont, block_size, block_size);
        blockhaus_shape_set_color(tile, blockhaus_spectrum_color(i));
    }
    return cont;
}

struct blockhaus_bar_t {
    lv_obj_t *container;
    lv_obj_t **blocks;
    int n_blocks;
    int filled;
    int hue;
};

blockhaus_bar_handle_t blockhaus_bar_create(lv_obj_t *parent, int n_blocks,
                                            lv_coord_t block_w, lv_coord_t block_h,
                                            lv_coord_t gap, int hue)
{
    if (n_blocks <= 0) return NULL;

    blockhaus_bar_t *bar = (blockhaus_bar_t *)malloc(sizeof(blockhaus_bar_t));
    if (!bar) return NULL;
    bar->blocks = (lv_obj_t **)calloc((size_t)n_blocks, sizeof(lv_obj_t *));
    if (!bar->blocks) { free(bar); return NULL; }
    bar->n_blocks = n_blocks;
    bar->filled = 0;
    bar->hue = hue;

    bar->container = lv_obj_create(parent);
    lv_obj_remove_style_all(bar->container);
    lv_obj_set_size(bar->container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bar->container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar->container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar->container, gap, 0);
    lv_obj_set_style_pad_all(bar->container, 0, 0);
    lv_obj_set_style_border_width(bar->container, 0, 0);
    lv_obj_set_style_bg_opa(bar->container, LV_OPA_TRANSP, 0);

    for (int i = 0; i < n_blocks; i++) {
        bar->blocks[i] = blockhaus_block_create(bar->container, block_w, block_h);
        lv_obj_set_style_bg_color(bar->blocks[i], lv_color_hex(blockhaus_resting(hue)), 0);
    }

    blockhaus_bar_set_fill(bar, 0);
    return bar;
}

void blockhaus_bar_set_fill(blockhaus_bar_handle_t bar, int filled)
{
    if (!bar) return;
    if (filled < 0) filled = 0;
    if (filled > bar->n_blocks) filled = bar->n_blocks;
    bar->filled = filled;

    for (int i = 0; i < bar->n_blocks; i++) {
        blockhaus_color_t c = (i < filled)
            ? blockhaus_active(bar->hue)
            : blockhaus_resting(bar->hue);
        lv_obj_set_style_bg_color(bar->blocks[i], lv_color_hex(c), 0);
    }
}

void blockhaus_bar_set_hue(blockhaus_bar_handle_t bar, int hue)
{
    if (!bar) return;
    bar->hue = hue;
    blockhaus_bar_set_fill(bar, bar->filled);
}

lv_obj_t *blockhaus_bar_obj(blockhaus_bar_handle_t bar)
{
    return bar ? bar->container : NULL;
}

void blockhaus_shape_set_color(lv_obj_t *obj, blockhaus_color_t color)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
}

void blockhaus_shape_set_hue_signal(lv_obj_t *obj, int hue, int sig)
{
    blockhaus_shape_set_color(obj, blockhaus_color_of(hue, sig));
}