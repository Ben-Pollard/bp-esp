#include "signals_tab.h"
#include "blockhaus.h"
#include <cstdio>

static blockhaus_indicator_handle_t s_demo_inds[BLOCKHAUS_SIGNAL_COUNT];
static const char *s_demo_labels[BLOCKHAUS_SIGNAL_COUNT] = {
    "idle", "active", "attention", "warning", "critical"
};

void create_signals_tab(lv_obj_t *parent)
{
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_pad_row(parent, 6, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Status indicators");
    lv_obj_set_style_text_font(title, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);

    blockhaus_spectrum_create(parent, 10, 4);

    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 10, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    for (int i = 0; i < BLOCKHAUS_SIGNAL_COUNT; i++) {
        lv_obj_t *col = lv_obj_create(row);
        lv_obj_remove_style_all(col);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(col, 4, 0);
        lv_obj_set_style_pad_all(col, 0, 0);
        lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        int hue = (i < BLOCKHAUS_HUE_COUNT) ? i : BLOCKHAUS_HUE_NAVY;
        blockhaus_indicator_handle_t ind = blockhaus_indicator_create(col, hue);
        blockhaus_indicator_set_signal(ind, i);
        blockhaus_indicator_set_label(ind, s_demo_labels[i]);
        s_demo_inds[i] = ind;

        lv_obj_center(blockhaus_indicator_obj(ind));
    }

    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_remove_style_all(sep);
    lv_obj_set_size(sep, lv_pct(100), 1);
    lv_obj_set_style_bg_opa(sep, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x888888), 0);

    lv_obj_t *sig_title = lv_label_create(parent);
    lv_label_set_text(sig_title, "Signal strip (processing)");
    lv_obj_set_style_text_font(sig_title, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(sig_title, lv_color_hex(0x888888), 0);

    blockhaus_signal_strip_create(parent, 10, BLOCKHAUS_HUE_NAVY);

    lv_obj_t *desc = lv_label_create(parent);
    lv_label_set_text(desc, "Pulse travels across the row —\nreusable processing animation.");
    lv_obj_set_style_text_font(desc, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(desc, lv_color_hex(0x5A7D9A), 0);
}