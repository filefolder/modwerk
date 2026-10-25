// SPDX-License-Identifier: GPL-3.0-or-later
// Pad Thresh for the Analog Rytm MKII: a throwaway experiment, never a release.
//
// Stock ignores a pad strike whose force byte is at or below a threshold, 10.
// rytm-emu's notes (docs/RYTM-PORT.md, measured in the emulator, mixing OS 1.73
// and 1.74) put it at 0x40285624, 0x1c bytes after the pad map at 0x40285608,
// which is the identity: the bytes 0, 1, 2 ... 11. OS 1.74's code is 1.73's
// moved, and its data may have moved by another amount, so this mod does not
// trust the address: at the first turn it looks for the pad map within
// SCAN bytes of where it was, takes the threshold at the same distance from
// it, and uses it only if it holds the stock value, 10, as a 1, 2 or 4 byte
// big-endian number.
//
// FUNC + turn encoder H opens a full-screen overlay (about 10 s after each
// turn) and changes the threshold, one step every two encoder counts:
//
//   top line   width of the value in bytes (1, 2 or 4; 0 = not found, nothing
//              is ever written), the value found at the first turn, the value
//              now in memory, and the low 16 bits of the threshold's address
//   rows 1-3   8 words from 8 bytes before the pad map, 3 words a row
//   a line
//   rows 5-7   the next 8 words (the threshold is 0x1c after the pad map: the
//              start of this block's 2nd word)
//
// The value lives in RAM: a power cycle restores stock. Nothing else is
// written. Firmware routines are reached at fixed addresses given for OS 1.74
// by build.json's cflags.

typedef void (*fillrect_t)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef unsigned (*is_key_held_t)(int key);
#define FILLRECT ((fillrect_t)FILLRECT_AT)
#define IS_KEY_HELD ((is_key_held_t)IS_KEY_HELD_AT)

#define THRESH_OFF (THRESH_AT - MAP_AT)   // the threshold's distance from the pad map
#define SCAN 0x2000            // how far from MAP_AT to look for the pad map
#define MAP_LEN 12             // pads 0..11
#define KEY_FUNC 0x80
#define ENC_H 7                // ev_enc on the Rytm: encoders 0-7 are A-H
#define STOCK 10               // the stock threshold
#define MAX_VALUE 40
#define COUNTS_PER_STEP 2
#define WORDS 8
#define SHOW_TICKS 200         // about 10 s of the ~20 Hz tick

static unsigned thresh;        // the threshold's address once found, else 0
static int width;              // 1, 2 or 4 bytes once found; 0 = not found, never write
static int detected;
static int orig;               // the value found at the first turn
static int acc;                // encoder counts not yet a step
static int show_ticks;
static int redraw;

// 3 x 5 hex digits, the top row first, bit 2 the left pixel.
static const unsigned char glyphs[16][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7},
    {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1},
    {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7}, {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6},
    {7, 4, 4, 4, 7}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7}, {7, 4, 6, 4, 4},
};

// y counts up from the bottom row (0); `top` is the glyph's top row.
static void digit(void *bmp, int d, int x, int top)
{
    int r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 3; c++)
            if (glyphs[d][r] & (4 >> c))
                FILLRECT(bmp, x + c, top - r, x + c, top - r, 1);
}

// n hex digits of v, the most significant first; returns the next x.
static int hex(void *bmp, unsigned v, int n, int x, int top)
{
    int i;
    for (i = n - 1; i >= 0; i--, x += 4)
        digit(bmp, (int)(v >> (4 * i) & 15), x, top);
    return x;
}

static void words_row(void *bmp, const unsigned *w, int from, int top)
{
    int i, x = 2;
    for (i = from; i < from + 3 && i < WORDS; i++)
        x = hex(bmp, w[i], 8, x, top) + 3;
}

static unsigned read_value(void)
{
    if (!thresh)
        return 0;
    switch (width) {
    case 1: return *(volatile unsigned char *)thresh;
    case 2: return *(volatile unsigned short *)thresh;
    case 4: return *(volatile unsigned *)thresh;
    }
    return 0;
}

