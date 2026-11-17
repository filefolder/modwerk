// SPDX-License-Identifier: GPL-3.0-or-later
// Pad Thresh for the Analog Rytm MKII: a throwaway experiment, never a release.
//
// OS 1.74 turns a pad strike's force byte (0-255) into a velocity at 0x400821fa.
// It reads a threshold T, a long at THRESH_AT (stock 10, set once from the
// initialized data: its only setter, 0x40081bc0, is never called), then:
//
//   v = (force - T) * 11 / 4;  v = v * 255 / (255 - T), at most 255
//   velocity = v * v * 258 >> 16;  a strike whose velocity is 0 is dropped
//
// The square makes a strike below T play softly too, and drops a strike whose
// force is within about 5 of T. hooks.s records each strike's force and
// velocity and can swap the curve.
//
//   FUNC + turn H   T in RAM, 0 to 40, written only if it held the stock 10
//   FUNC + turn G   the curve: 0 stock, 1 linear (force above T, scaled),
//                   2 soft (255 * (2x - x^2)); with 1 and 2 nothing at or
//                   below T plays
//
// hooks.s also logs every key event (pads included) and pad strike the panel
// driver posts. After the first such turn, a full-screen log shows for about
// 10 s after every turn and every logged event. Its top line: T, C and P (pad
// pressure events counted, 0-99, not logged). Then the last 8 events, newest
// at the top, each:
//   K or S       a key event, or a pad strike
//   2 hex digits the event's key or pad code
//   2 hex digits a key's flags (bit 0: pressed) or a strike's velocity
//   3 digits     a strike's force byte, as the pad sent it
//   4 digits     the panel ticks (probably ms) since the event before, at most 9999
// Everything lives in RAM: a power cycle restores stock.

typedef void (*fillrect_t)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef unsigned (*is_key_held_t)(int key);
#define FILLRECT ((fillrect_t)FILLRECT_AT)
#define IS_KEY_HELD ((is_key_held_t)IS_KEY_HELD_AT)
#define THRESH (*(volatile int *)THRESH_AT)

#define KEY_FUNC 0x80
#define ENC_G 6                // ev_enc on the Rytm: encoders 0-7 are A-H
#define ENC_H 7
#define STOCK 10               // the stock threshold
#define MAX_VALUE 40
#define CURVES 3
#define COUNTS_PER_STEP 2
#define SHOW_TICKS 200         // about 10 s of the ~20 Hz tick

// Written by hooks.s at every strike, and pt_curve read there.
volatile unsigned char pt_last_force, pt_last_vel;
volatile unsigned pt_count;
volatile int pt_curve;

static int state;              // 0 = not checked, 1 = stock value found, -1 = not found
static int armed;              // set by the first FUNC + G or H turn
static int acc;                // encoder counts not yet a step
static unsigned seen;          // log_seq at the last redraw
static int show_ticks;
static int redraw;

// 3 x 5 glyphs, the top row first, bit 2 the left pixel: 0-F, then T C P K S.
enum { G_T = 16, G_C, G_P, G_K, G_S };
static const unsigned char glyphs[21][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
    {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {7, 4, 4, 4, 7}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
    {7, 4, 6, 4, 4}, {7, 2, 2, 2, 2}, {7, 4, 4, 4, 7}, {6, 5, 6, 4, 4}, {5, 5, 6, 5, 5},
    {3, 4, 2, 1, 6},
};

#define TICKS (*(volatile unsigned *)0x42f92114)   // the panel task's counter, kept below 4096
#define LOG 8
struct entry { unsigned char kind, code, value, force; unsigned dt; };
static struct entry log_ring[LOG];
static unsigned log_head, log_n, last_tick, pressure, log_seq;

// Called from hooks.s for each event the panel driver posts.
void pt_log(char *ev)
{
    struct entry *e;
    unsigned now = TICKS & 0xfff;
    if (ev[0] == 2) {
        pressure++;
        log_seq++;
        return;
    }
    e = &log_ring[log_head];
    log_head = (log_head + 1) % LOG;
    if (log_n < LOG)
        log_n++;
    e->kind = (unsigned char)ev[0];
    e->code = (unsigned char)*(int *)(ev + 4);
    e->value = (unsigned char)ev[ev[0] == 3 ? 8 : 11];   // a strike's velocity byte, a key's flags long
    e->force = ev[0] == 3 ? pt_last_force : 0;
    e->dt = (now - last_tick) & 0xfff;
    last_tick = now;
    log_seq++;
}

// y counts up from the bottom row (0); `top` is the glyph's top row.
static void glyph(void *bmp, int g, int x, int top)
{
    int r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 3; c++)
            if (glyphs[g][r] & (4 >> c))
                FILLRECT(bmp, x + c, top - r, x + c, top - r, 1);
}

