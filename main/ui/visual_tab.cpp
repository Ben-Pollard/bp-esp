#include "visual_tab.h"
#include "esp_random.h"
#include <cmath>

#define PARTICLE_COUNT 12

typedef struct {
    lv_obj_t *obj;
    float x, y;
    float vx, vy;
    float hue;
} particle_t;

static particle_t s_particles[PARTICLE_COUNT];
static lv_obj_t *s_vis_cont = NULL;

static void particle_timer_cb(lv_timer_t *tm)
{
    if (s_vis_cont && lv_obj_has_flag(s_vis_cont, LV_OBJ_FLAG_HIDDEN))
        return;

    bool touched = false;
    float tx = 0, ty = 0;

    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            touched = lv_indev_get_state(indev) & LV_INDEV_STATE_PR;
            lv_point_t vp;
            lv_indev_get_point(indev, &vp);
            tx = (float)vp.x;
            ty = (float)vp.y;
            break;
        }
        indev = lv_indev_get_next(indev);
    }

    if (touched && s_vis_cont) {
        lv_area_t ca;
        lv_obj_get_coords(s_vis_cont, &ca);
        tx -= (float)ca.x1;
        ty -= (float)ca.y1;
    }

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!s_particles[i].obj) continue;
        if (touched) {
            float dx = s_particles[i].x - tx;
            float dy = s_particles[i].y - ty;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 100.0f && d > 0.5f) {
                float force = (100.0f - d) * 0.04f;
                s_particles[i].vx += dx / d * force;
                s_particles[i].vy += dy / d * force;
            }
        }
        s_particles[i].vx *= 0.992f;
        s_particles[i].vy *= 0.992f;
        s_particles[i].vy += 0.015f;
        s_particles[i].x += s_particles[i].vx;
        s_particles[i].y += s_particles[i].vy;
        if (s_particles[i].x < -10) s_particles[i].x = 330;
        if (s_particles[i].x > 330) s_particles[i].x = -10;
        if (s_particles[i].y < -10) s_particles[i].y = 210;
        if (s_particles[i].y > 210) s_particles[i].y = -10;
        lv_obj_set_pos(s_particles[i].obj, (int)s_particles[i].x, (int)s_particles[i].y);
        float h = s_particles[i].hue;
        if (h >= 360.0f) h -= 360.0f;
        lv_color_t c = lv_color_hsv_to_rgb((int)h, 180, 255);
        lv_obj_set_style_bg_color(s_particles[i].obj, c, 0);
        s_particles[i].hue += 0.3f + (float)(i % 3) * 0.1f;
    }
}

void create_visual_tab(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x040810), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    s_vis_cont = parent;
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        lv_obj_t *o = lv_obj_create(parent);
        lv_obj_remove_style_all(o);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_set_size(o, 8, 8);
        s_particles[i].obj = o;
        s_particles[i].x  = (float)(esp_random() % 310);
        s_particles[i].y  = (float)(esp_random() % 180 + 10);
        s_particles[i].vx = (float)(esp_random() % 200 - 100) * 0.02f;
        s_particles[i].vy = (float)(esp_random() % 200 - 100) * 0.02f;
        s_particles[i].hue = (float)(esp_random() % 360);
    }
    lv_timer_t *pt = lv_timer_create(particle_timer_cb, 66, NULL);
    lv_timer_set_repeat_count(pt, -1);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "touch the void");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x1a2a3a), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}