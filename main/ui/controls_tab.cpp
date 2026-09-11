#include "controls_tab.h"
#include "supervisor.h"
#include "blockhaus.h"
#include <cstdlib>

static blockhaus_indicator_handle_t s_ind_r = NULL;
static blockhaus_indicator_handle_t s_ind_g = NULL;
static blockhaus_indicator_handle_t s_ind_b = NULL;
static lv_obj_t *s_ldr_label = NULL;
static uint8_t s_rgb_val[3] = {0, 0, 0};

#define SLIDER_BLOCKS 10

typedef struct {
    lv_obj_t *value_lbl;
    Supervisor *sup;
} slider_ctx_t;

static void update_indicators(void)
{
    if (!s_ind_r) return;
    int sig_r = (s_rgb_val[0] > 0) ? BLOCKHAUS_SIGNAL_ACTIVE : BLOCKHAUS_SIGNAL_IDLE;
    int sig_g = (s_rgb_val[1] > 0) ? BLOCKHAUS_SIGNAL_ACTIVE : BLOCKHAUS_SIGNAL_IDLE;
    int sig_b = (s_rgb_val[2] > 0) ? BLOCKHAUS_SIGNAL_ACTIVE : BLOCKHAUS_SIGNAL_IDLE;
    blockhaus_indicator_set_signal(s_ind_r, sig_r);
    blockhaus_indicator_set_signal(s_ind_g, sig_g);
    blockhaus_indicator_set_signal(s_ind_b, sig_b);
}

static void led_slider_r(int value, void *user_data)
{
    Supervisor *sup = (Supervisor *)user_data;
    s_rgb_val[0] = (uint8_t)value;
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_indicators();
}

static void led_slider_g(int value, void *user_data)
{
    Supervisor *sup = (Supervisor *)user_data;
    s_rgb_val[1] = (uint8_t)value;
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_indicators();
}

static void led_slider_b(int value, void *user_data)
{
    Supervisor *sup = (Supervisor *)user_data;
    s_rgb_val[2] = (uint8_t)value;
    sup->submit({CommandKind::SetRgb, s_rgb_val[0], s_rgb_val[1], s_rgb_val[2]});
    update_indicators();
}

static void bl_slider(int value, void *user_data)
{
    Supervisor *sup = (Supervisor *)user_data;
    sup->submit({CommandKind::SetBacklight, (uint8_t)value, 0, 0});
}

static void on_rgb_r(int value, void *user_data)
{
    slider_ctx_t *ctx = (slider_ctx_t *)user_data;
    lv_label_set_text_fmt(ctx->value_lbl, "%d%%", value);
    led_slider_r(value, ctx->sup);
}

static void on_rgb_g(int value, void *user_data)
{
    slider_ctx_t *ctx = (slider_ctx_t *)user_data;
    lv_label_set_text_fmt(ctx->value_lbl, "%d%%", value);
    led_slider_g(value, ctx->sup);
}

static void on_rgb_b(int value, void *user_data)
{
    slider_ctx_t *ctx = (slider_ctx_t *)user_data;
    lv_label_set_text_fmt(ctx->value_lbl, "%d%%", value);
    led_slider_b(value, ctx->sup);
}

static void on_bl(int value, void *user_data)
{
    slider_ctx_t *ctx = (slider_ctx_t *)user_data;
    lv_label_set_text_fmt(ctx->value_lbl, "%d%%", value);
    bl_slider(value, ctx->sup);
}

static void create_slider_row(lv_obj_t *parent, const char *label_text, int hue, int initial,
                              blockhaus_slider_cb_t cb, Supervisor *sup)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 28);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 6, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(blockhaus_active(hue)), 0);

    blockhaus_slider_handle_t sl = blockhaus_slider_create(row, SLIDER_BLOCKS, 15, 12, 3, hue);
    blockhaus_slider_set_range(sl, 0, 100);
    blockhaus_slider_set_value(sl, initial);

    lv_obj_t *val_lbl = lv_label_create(row);
    lv_label_set_text_fmt(val_lbl, "%d%%", initial);
    lv_obj_set_style_text_font(val_lbl, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(blockhaus_active(hue)), 0);

    slider_ctx_t *ctx = (slider_ctx_t *)malloc(sizeof(slider_ctx_t));
    if (!ctx) return;
    ctx->value_lbl = val_lbl;
    ctx->sup = sup;
    blockhaus_slider_set_callback(sl, cb, ctx);
}

static void poll_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    AppState s = sup->snapshot();
    if (s_ldr_label) {
        lv_label_set_text_fmt(s_ldr_label, "LDR: %d  adj %d%%  BL %d%%",
            s.ldr_raw, s.ldr_factor, s.backlight_eff);
    }
    s_rgb_val[0] = s.rgb[0];
    s_rgb_val[1] = s.rgb[1];
    s_rgb_val[2] = s.rgb[2];
    update_indicators();
}

void create_controls_tab(lv_obj_t *parent, Supervisor *sup)
{
    lv_obj_t *ind_row = lv_obj_create(parent);
    lv_obj_remove_style_all(ind_row);
    lv_obj_set_size(ind_row, lv_pct(100), 22);
    lv_obj_set_flex_flow(ind_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(ind_row, 12, 0);
    lv_obj_set_style_pad_all(ind_row, 2, 0);

    s_ind_r = blockhaus_indicator_create(ind_row, BLOCKHAUS_HUE_MAROON);
    blockhaus_indicator_set_label(s_ind_r, "R");

    s_ind_g = blockhaus_indicator_create(ind_row, BLOCKHAUS_HUE_FOREST);
    blockhaus_indicator_set_label(s_ind_g, "G");

    s_ind_b = blockhaus_indicator_create(ind_row, BLOCKHAUS_HUE_NAVY);
    blockhaus_indicator_set_label(s_ind_b, "B");

    int i = 0;
    lv_obj_t *child = lv_obj_get_child(ind_row, (uint32_t)i);
    while (child) {
        blockhaus_indicator_handle_t h = (i == 0) ? s_ind_r : (i == 1) ? s_ind_g : s_ind_b;
        lv_obj_center(blockhaus_indicator_obj(h));
        i++;
        child = lv_obj_get_child(ind_row, (uint32_t)i);
    }

    create_slider_row(parent, "R", BLOCKHAUS_HUE_MAROON, 0, on_rgb_r, sup);
    create_slider_row(parent, "G", BLOCKHAUS_HUE_FOREST, 0, on_rgb_g, sup);
    create_slider_row(parent, "B", BLOCKHAUS_HUE_NAVY, 0, on_rgb_b, sup);
    create_slider_row(parent, "BL", BLOCKHAUS_HUE_MUSTARD, 100, on_bl, sup);

    s_ldr_label = lv_label_create(parent);
    lv_label_set_text(s_ldr_label, "LDR: ---  adj ---  BL ---%");
    lv_obj_set_style_text_font(s_ldr_label, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(s_ldr_label, lv_color_hex(0x88aacc), 0);

    lv_timer_create(poll_timer_cb, 250, sup);
}