// n digits of v in base 10 or 16, the most significant first; returns the next x.
static int number(void *bmp, unsigned v, int n, int base, int x, int top)
{
    int i;
    unsigned d = 1;
    for (i = 1; i < n; i++)
        d *= (unsigned)base;
    for (; n; n--, d /= (unsigned)base, x += 4)
        glyph(bmp, (int)(v / d % (unsigned)base), x, top);
    return x;
}

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

// ev_enc: FUNC + turn H changes T, FUNC + turn G the curve; every other turn is left alone.
int pad_thresh_enc(void *brain, char *ev)
{
    int enc = *(int *)(ev + 12), delta = *(int *)(ev + 16), steps;
    (void)brain;
    if ((enc != ENC_H && enc != ENC_G) || !IS_KEY_HELD(KEY_FUNC))
        return 0;
    if (!state)
        state = THRESH == STOCK ? 1 : -1;
    armed = 1;
    show_ticks = SHOW_TICKS;
    redraw = 1;
    acc += delta;
    steps = acc / COUNTS_PER_STEP;
    acc -= steps * COUNTS_PER_STEP;
    if (!steps)
        return 1;
    if (enc == ENC_G)
        pt_curve = clamp(pt_curve + steps, 0, CURVES - 1);
    else if (state > 0)
        THRESH = clamp(THRESH + steps, 0, MAX_VALUE);
    return 1;
}

// ev_tick: ask for a redraw after a turn or a strike, and when the box goes.
void pad_thresh_tick(char *ctrl)
{
    if (armed && log_seq != seen) {
        seen = log_seq;
        show_ticks = SHOW_TICKS;
        redraw = 1;
    }
    if (redraw) {
        redraw = 0;
        ctrl[0x20] = 1;
    }
    if (show_ticks && !--show_ticks)
        ctrl[0x20] = 1;
}

// ev_draw: the log, full screen. T shows 0 and a cross if the stock value was not found.
void pad_thresh_draw(void *bmp, void *ctrl)
{
    int i, x, top;
    struct entry *e;
    (void)ctrl;
    if (!show_ticks)
        return;
    FILLRECT(bmp, 0, 0, 127, 63, 0);
    glyph(bmp, G_T, 2, 62);
    number(bmp, state > 0 ? (unsigned)THRESH : 0, 2, 10, 8, 62);
    if (state < 0)
        for (i = 0; i < 5; i++) {
            FILLRECT(bmp, 17 + i, 62 - i, 17 + i, 62 - i, 1);
            FILLRECT(bmp, 21 - i, 62 - i, 21 - i, 62 - i, 1);
        }
    glyph(bmp, G_C, 30, 62);
    number(bmp, (unsigned)pt_curve, 1, 10, 36, 62);
    glyph(bmp, G_P, 48, 62);
    number(bmp, pressure % 100, 2, 10, 54, 62);
    FILLRECT(bmp, 0, 55, 127, 55, 1);
    for (i = 0; i < (int)log_n; i++) {
        e = &log_ring[(log_head + LOG - 1 - (unsigned)i) % LOG];
        top = 52 - i * 7;
        glyph(bmp, e->kind == 3 ? G_S : G_K, 2, top);
        x = number(bmp, e->code, 2, 16, 10, top);
        x = number(bmp, e->value, 2, 16, x + 4, top);
        if (e->kind == 3)
            number(bmp, e->force, 3, 10, x + 4, top);
        number(bmp, e->dt > 9999 ? 9999 : e->dt, 4, 10, 60, top);
    }
}
