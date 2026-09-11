#include "blockhaus_shapes.h"
#include "blockhaus_signals.h"
#include <cstddef>

static void remove_style(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

lv_obj_t *blockhaus_block_create(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *obj = lv_obj_create(parent);
    remove_style(obj);
    lv_obj_set_size(obj, size, size);
    return obj;
}

lv_obj_t *blockhaus_bar_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    remove_style(obj);
    lv_obj_set_size(obj, w, h);
    return obj;
}

lv_obj_t *blockhaus_tile_create(lv_obj_t *parent, lv_coord_t size, lv_coord_t radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    remove_style(obj);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_size(obj, size, size);
    return obj;
}

lv_obj_t *blockhaus_dot_create(lv_obj_t *parent, lv_coord_t diameter)
{
    lv_obj_t *obj = lv_obj_create(parent);
    remove_style(obj);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(obj, diameter, diameter);
    return obj;
}

lv_obj_t *blockhaus_frame_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    return obj;
}

lv_obj_t *blockhaus_strip_create(lv_obj_t *parent, int n_blocks, lv_coord_t block_size, lv_coord_t gap)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, gap, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);

    for (int i = 0; i < n_blocks; i++) {
        blockhaus_block_create(cont, block_size);
    }
    return cont;
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
        lv_obj_t *tile = blockhaus_tile_create(cont, block_size, 2);
        blockhaus_shape_set_color(tile, blockhaus_spectrum_color(i));
    }
    return cont;
}

void blockhaus_shape_set_color(lv_obj_t *obj, blockhaus_color_t color)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
}

void blockhaus_shape_set_hue_signal(lv_obj_t *obj, int hue, int sig)
{
    blockhaus_shape_set_color(obj, blockhaus_color_of(hue, sig));
}