static void write_value(unsigned v)
{
    if (!thresh)
        return;
    switch (width) {
    case 1: *(volatile unsigned char *)thresh = (unsigned char)v; break;
    case 2: *(volatile unsigned short *)thresh = (unsigned short)v; break;
    case 4: *(volatile unsigned *)thresh = v; break;
    }
}

// The width at `a` of the stock value, as a byte, a short or a long (0 if none).
// Wider reads only at addresses aligned for them.
static int stock_width(unsigned a)
{
    if (*(volatile unsigned char *)a == STOCK)
        return 1;
    if (!(a & 1) && *(volatile unsigned short *)a == STOCK)
        return 2;
    if (!(a & 3) && *(volatile unsigned *)a == STOCK)
        return 4;
    return 0;
}

static int is_map(unsigned a)
{
    int i;
    for (i = 0; i < MAP_LEN; i++)
        if (*(volatile unsigned char *)(a + i) != i)
            return 0;
    return 1;
}

// Find the pad map nearest MAP_AT, then the stock value at its offset.
static void detect(void)
{
    unsigned d, a;
    int w;
    detected = 1;
    for (d = 0; d <= SCAN && !thresh; d++) {
        a = MAP_AT + d;
        if (is_map(a) && (w = stock_width(a + THRESH_OFF))) {
            thresh = a + THRESH_OFF;
            width = w;
        } else if (d && is_map(a = MAP_AT - d) && (w = stock_width(a + THRESH_OFF))) {
            thresh = a + THRESH_OFF;
            width = w;
        }
    }
    orig = thresh ? STOCK : 0;
}

// ev_enc: FUNC + turn H changes the threshold; every other turn is left alone.
int pad_thresh_enc(void *brain, char *ev)
{
    int enc = *(int *)(ev + 12), delta = *(int *)(ev + 16), steps, v;
    (void)brain;
    if (enc != ENC_H || !IS_KEY_HELD(KEY_FUNC))
        return 0;
    if (!detected)
        detect();
    show_ticks = SHOW_TICKS;
    redraw = 1;
    if (width) {
        acc += delta;
        steps = acc / COUNTS_PER_STEP;
        acc -= steps * COUNTS_PER_STEP;
        if (steps) {
            v = (int)read_value() + steps;
            if (v < 0) v = 0;
            if (v > MAX_VALUE) v = MAX_VALUE;
            write_value((unsigned)v);
        }
    }
    return 1;
}

// ev_tick: ask for a redraw after a turn, and when the overlay ends.
void pad_thresh_tick(char *ctrl)
{
    if (redraw) {
        redraw = 0;
        ctrl[0x20] = 1;
    }
    if (show_ticks && !--show_ticks)
        ctrl[0x20] = 1;
}

// ev_draw: the overlay.
void pad_thresh_draw(void *bmp, void *ctrl)
{
    unsigned a[WORDS], b[WORDS], base;
    int i;
    (void)ctrl;
    if (!show_ticks)
        return;
    base = ((thresh ? thresh - THRESH_OFF : MAP_AT) - 8) & ~3u;   // word aligned
    for (i = 0; i < WORDS; i++) {
        a[i] = ((volatile unsigned *)base)[i];
        b[i] = ((volatile unsigned *)(base + 4 * WORDS))[i];
    }
    FILLRECT(bmp, 0, 0, 127, 63, 0);
    hex(bmp, (unsigned)width, 1, 2, 62);
    hex(bmp, (unsigned)orig, 2, 10, 62);
    hex(bmp, read_value(), 2, 22, 62);
    hex(bmp, thresh & 0xffff, 4, 34, 62);
    words_row(bmp, a, 0, 52);
    words_row(bmp, a, 3, 45);
    words_row(bmp, a, 6, 38);
    FILLRECT(bmp, 0, 33, 127, 33, 1);
    words_row(bmp, b, 0, 29);
    words_row(bmp, b, 3, 22);
    words_row(bmp, b, 6, 15);
}
