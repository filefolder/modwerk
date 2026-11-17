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
// So a strike plays only when force > T + about 5 (v must reach 16), and the
// squared curve keeps light strikes quiet. Lowering T lets lighter strikes
// through and makes every strike a little louder.
//
// FUNC + turn encoder H changes T in RAM (0 to 40, one step every two
// encoder counts) and shows a small box with T for about 3 s after each turn.
// T is written only if the long at THRESH_AT held the stock 10 at the first
// turn, so another OS build is never written to. A power cycle restores stock.

typedef void (*fillrect_t)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef unsigned (*is_key_held_t)(int key);
#define FILLRECT ((fillrect_t)FILLRECT_AT)
#define IS_KEY_HELD ((is_key_held_t)IS_KEY_HELD_AT)
#define THRESH (*(volatile int *)THRESH_AT)

#define KEY_FUNC 0x80
#define ENC_H 7                // ev_enc on the Rytm: encoders 0-7 are A-H
#define STOCK 10               // the stock threshold
#define MAX_VALUE 40
#define COUNTS_PER_STEP 2
#define SHOW_TICKS 60          // about 3 s of the ~20 Hz tick

static int state;              // 0 = not checked, 1 = stock value found, -1 = not found
static int acc;                // encoder counts not yet a step
static int show_ticks;
static int redraw;

// 3 x 5 digits, the top row first, bit 2 the left pixel.
static const unsigned char glyphs[10][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
    {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

// A digit at scale s; y counts up from the bottom row (0), `top` is the glyph's top row.
static void digit(void *bmp, int d, int x, int top, int s, int colour)
{
    int r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 3; c++)
            if (glyphs[d][r] & (4 >> c))
                FILLRECT(bmp, x + c * s, top - r * s - (s - 1), x + c * s + s - 1, top - r * s, colour);
}

// ev_enc: FUNC + turn H changes the threshold; every other turn is left alone.
int pad_thresh_enc(void *brain, char *ev)
{
    int enc = *(int *)(ev + 12), delta = *(int *)(ev + 16), steps, v;
    (void)brain;
    if (enc != ENC_H || !IS_KEY_HELD(KEY_FUNC))
        return 0;
    if (!state)
        state = THRESH == STOCK ? 1 : -1;
    show_ticks = SHOW_TICKS;
    redraw = 1;
    if (state > 0) {
        acc += delta;
        steps = acc / COUNTS_PER_STEP;
        acc -= steps * COUNTS_PER_STEP;
        if (steps) {
            v = THRESH + steps;
            if (v < 0) v = 0;
            if (v > MAX_VALUE) v = MAX_VALUE;
            THRESH = v;
        }
    }
    return 1;
}

// ev_tick: ask for a redraw after a turn, and when the box goes.
void pad_thresh_tick(char *ctrl)
{
    if (redraw) {
        redraw = 0;
        ctrl[0x20] = 1;
    }
    if (show_ticks && !--show_ticks)
        ctrl[0x20] = 1;
}

// ev_draw: a box in the top right corner with T, or a cross if the stock value was not found.
void pad_thresh_draw(void *bmp, void *ctrl)
{
    int t;
    (void)ctrl;
    if (!show_ticks)
        return;
    FILLRECT(bmp, 98, 44, 127, 63, 1);
    FILLRECT(bmp, 99, 45, 126, 62, 0);
    if (state < 0) {
        for (t = 0; t < 12; t++) {
            FILLRECT(bmp, 107 + t, 48 + t, 107 + t, 48 + t, 1);
            FILLRECT(bmp, 118 - t, 48 + t, 118 - t, 48 + t, 1);
        }
        return;
    }
    t = THRESH;
    digit(bmp, t / 10 % 10, 103, 60, 3, 1);
    digit(bmp, t % 10, 114, 60, 3, 1);
}
