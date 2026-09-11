#include "blockhaus_ident.h"
#include "blockhaus_shapes.h"
#include "blockhaus_motion.h"
#include "blockhaus_typography.h"
#include <cstdlib>
#include <cmath>

#define N_BLOCKS 5

#define GOLDEN_ANGLE 2.39996323f     /* golden angle = 137.5077 deg */
#define SPIRAL_SWEEP 6.2831853f      /* one full turn, radians */
#define SPIRAL_CX    160.0f
#define SPIRAL_CY    120.0f
#define SPIRAL_R0    122.0f          /* start radius near the screen edge */
#define SPIRAL_TURNS 1.25f           /* windings per block for the arrival */

typedef struct {
    lv_obj_t *block;
    float a0, a1;                    /* polar angle, radians */
    float r0, r1;                    /* radius, px */
} spiral_move_t;

typedef struct {
    lv_obj_t *scr;
    lv_obj_t *blocks[N_BLOCKS];
    lv_obj_t *spectrum_group;
    lv_obj_t *wordmark;
    lv_obj_t *subtitle;
    spiral_move_t moves[N_BLOCKS];
    blockhaus_ident_done_cb_t done;
    void *user_data;
    int step;
} ident_ctx_t;

static const lv_point_t s_die5[N_BLOCKS] = {
    {110, 75}, {210, 75}, {160, 120}, {110, 165}, {210, 165},
};

static ident_ctx_t *s_ctx = NULL;

/* ── Golden-spiral block movement ── */

static void spiral_exec_cb(void *var, int32_t v)
{
    spiral_move_t *m = (spiral_move_t *)var;
    float t = (float)v / 1000.0f;
    float a = m->a0 + (m->a1 - m->a0) * t;
    float r = m->r0 + (m->r1 - m->r0) * t;
    lv_obj_set_pos(m->block,
        (lv_coord_t)(SPIRAL_CX + r * cosf(a)),
        (lv_coord_t)(SPIRAL_CY + r * sinf(a)));
}

static void animate_block_spiral(spiral_move_t *m, int dur_ms, int delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, m);
    lv_anim_set_exec_cb(&a, spiral_exec_cb);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_duration(&a, (uint32_t)dur_ms);
    lv_anim_set_delay(&a, (uint32_t)delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/* ── Phase functions ── */

static void phase_spectrum(ident_ctx_t *ctx);
static void phase_wordmark(ident_ctx_t *ctx);
static void phase_finish(ident_ctx_t *ctx);

static void phase_arrive(ident_ctx_t *ctx)
{
    ctx->step = 1;

    for (int i = 0; i < N_BLOCKS; i++) {
        int hue = i < BLOCKHAUS_HUE_COUNT ? i : (i - BLOCKHAUS_HUE_COUNT);
        blockhaus_color_t c = blockhaus_active(hue);
        lv_obj_set_style_bg_color(ctx->blocks[i], lv_color_hex(c), 0);

        animate_block_spiral(&ctx->moves[i], 900, i * 90);
    }

    lv_timer_t *tm = lv_timer_create([](lv_timer_t *t) {
        lv_timer_del(t);
        phase_spectrum((ident_ctx_t *)lv_timer_get_user_data(t));
    }, 1400, ctx);
    lv_timer_set_repeat_count(tm, 1);
}

static void phase_spectrum(ident_ctx_t *ctx)
{
    ctx->step = 2;

    lv_obj_t *group = blockhaus_spectrum_create(ctx->scr, 14, 4);
    lv_obj_set_pos(group, -200, 195);
    ctx->spectrum_group = group;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        ident_ctx_t *c = (ident_ctx_t *)var;
        if (c->spectrum_group) lv_obj_set_x(c->spectrum_group, (lv_coord_t)v);
    });
    lv_anim_set_values(&a, -200, 320);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_delay(&a, 200);
    lv_anim_set_user_data(&a, ctx);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *anim) {
        ident_ctx_t *c = (ident_ctx_t *)lv_anim_get_user_data(anim);
        lv_timer_t *tm = lv_timer_create([](lv_timer_t *t) {
            lv_timer_del(t);
            phase_wordmark((ident_ctx_t *)lv_timer_get_user_data(t));
        }, 400, c);
        lv_timer_set_repeat_count(tm, 1);
    });
    lv_anim_start(&a);
}

