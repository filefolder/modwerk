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
// After the first such turn, a box in the top right shows for about 10 s after
// every turn and every strike:  T, C (curve), F (the last strike's force),
// V (its velocity, 0-255, 0 = dropped) and N (strikes counted, 0-99).
// A strike that never reaches the OS changes none of F, V and N.
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
static unsigned seen;          // pt_count at the last redraw
static int show_ticks;
static int redraw;

// 3 x 5 glyphs, the top row first, bit 2 the left pixel: 0-9, then T C F V N.
enum { G_T = 10, G_C, G_F, G_V, G_N };
static const unsigned char glyphs[15][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
    {7, 2, 2, 2, 2}, {7, 4, 4, 4, 7}, {7, 4, 6, 4, 4}, {5, 5, 5, 5, 2}, {5, 7, 7, 7, 5},
};

// y counts up from the bottom row (0); `top` is the glyph's top row.
static void glyph(void *bmp, int g, int x, int top)
{
    int r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 3; c++)
            if (glyphs[g][r] & (4 >> c))
                FILLRECT(bmp, x + c, top - r, x + c, top - r, 1);
}

// A label and up to three digits, right-aligned.
static void row(void *bmp, int label, unsigned v, int top)
{
    int x = 120;
    glyph(bmp, label, 102, top);
    do {
        glyph(bmp, (int)(v % 10), x, top);
        v /= 10;
        x -= 4;
    } while (v);
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
    if (armed && pt_count != seen) {
        seen = pt_count;
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

// ev_draw: the box. T shows 0 and a cross beside it if the stock value was not found.
void pad_thresh_draw(void *bmp, void *ctrl)
{
    int i;
    (void)ctrl;
    if (!show_ticks)
        return;
    FILLRECT(bmp, 98, 28, 127, 63, 1);
    FILLRECT(bmp, 99, 29, 126, 62, 0);
    row(bmp, G_T, state > 0 ? (unsigned)THRESH : 0, 60);
    if (state < 0)
        for (i = 0; i < 5; i++) {
            FILLRECT(bmp, 108 + i, 60 - i, 108 + i, 60 - i, 1);
            FILLRECT(bmp, 112 - i, 60 - i, 112 - i, 60 - i, 1);
        }
    row(bmp, G_C, (unsigned)pt_curve, 53);
    row(bmp, G_F, pt_last_force, 46);
    row(bmp, G_V, pt_last_vel, 39);
    row(bmp, G_N, pt_count % 100, 32);
}
