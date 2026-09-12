#include "blockhaus_motion.h"
#include "blockhaus_palette.h"
#include <cstdlib>
#include <cmath>

static uint32_t mix_hex(uint32_t a, uint32_t b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    uint8_t r = (uint8_t)((((a >> 16) & 0xFF) * (1.0f - t)) + (((b >> 16) & 0xFF) * t));
    uint8_t g = (uint8_t)((((a >> 8) & 0xFF) * (1.0f - t)) + (((b >> 8) & 0xFF) * t));
    uint8_t bl = (uint8_t)(((a & 0xFF) * (1.0f - t)) + ((b & 0xFF) * t));
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
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

/* ── Pulse ── */

struct blockhaus_pulse_t {
    lv_obj_t **blocks;
    int count;
    int hue;
    blockhaus_pulse_mode_t mode;
    int pos;
    float phase;
    lv_timer_t *timer;
};

/* Envelope falloff: full brightness at the crest, shading to 0 over WIDTH
 * blocks on each side. Gaussian-ish (cosine) so the transition reads as a
 * wave, not a ramp. */
#define PULSE_HALF_WIDTH 3.0f

static float pulse_env(int d)
{
    float dd = (float)d;
    if (dd >= PULSE_HALF_WIDTH) return 0.0f;
    return 0.5f + 0.5f * cosf((dd / PULSE_HALF_WIDTH) * (float)M_PI);
}

static void pulse_cb(lv_timer_t *tm)
{
    blockhaus_pulse_t *p = (blockhaus_pulse_t *)lv_timer_get_user_data(tm);

    /* Fractional crest lets the colour pulse slide smoothly along the
     * spectrum rather than snapping between indices. */
    p->phase += 1.0f;
    int crest = (int)p->phase % p->count;

    for (int i = 0; i < p->count; i++) {
        /* Wrap-around distance so the crest leaves one edge and enters the
         * other continuously. */
        int d = i - crest;
        if (d < 0) d = -d;
        int wrap = p->count - d;
        if (wrap < d) d = wrap;

        float env = pulse_env(d);
        blockhaus_color_t crest_color;
        blockhaus_color_t rest_color = blockhaus_resting(p->hue);

        if (p->mode == BLOCKHAUS_PULSE_COLOR) {
            /* Slide the crest along the spectrum array. */
            int n = blockhaus_spectrum_count();
            float t = (float)crest / (float)p->count;
            int idx = (int)(t * n) % n;
            crest_color = blockhaus_spectrum_active(idx);
        } else {
            crest_color = blockhaus_active(p->hue);
        }

        lv_obj_set_style_bg_color(p->blocks[i],
            lv_color_hex(mix_hex(rest_color, crest_color, env)), 0);
    }
}

blockhaus_pulse_handle_t blockhaus_pulse_start(lv_obj_t **blocks, int count, int hue,
    int period_ms, blockhaus_pulse_mode_t mode)
{
    if (!blocks || count <= 0) return NULL;
    blockhaus_pulse_t *p = (blockhaus_pulse_t *)malloc(sizeof(blockhaus_pulse_t));
    if (!p) return NULL;
    p->blocks = blocks;
    p->count = count;
    p->hue = hue;
    p->mode = mode;
    p->pos = 0;
    p->phase = 0.0f;
    p->timer = lv_timer_create(pulse_cb, (uint32_t)period_ms, p);
    return p;
}

void blockhaus_pulse_stop(blockhaus_pulse_handle_t *handle)
{
    if (!handle || !*handle) return;
    blockhaus_pulse_t *p = *handle;
    if (p->timer) lv_timer_del(p->timer);
    free(p);
    *handle = NULL;
}

/* ── Blink ── */

struct blockhaus_blink_t {
    lv_obj_t *obj;
    blockhaus_color_t rest;
    blockhaus_color_t active;
    int period_ms;
    float phase;
    lv_timer_t *timer;
};

#define BLINK_STEP_MS 40

/* A blink is a clear alternation: hold the active colour, then hold the rest
 * colour, with a brief eased transition between. A square-ish wave with soft
 * edges reads as "blinking", whereas a pure sine at these tempos reads as a
 * tremble/chatter. */
static void blink_cb(lv_timer_t *tm)
{
    blockhaus_blink_t *b = (blockhaus_blink_t *)lv_timer_get_user_data(tm);
    b->phase += (float)BLINK_STEP_MS / (float)b->period_ms;
    if (b->phase >= 1.0f) b->phase -= 1.0f;

    /* 0..1 ramp over a narrow window at the start of the on-phase, hold the
     * rest. Maps to a soft rise, a hold, then a snap back to rest. */
    const float transition = 0.25f;
    float s;
    if (b->phase < transition) {
        float t = b->phase / transition;
        s = t * t * (3.0f - 2.0f * t); /* smoothstep rise */
    } else if (b->phase < 0.5f) {
        s = 1.0f; /* hold lit */
    } else if (b->phase < 0.5f + transition) {
        float t = (b->phase - 0.5f) / transition;
        s = 1.0f - (t * t * (3.0f - 2.0f * t)); /* smoothstep fall */
    } else {
        s = 0.0f; /* hold dark */
    }

    lv_obj_set_style_bg_color(b->obj, lv_color_hex(mix_hex(b->rest, b->active, s)), 0);
}

blockhaus_blink_handle_t blockhaus_blink_start(lv_obj_t *obj,
                                               blockhaus_color_t rest_color,
                                               blockhaus_color_t active_color,
                                               int period_ms)
{
    if (!obj || period_ms <= 0) return NULL;
    blockhaus_blink_t *b = (blockhaus_blink_t *)malloc(sizeof(blockhaus_blink_t));
    if (!b) return NULL;
    b->obj = obj;
    b->rest = rest_color;
    b->active = active_color;
    b->period_ms = period_ms;
    b->phase = 0.0f;
    b->timer = lv_timer_create(blink_cb, BLINK_STEP_MS, b);
    return b;
}

void blockhaus_blink_stop(blockhaus_blink_handle_t *handle)
{
    if (!handle || !*handle) return;
    blockhaus_blink_t *b = *handle;
    if (b->timer) lv_timer_del(b->timer);
    free(b);
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