static void phase_wordmark(ident_ctx_t *ctx)
{
    ctx->step = 3;

    const lv_font_t *font = blockhaus_font_mono(40);
    if (!font) font = &lv_font_montserrat_48;

    ctx->wordmark = lv_label_create(ctx->scr);
    lv_label_set_text(ctx->wordmark, "bp-esp");
    lv_obj_set_style_text_font(ctx->wordmark, font, 0);
    lv_obj_set_style_text_color(ctx->wordmark, lv_color_hex(0xE8E4DC), 0);
    lv_obj_align(ctx->wordmark, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_opa(ctx->wordmark, LV_OPA_0, 0);

    ctx->subtitle = lv_label_create(ctx->scr);
    lv_label_set_text(ctx->subtitle, "cheap yellow display");
    lv_obj_set_style_text_font(ctx->subtitle, blockhaus_font_mono(14), 0);
    lv_obj_set_style_text_color(ctx->subtitle, lv_color_hex(0x5A7D9A), 0);
    lv_obj_align(ctx->subtitle, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_opa(ctx->subtitle, LV_OPA_0, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->wordmark);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
    });
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_delay(&a, 200);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->subtitle);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
    });
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_delay(&a, 500);
    lv_anim_start(&a);

    lv_timer_t *tm = lv_timer_create([](lv_timer_t *t) {
        lv_timer_del(t);
        phase_finish((ident_ctx_t *)lv_timer_get_user_data(t));
    }, 2500, ctx);
    lv_timer_set_repeat_count(tm, 1);
}

static void phase_finish(ident_ctx_t *ctx)
{
    ctx->step = 4;

    if (ctx->spectrum_group) lv_obj_delete(ctx->spectrum_group);
    ctx->spectrum_group = NULL;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->blocks);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_t **blk = (lv_obj_t **)var;
        for (int i = 0; i < N_BLOCKS; i++) {
            if (blk[i]) lv_obj_set_style_opa(blk[i], (lv_opa_t)v, 0);
        }
    });
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_duration(&a, 500);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *) {
        ident_ctx_t *c = s_ctx;
        if (!c) return;
        for (int i = 0; i < N_BLOCKS; i++) {
            if (c->blocks[i]) lv_obj_delete(c->blocks[i]);
        }
        if (c->wordmark) lv_obj_delete(c->wordmark);
        if (c->subtitle) lv_obj_delete(c->subtitle);

        blockhaus_ident_done_cb_t done = c->done;
        void *ud = c->user_data;
        free(c);
        s_ctx = NULL;
        if (done) done(ud);
    });
    lv_anim_start(&a);
}

/* ── Public API ── */

void blockhaus_ident_show(lv_obj_t *scr, const char *name, blockhaus_ident_done_cb_t done, void *user_data)
{
    if (s_ctx) return;
    (void)name;

    ident_ctx_t *ctx = (ident_ctx_t *)malloc(sizeof(ident_ctx_t));
    if (!ctx) { if (done) done(user_data); return; }

    s_ctx = ctx;
    ctx->scr = scr;
    ctx->done = done;
    ctx->user_data = user_data;
    ctx->step = 0;
    ctx->spectrum_group = NULL;
    ctx->wordmark = NULL;
    ctx->subtitle = NULL;

    lv_obj_set_style_bg_color(scr, lv_color_hex(blockhaus_bg()), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < N_BLOCKS; i++) {
        lv_obj_t *b = lv_obj_create(scr);
        lv_obj_remove_style_all(b);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_radius(b, BLOCKHAUS_CORNER, 0);
        lv_obj_set_size(b, 20, 20);

        float dx = (float)s_die5[i].x - SPIRAL_CX;
        float dy = (float)s_die5[i].y - SPIRAL_CY;
        float a1 = atan2f(dy, dx);
        float r1 = sqrtf(dx * dx + dy * dy);
        int   dir = (i & 1) ? -1 : 1;
        float a0 = a1 - (float)dir * SPIRAL_SWEEP * SPIRAL_TURNS;

        float x0 = SPIRAL_CX + SPIRAL_R0 * cosf(a0);
        float y0 = SPIRAL_CY + SPIRAL_R0 * sinf(a0);

        lv_obj_set_pos(b, (lv_coord_t)x0, (lv_coord_t)y0);
        lv_obj_set_style_bg_color(b, lv_color_hex(blockhaus_active(BLOCKHAUS_HUE_NEUTRAL)), 0);
        ctx->blocks[i] = b;

        ctx->moves[i].block = b;
        ctx->moves[i].a0 = a0;
        ctx->moves[i].a1 = a1;
        ctx->moves[i].r0 = SPIRAL_R0;
        ctx->moves[i].r1 = r1;
    }

    lv_timer_t *tm = lv_timer_create([](lv_timer_t *t) {
        lv_timer_del(t);
        phase_arrive((ident_ctx_t *)lv_timer_get_user_data(t));
    }, 300, ctx);
    lv_timer_set_repeat_count(tm, 1);
}

