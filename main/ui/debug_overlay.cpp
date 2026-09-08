#include "debug_overlay.h"
#include "supervisor.h"
#include "esp_lvgl_port.h"

#ifdef CONFIG_ESP_DEBUG_TOUCH_OVERLAY

#include "src/debugging/sysmon/lv_sysmon.h"

static lv_obj_t *s_dbg_label = NULL;

static void dbg_timer_cb(lv_timer_t *tm)
{
    Supervisor *sup = (Supervisor *)lv_timer_get_user_data(tm);
    if (!s_dbg_label || !sup) return;

    AppState s = sup->snapshot();

    int rx = 0, ry = 0;
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_point_t dbg_pt;
            lv_indev_get_point(indev, &dbg_pt);
            rx = dbg_pt.x;
            ry = dbg_pt.y;
            break;
        }
        indev = lv_indev_get_next(indev);
    }

    lv_label_set_text_fmt(s_dbg_label,
        "raw %d,%d  scr %d,%d\nLDR %u  adj %d%%  BL %d%%",
        0, 0,
        rx, ry,
        (unsigned)s.ldr_raw,
        s.ldr_factor,
        s.backlight_eff);
}

void setup_debug_overlay(lv_display_t *disp, Supervisor *sup)
{
    s_dbg_label = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_color(s_dbg_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(s_dbg_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(s_dbg_label, "raw -,-  scr -,-\nLDR ---  adj --%  BL --%");
    lv_obj_align(s_dbg_label, LV_ALIGN_TOP_LEFT, 4, 4);

    lv_sysmon_show_performance(disp);
    lv_sysmon_show_memory(disp);

    lv_timer_create(dbg_timer_cb, 250, sup);
}

#else
void setup_debug_overlay(lv_display_t *, Supervisor *)
{
}
#endif