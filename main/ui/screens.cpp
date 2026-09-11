#include "screens.h"
#include "controls_tab.h"
#include "visual_tab.h"
#include "signals_tab.h"
#include "supervisor.h"
#include "blockhaus.h"

static lv_obj_t *s_controls = NULL;
static lv_obj_t *s_visual = NULL;
static lv_obj_t *s_signals = NULL;

static lv_obj_t *s_btn[3] = {NULL, NULL, NULL};
static const int s_tab_hue[3] = {BLOCKHAUS_HUE_MUSTARD, BLOCKHAUS_HUE_FOREST, BLOCKHAUS_HUE_NAVY};

static void set_tab_active(int active)
{
    for (int i = 0; i < 3; i++) {
        if (!s_btn[i]) continue;
        lv_obj_set_style_bg_color(s_btn[i],
            lv_color_hex(blockhaus_active(i == active ? s_tab_hue[i] : BLOCKHAUS_HUE_NEUTRAL)), 0);
    }
}

static void show_controls(lv_event_t *e)
{
    lv_obj_remove_flag(s_controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_signals, LV_OBJ_FLAG_HIDDEN);
    set_tab_active(0);
}

static void show_visual(lv_event_t *e)
{
    lv_obj_remove_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_signals, LV_OBJ_FLAG_HIDDEN);
    set_tab_active(1);
}

static void show_signals(lv_event_t *e)
{
    lv_obj_remove_flag(s_signals, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    set_tab_active(2);
}

static void main_ui_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    lv_obj_clean(lv_screen_active());
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(blockhaus_bg()), 0);

    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, lv_pct(100), 28);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bar, 2, 0);
    lv_obj_set_style_pad_all(bar, 3, 0);

    lv_obj_t *btn_c = blockhaus_block_create(bar, 100, 22);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(blockhaus_active(BLOCKHAUS_HUE_MUSTARD)), 0);
    lv_obj_add_flag(btn_c, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_c = lv_label_create(btn_c);
    lv_label_set_text(lbl_c, "Controls");
    lv_obj_set_style_text_font(lbl_c, blockhaus_font_mono(14), 0);
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_c, show_controls, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_v = blockhaus_block_create(bar, 100, 22);
    lv_obj_set_style_bg_color(btn_v, lv_color_hex(blockhaus_resting(BLOCKHAUS_HUE_NEUTRAL)), 0);
    lv_obj_add_flag(btn_v, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_v = lv_label_create(btn_v);
    lv_label_set_text(lbl_v, "Visual");
    lv_obj_set_style_text_font(lbl_v, blockhaus_font_mono(14), 0);
    lv_obj_center(lbl_v);
    lv_obj_add_event_cb(btn_v, show_visual, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_s = blockhaus_block_create(bar, 100, 22);
    lv_obj_set_style_bg_color(btn_s, lv_color_hex(blockhaus_resting(BLOCKHAUS_HUE_NEUTRAL)), 0);
    lv_obj_add_flag(btn_s, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *lbl_s = lv_label_create(btn_s);
    lv_label_set_text(lbl_s, "Signals");
    lv_obj_set_style_text_font(lbl_s, blockhaus_font_mono(14), 0);
    lv_obj_center(lbl_s);
    lv_obj_add_event_cb(btn_s, show_signals, LV_EVENT_CLICKED, NULL);

    s_btn[0] = btn_c;
    s_btn[1] = btn_v;
    s_btn[2] = btn_s;

    int content_y = 30;
    int content_h = 240 - 30;

    s_controls = blockhaus_frame_create(scr, 320, content_h);
    lv_obj_set_scrollbar_mode(s_controls, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_controls, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_controls, 0, content_y);
    lv_obj_set_flex_flow(s_controls, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(s_controls, 4, 0);
    lv_obj_set_style_pad_row(s_controls, 2, 0);
    create_controls_tab(s_controls, sup);

    s_visual = blockhaus_frame_create(scr, 320, content_h);
    lv_obj_set_scrollbar_mode(s_visual, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_visual, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_visual, 0, content_y);
    lv_obj_add_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    create_visual_tab(s_visual);

    s_signals = blockhaus_frame_create(scr, 320, content_h);
    lv_obj_set_scrollbar_mode(s_signals, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_signals, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_signals, 0, content_y);
    lv_obj_add_flag(s_signals, LV_OBJ_FLAG_HIDDEN);
    create_signals_tab(s_signals);

    lv_timer_del(tm);
}

void show_splash(lv_obj_t *scr, blockhaus_ident_done_cb_t done, void *user_data)
{
    blockhaus_ident_show(scr, "bp-esp", done, user_data);
}

void start_main_ui(Supervisor *sup, int delay_ms)
{
    lv_timer_create(main_ui_timer_cb, (uint32_t)delay_ms, sup);
}