/* ── Signal strip ── */

struct blockhaus_signal_strip_t {
    lv_obj_t *container;
    lv_obj_t **blocks;
    int count;
    int hue;
    int position;
    lv_timer_t *timer;
};

static void strip_cb(lv_timer_t *tm)
{
    blockhaus_signal_strip_t *s = (blockhaus_signal_strip_t *)lv_timer_get_user_data(tm);
    if (!s || !s->blocks) return;
    for (int i = 0; i < s->count; i++) {
        int d = abs(i - s->position);
        blockhaus_color_t c;
        if (d == 0) {
            c = blockhaus_active(s->hue);
        } else if (d == 1) {
            uint32_t base = blockhaus_active(s->hue);
            uint8_t r = ((base >> 16) & 0xFF) * 40 / 100;
            uint8_t g = ((base >> 8) & 0xFF) * 40 / 100;
            uint8_t b = (base & 0xFF) * 40 / 100;
            c = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        } else {
            c = blockhaus_resting(s->hue);
        }
        lv_obj_set_style_bg_color(s->blocks[i], lv_color_hex(c), 0);
    }
    s->position = (s->position + 1) % s->count;
}

blockhaus_signal_strip_handle_t blockhaus_signal_strip_create(lv_obj_t *parent, int n_blocks, int hue)
{
    blockhaus_signal_strip_t *s = (blockhaus_signal_strip_t *)malloc(sizeof(blockhaus_signal_strip_t));
    if (!s) return NULL;
    s->count = n_blocks;
    s->hue = hue;
    s->position = 0;

    s->container = lv_obj_create(parent);
    lv_obj_remove_style_all(s->container);
    lv_obj_set_size(s->container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s->container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s->container, 3, 0);
    lv_obj_set_style_pad_all(s->container, 0, 0);
    lv_obj_set_style_border_width(s->container, 0, 0);
    lv_obj_set_style_bg_opa(s->container, LV_OPA_TRANSP, 0);

    s->blocks = (lv_obj_t **)malloc(sizeof(lv_obj_t *) * (size_t)n_blocks);
    if (!s->blocks) { free(s); return NULL; }

    for (int i = 0; i < n_blocks; i++) {
        lv_obj_t *b = lv_obj_create(s->container);
        lv_obj_remove_style_all(b);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_radius(b, BLOCKHAUS_CORNER, 0);
        lv_obj_set_size(b, 12, 18);
        lv_obj_set_style_bg_color(b, lv_color_hex(blockhaus_resting(hue)), 0);
        s->blocks[i] = b;
    }

    s->timer = lv_timer_create(strip_cb, 100, s);
    return s;
}

void blockhaus_signal_strip_set_signal(blockhaus_signal_strip_handle_t strip, int sig)
{
    if (!strip) return;
    if (sig == BLOCKHAUS_SIGNAL_IDLE && strip->timer) {
        lv_timer_del(strip->timer);
        strip->timer = NULL;
        for (int i = 0; i < strip->count; i++)
            lv_obj_set_style_bg_color(strip->blocks[i], lv_color_hex(blockhaus_resting(strip->hue)), 0);
    } else if (sig != BLOCKHAUS_SIGNAL_IDLE && !strip->timer) {
        strip->position = 0;
        strip->timer = lv_timer_create(strip_cb, 100, strip);
    }
}

void blockhaus_signal_strip_set_blocks(blockhaus_signal_strip_handle_t strip, int n)
{
    if (!strip || n == strip->count) return;
    while (strip->count < n) {
        lv_obj_t *b = lv_obj_create(strip->container);
        lv_obj_remove_style_all(b);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_set_style_radius(b, BLOCKHAUS_CORNER, 0);
        lv_obj_set_size(b, 12, 18);
        lv_obj_set_style_bg_color(b, lv_color_hex(blockhaus_resting(strip->hue)), 0);
        lv_obj_t **newb = (lv_obj_t **)realloc(strip->blocks, sizeof(lv_obj_t *) * (size_t)(strip->count + 1));
        if (!newb) break;
        strip->blocks = newb;
        strip->blocks[strip->count++] = b;
    }
    while (strip->count > n && strip->count > 0) {
        strip->count--;
        lv_obj_delete(strip->blocks[strip->count]);
    }
}

void blockhaus_signal_strip_destroy(blockhaus_signal_strip_handle_t strip)
{
    if (!strip) return;
    if (strip->timer) lv_timer_del(strip->timer);
    free(strip->blocks);
    lv_obj_delete(strip->container);
    free(strip);
}