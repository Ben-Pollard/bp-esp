#include "screens.h"
#include "controls_tab.h"
#include "visual_tab.h"
#include "supervisor.h"

static void main_ui_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    lv_obj_clean(lv_screen_active());

    lv_obj_t *tv = lv_tabview_create(lv_screen_active());
    lv_obj_set_size(tv, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(lv_tabview_get_content(tv), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tab_ctrl = lv_tabview_add_tab(tv, "Controls");
    lv_obj_t *tab_vis  = lv_tabview_add_tab(tv, "Visual");

    create_controls_tab(tab_ctrl, sup);
    create_visual_tab(tab_vis);

    lv_timer_del(tm);
}

void start_main_ui(Supervisor *sup)
{
    lv_timer_create(main_ui_timer_cb, 3000, sup);
}