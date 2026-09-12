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

#define WM_NUM_GLYPHS 6
#define WM_STAGGER_MS 260            /* next glyph starts as the previous peaks */
#define WM_RISE_MS    260            /* fade in: black -> glyph colour */
#define WM_FALL_MS    320            /* fade out: glyph colour -> white */
#define WM_STEP_MS    33             /* marquee refresh tick */

typedef struct {
    lv_obj_t *block;
    float a0, a1;                    /* polar angle, radians */
    float r0, r1;                    /* radius, px */
} spiral_move_t;

typedef struct {
    lv_obj_t *scr;
    lv_obj_t *blocks[N_BLOCKS];
    lv_obj_t *spectrum_group;
    lv_obj_t *wmark_group;           /* flex row of the six glyph labels */
    lv_obj_t *glyphs[WM_NUM_GLYPHS]; /* one label per glyph of "bp-esp" */
    lv_timer_t *pulse_timer;
    uint32_t marquee_start;          /* lv_tick_get() at marquee start */
    spiral_move_t moves[N_BLOCKS];
    blockhaus_ident_done_cb_t done;
    void *user_data;
    int step;
} ident_ctx_t;

/* Die-5 targets are the block *centres*; lv_obj_set_pos uses the top-left, so
 * each is stored offset by half the 20px block size (i.e. centred on-screen).
 * The centre blue block therefore rests dead-centre (160,120). */
static const lv_point_t s_die5[N_BLOCKS] = {
    {100, 65}, {200, 65}, {150, 110}, {100, 155}, {200, 155},
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
        blockhaus_color_t c = blockhaus_spectrum_active(i);
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

/* ── Wordmark marquee ──
 * Six glyphs ("b", "p", "-", "e", "s", "p"), each its own label so every glyph
 * can hold a single solid colour. Five letters run through the spectrum in
 * left-to-right order (red, orange, yellow, green, blue); the dash takes the
 * palette's white. A single wave of colour travels left to right with each
 * glyph pulsing from black (off) up to its colour and on to white, and each
 * pulse overlaps its neighbours — b reaches its peak exactly as p begins.
 * Looping is avoided: the sweep is one-shot and ends with the word all white. */

static blockhaus_color_t mix_rgb(blockhaus_color_t a, blockhaus_color_t b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    int br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint8_t r = (uint8_t)(ar + (br - ar) * t);
    uint8_t g = (uint8_t)(ag + (bg - ag) * t);
    uint8_t bl = (uint8_t)(ab + (bb - ab) * t);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
}

static float smoothstep01(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static blockhaus_color_t wordmark_glyph_color(int g)
{
    switch (g) {
    case 0: return blockhaus_spectrum_active(0);             /* b  red */
    case 1: return blockhaus_spectrum_active(1);             /* p  orange */
    case 2: return blockhaus_active(BLOCKHAUS_HUE_NEUTRAL);  /* -  white */
    case 3: return blockhaus_spectrum_active(2);             /* e  yellow */
    case 4: return blockhaus_spectrum_active(3);             /* s  green */
    default: return blockhaus_spectrum_active(4);            /* p  blue */
    }
}

static bool build_wordmark(ident_ctx_t *ctx, const lv_font_t *font)
{
    static const char *s_glyph[WM_NUM_GLYPHS] = { "b", "p", "-", "e", "s", "p" };

    lv_obj_t *group = lv_obj_create(ctx->scr);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(group, 0, 0);
    lv_obj_set_style_pad_column(group, 0, 0);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);

    for (int g = 0; g < WM_NUM_GLYPHS; g++) {
        lv_obj_t *lbl = lv_label_create(group);
        lv_obj_set_style_pad_all(lbl, 0, 0);
        lv_obj_set_style_text_font(lbl, font, 0);
        lv_label_set_text(lbl, s_glyph[g]);
        lv_obj_set_style_text_color(lbl,
            lv_color_hex(blockhaus_resting(BLOCKHAUS_HUE_NEUTRAL)), 0);
        ctx->glyphs[g] = lbl;
    }

    ctx->wmark_group = group;

    /* Centre vertically halfway between the screen top (0) and the top edge of
     * the top two die-5 blocks (y=65) -> y=32. */
    int32_t lh = lv_font_get_line_height(font);
    lv_obj_align(group, LV_ALIGN_TOP_MID, 0, 32 - lh / 2);
    lv_obj_set_style_opa(group, LV_OPA_0, 0);
    return true;
}

static blockhaus_color_t wordmark_glyph_color_at(int g, uint32_t elapsed_ms)
{
    uint32_t start = (uint32_t)g * WM_STAGGER_MS;
    blockhaus_color_t black = blockhaus_resting(BLOCKHAUS_HUE_NEUTRAL);
    blockhaus_color_t white = blockhaus_active(BLOCKHAUS_HUE_NEUTRAL);

    if (elapsed_ms <= start) return black;

    uint32_t local = elapsed_ms - start;
    blockhaus_color_t target = wordmark_glyph_color(g);

    if (local < WM_RISE_MS) {
        float u = smoothstep01((float)local / (float)WM_RISE_MS);
        return mix_rgb(black, target, u);
    }
    if (local < WM_RISE_MS + WM_FALL_MS) {
        float u = smoothstep01((float)(local - WM_RISE_MS) / (float)WM_FALL_MS);
        return mix_rgb(target, white, u);
    }
    return white;
}

static void wordmark_marquee_cb(lv_timer_t *tm)
{
    ident_ctx_t *ctx = (ident_ctx_t *)lv_timer_get_user_data(tm);
    if (!ctx || !ctx->wmark_group) return;

    uint32_t elapsed = lv_tick_elaps(ctx->marquee_start);

    for (int g = 0; g < WM_NUM_GLYPHS; g++) {
        if (!ctx->glyphs[g]) continue;
        blockhaus_color_t c = wordmark_glyph_color_at(g, elapsed);
        lv_obj_set_style_text_color(ctx->glyphs[g], lv_color_hex(c), 0);
    }
}

static void bulk_marquee_start(ident_ctx_t *ctx)
{
    if (ctx->pulse_timer) return;
    ctx->marquee_start = lv_tick_get();
    ctx->pulse_timer = lv_timer_create(wordmark_marquee_cb, WM_STEP_MS, ctx);

    lv_timer_t *tm = lv_timer_create([](lv_timer_t *t) {
        lv_timer_del(t);
        phase_finish((ident_ctx_t *)lv_timer_get_user_data(t));
    }, WM_NUM_GLYPHS * WM_STAGGER_MS + WM_FALL_MS + 500, ctx);
    lv_timer_set_repeat_count(tm, 1);
}

static void phase_wordmark(ident_ctx_t *ctx)
{
    ctx->step = 3;

    const lv_font_t *font = blockhaus_font_mono(40);
    if (!font) font = &lv_font_montserrat_48;

    if (!build_wordmark(ctx, font)) {
        phase_finish(ctx);
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->wmark_group);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
    });
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_delay(&a, 200);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *anim) {
        ident_ctx_t *c = s_ctx;
        if (!c) return;
        bulk_marquee_start(c);
    });
    lv_anim_start(&a);
}

