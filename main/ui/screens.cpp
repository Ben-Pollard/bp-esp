#include "screens.h"
#include "controls_tab.h"
#include "visual_tab.h"
#include "supervisor.h"

static lv_obj_t *s_controls = NULL;
static lv_obj_t *s_visual = NULL;

static void show_controls(lv_event_t *e)
{
    lv_obj_remove_flag(s_controls, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
}

static void show_visual(lv_event_t *e)
{
    lv_obj_remove_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_controls, LV_OBJ_FLAG_HIDDEN);
}

static void main_ui_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    lv_obj_clean(lv_screen_active());
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x11151a), 0);

    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, lv_pct(100), 28);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bar, 2, 0);
    lv_obj_set_style_pad_all(bar, 3, 0);

    lv_obj_t *btn_c = lv_btn_create(bar);
    lv_obj_set_size(btn_c, 150, 22);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(0x2a3a5a), 0);
    lv_obj_t *lbl_c = lv_label_create(btn_c);
    lv_label_set_text(lbl_c, "Controls");
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_c, show_controls, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_v = lv_btn_create(bar);
    lv_obj_set_size(btn_v, 150, 22);
    lv_obj_set_style_bg_color(btn_v, lv_color_hex(0x2a3a5a), 0);
    lv_obj_t *lbl_v = lv_label_create(btn_v);
    lv_label_set_text(lbl_v, "Visual");
    lv_obj_center(lbl_v);
    lv_obj_add_event_cb(btn_v, show_visual, LV_EVENT_CLICKED, NULL);

    int content_y = 30;
    int content_h = 240 - 30;

    s_controls = lv_obj_create(scr);
    lv_obj_remove_style_all(s_controls);
    lv_obj_set_scrollbar_mode(s_controls, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_controls, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_controls, 0, content_y);
    lv_obj_set_size(s_controls, 320, content_h);
    lv_obj_set_flex_flow(s_controls, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(s_controls, 4, 0);
    lv_obj_set_style_pad_row(s_controls, 2, 0);
    create_controls_tab(s_controls, sup);

    s_visual = lv_obj_create(scr);
    lv_obj_remove_style_all(s_visual);
    lv_obj_set_scrollbar_mode(s_visual, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(s_visual, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_visual, 0, content_y);
    lv_obj_set_size(s_visual, 320, content_h);
    lv_obj_add_flag(s_visual, LV_OBJ_FLAG_HIDDEN);
    create_visual_tab(s_visual);

    lv_timer_del(tm);
}

void start_main_ui(Supervisor *sup)
{
    lv_timer_create(main_ui_timer_cb, 3000, sup);
}