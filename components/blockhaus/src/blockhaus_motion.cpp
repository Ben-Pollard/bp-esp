#include "blockhaus_motion.h"
#include <cstdlib>
#include <cmath>

static uint32_t dim_hex(uint32_t color, float factor)
{
    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * factor);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * factor);
    uint8_t b = (uint8_t)((color & 0xFF) * factor);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* ── Sweep ── */

typedef struct {
    lv_obj_t *sweep;
    lv_coord_t w;
} sweep_ctx_t;

static void sweep_exec_cb(void *var, int32_t v)
{
    sweep_ctx_t *ctx = (sweep_ctx_t *)var;
    lv_obj_set_x(ctx->sweep, (lv_coord_t)v);
}

static void sweep_free_cb(lv_anim_t *a)
{
    sweep_ctx_t *ctx = (sweep_ctx_t *)lv_anim_get_user_data(a);
    if (ctx->sweep) lv_obj_delete(ctx->sweep);
    free(ctx);
}

lv_anim_t *blockhaus_sweep_start(lv_obj_t *container, int dir)
{
    (void)dir;
    lv_coord_t cw = lv_obj_get_width(container);

    sweep_ctx_t *ctx = (sweep_ctx_t *)malloc(sizeof(sweep_ctx_t));
    if (!ctx) return NULL;
    ctx->w = 16;

    ctx->sweep = lv_obj_create(container);
    lv_obj_remove_style_all(ctx->sweep);
    lv_obj_set_size(ctx->sweep, ctx->w, lv_obj_get_height(container));
    lv_obj_set_style_bg_opa(ctx->sweep, LV_OPA_30, 0);
    lv_obj_set_style_bg_color(ctx->sweep, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(ctx->sweep, 0, 0);
    lv_obj_set_pos(ctx->sweep, -ctx->w, 0);

    lv_anim_t *a = (lv_anim_t *)malloc(sizeof(lv_anim_t));
    if (!a) { free(ctx); return NULL; }

    lv_anim_init(a);
    lv_anim_set_var(a, ctx);
    lv_anim_set_exec_cb(a, sweep_exec_cb);
    lv_anim_set_values(a, -ctx->w, cw);
    lv_anim_set_duration(a, 800);
    lv_anim_set_repeat_count(a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_user_data(a, ctx);
    lv_anim_set_deleted_cb(a, sweep_free_cb);
    lv_anim_start(a);
    return a;
}

void blockhaus_sweep_stop(lv_anim_t **anim)
{
    if (!anim || !*anim) return;
    lv_anim_t *a = *anim;
    *anim = NULL;
    sweep_ctx_t *ctx = (sweep_ctx_t *)lv_anim_get_user_data(a);
    lv_anim_delete(ctx, sweep_exec_cb);
    if (ctx->sweep) lv_obj_delete(ctx->sweep);
    free(ctx);
    free(a);
}

/* ── Propagate ── */

struct blockhaus_propagate_t {
    lv_obj_t **blocks;
    int count;
    blockhaus_color_t active;
    blockhaus_color_t rest;
    int pos;
    lv_timer_t *timer;
};

static void propagate_cb(lv_timer_t *tm)
{
    blockhaus_propagate_t *p = (blockhaus_propagate_t *)lv_timer_get_user_data(tm);
    for (int i = 0; i < p->count; i++) {
        int d = abs(i - p->pos);
        blockhaus_color_t c;
        if (d == 0) c = p->active;
        else if (d == 1) c = dim_hex(p->active, 0.35f);
        else c = p->rest;
        lv_obj_set_style_bg_color(p->blocks[i], lv_color_hex(c), 0);
    }
    p->pos = (p->pos + 1) % p->count;
}

blockhaus_propagate_handle_t blockhaus_propagate_start(lv_obj_t **blocks, int count,
    blockhaus_color_t active_color, blockhaus_color_t rest_color, int period_ms)
{
    blockhaus_propagate_t *p = (blockhaus_propagate_t *)malloc(sizeof(blockhaus_propagate_t));
    if (!p) return NULL;
    p->blocks = blocks;
    p->count = count;
    p->active = active_color;
    p->rest = rest_color;
    p->pos = 0;
    p->timer = lv_timer_create(propagate_cb, (uint32_t)period_ms, p);
    return p;
}

void blockhaus_propagate_stop(blockhaus_propagate_handle_t *handle)
{
    if (!handle || !*handle) return;
    blockhaus_propagate_t *p = (blockhaus_propagate_t *)*handle;
    if (p->timer) lv_timer_del(p->timer);
    free(p);
    *handle = NULL;
}

/* ── Cascade ── */

typedef struct {
    lv_obj_t **blocks;
    int count;
    int current;
    blockhaus_color_t color;
    blockhaus_motion_done_cb_t done;
    void *user_data;
    lv_timer_t *timer;
} cascade_ctx_t;

static void cascade_cb(lv_timer_t *tm)
{
    cascade_ctx_t *c = (cascade_ctx_t *)lv_timer_get_user_data(tm);
    if (c->current < c->count) {
        lv_obj_set_style_bg_color(c->blocks[c->current], lv_color_hex(c->color), 0);
        c->current++;
    } else {
        lv_timer_del(c->timer);
        c->timer = NULL;
        if (c->done) c->done(c->user_data);
        free(c);
    }
}

void blockhaus_cascade(lv_obj_t **blocks, int count, blockhaus_color_t color,
    int delay_ms, blockhaus_motion_done_cb_t done, void *user_data)
{
    cascade_ctx_t *c = (cascade_ctx_t *)malloc(sizeof(cascade_ctx_t));
    if (!c) { if (done) done(user_data); return; }
    c->blocks = blocks;
    c->count = count;
    c->current = 0;
    c->color = color;
    c->done = done;
    c->user_data = user_data;
    c->timer = lv_timer_create(cascade_cb, (uint32_t)delay_ms, c);
}

/* ── Dissolve ── */

typedef struct {
    lv_obj_t *obj;
    blockhaus_motion_done_cb_t done;
    void *user_data;
} dissolve_ctx_t;

static void dissolve_exec_cb(void *var, int32_t v)
{
    dissolve_ctx_t *ctx = (dissolve_ctx_t *)var;
    lv_obj_set_style_opa(ctx->obj, (lv_opa_t)v, 0);
}

void blockhaus_dissolve(lv_obj_t *obj, int duration_ms,
    blockhaus_motion_done_cb_t done, void *user_data)
{
    dissolve_ctx_t *ctx = (dissolve_ctx_t *)malloc(sizeof(dissolve_ctx_t));
    if (!ctx) { if (done) done(user_data); return; }
    ctx->obj = obj;
    ctx->done = done;
    ctx->user_data = user_data;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx);
    lv_anim_set_exec_cb(&a, dissolve_exec_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_duration(&a, (uint32_t)duration_ms);
    lv_anim_set_user_data(&a, ctx);
    lv_anim_set_deleted_cb(&a, [](lv_anim_t *anim) {
        free(lv_anim_get_user_data(anim));
    });
    lv_anim_set_ready_cb(&a, [](lv_anim_t *anim) {
        dissolve_ctx_t *ctx = (dissolve_ctx_t *)lv_anim_get_user_data(anim);
        if (ctx->done) ctx->done(ctx->user_data);
        lv_obj_delete(ctx->obj);
    });
    lv_anim_start(&a);
}