static void phase_finish(ident_ctx_t *ctx)
{
    ctx->step = 4;

    if (ctx->spectrum_group) lv_obj_delete(ctx->spectrum_group);
    ctx->spectrum_group = NULL;

    if (ctx->pulse_timer) { lv_timer_del(ctx->pulse_timer); ctx->pulse_timer = NULL; }

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
        if (c->wmark_group) lv_obj_delete(c->wmark_group);

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
    ctx->wmark_group = NULL;
    for (int g = 0; g < WM_NUM_GLYPHS; g++) ctx->glyphs[g] = NULL;
    ctx->pulse_timer = NULL;
    ctx->marquee_start = 0;

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
    int signal;
    blockhaus_pulse_handle_t pulse;
};

blockhaus_signal_strip_handle_t blockhaus_signal_strip_create(lv_obj_t *parent, int n_blocks, int hue)
{
    blockhaus_signal_strip_t *s = (blockhaus_signal_strip_t *)malloc(sizeof(blockhaus_signal_strip_t));
    if (!s) return NULL;
    s->count = n_blocks;
    s->hue = hue;
    s->signal = BLOCKHAUS_SIGNAL_IDLE;
    s->pulse = NULL;

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
        lv_obj_t *b = blockhaus_block_create(s->container, 12, 18);
        lv_obj_set_style_bg_color(b, lv_color_hex(blockhaus_resting(hue)), 0);
        s->blocks[i] = b;
    }

    return s;
}

void blockhaus_signal_strip_set_signal(blockhaus_signal_strip_handle_t strip, int sig)
{
    if (!strip) return;
    if (strip->pulse) { blockhaus_pulse_stop(&strip->pulse); }
    strip->signal = sig;

    if (sig == BLOCKHAUS_SIGNAL_IDLE) {
        for (int i = 0; i < strip->count; i++)
            lv_obj_set_style_bg_color(strip->blocks[i], lv_color_hex(blockhaus_resting(strip->hue)), 0);
        return;
    }

    /* Urgency (tempo) and colour are read from the same signal table the
     * single-block indicators use, so a strip and a block never disagree on
     * meaning. SUCCESS has period 0, so it holds rather than pulses. */
    int hue = blockhaus_signal_hue(strip->hue, sig);
    int period = blockhaus_signal_period(sig);
    if (period <= 0) {
        for (int i = 0; i < strip->count; i++)
            lv_obj_set_style_bg_color(strip->blocks[i], lv_color_hex(blockhaus_active(hue)), 0);
        return;
    }

    strip->pulse = blockhaus_pulse_start(strip->blocks, strip->count, hue, period,
                                         BLOCKHAUS_PULSE_LIGHTNESS);
}

void blockhaus_signal_strip_set_blocks(blockhaus_signal_strip_handle_t strip, int n)
{
    if (!strip || n == strip->count) return;
    if (strip->pulse) blockhaus_pulse_stop(&strip->pulse);

    while (strip->count < n) {
        lv_obj_t *b = blockhaus_block_create(strip->container, 12, 18);
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

    blockhaus_signal_strip_set_signal(strip, strip->signal);
}

void blockhaus_signal_strip_destroy(blockhaus_signal_strip_handle_t strip)
{
    if (!strip) return;
    if (strip->pulse) blockhaus_pulse_stop(&strip->pulse);
    free(strip->blocks);
    lv_obj_delete(strip->container);
    free(strip);
}