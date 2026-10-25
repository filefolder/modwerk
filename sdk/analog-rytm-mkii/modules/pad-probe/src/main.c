// SPDX-License-Identifier: GPL-3.0-or-later
// Pad Probe for the Analog Rytm MKII: a throwaway diagnostic, never a release.
//
// Question it answers: does a pad hit carry its velocity or pressure in the
// ev_key event, and where? On every pad press (key ids 0x90-0x9b) it copies
// the first WORDS words of the event and shows them on the screen as hex:
//
//   row 0      last key id seen (any key), its flags, pad hits since reset
//   rows 1-3   the words of the LAST pad event, 3 words a row
//   a line
//   rows 5-7   VARIED: the bits that changed against the FIRST pad event
//
// Tap one pad lightly, then hard: the words that differ are the candidates for
// the velocity. Press NO to reset the FIRST/VARIED baseline. The overlay shows
// for about 10 s after each pad hit. The mod consumes nothing: every event
// still reaches the OS, so the pads, mutes and sounds behave as on stock.
//
// Stock routines are reached at fixed addresses given for OS 1.74 by
// build.json's cflags.

typedef void (*fillrect_t)(void *bmp, int x0, int y0, int x1, int y1, int colour);
#define FILLRECT ((fillrect_t)FILLRECT_AT)

#define KEY_PAD_FIRST 0x90
#define KEY_PAD_LAST 0x9b
#define KEY_NO 0x45
#define KEY_PRESSED 1          // ev_key flags: 1 pressed
#define WORDS 8                // words of the event kept, from event + 0
#define SHOW_TICKS 200         // about 10 s of the ~20 Hz tick

static unsigned last[WORDS];   // the last pad event
static unsigned first[WORDS];  // the first pad event since reset
static unsigned varied[WORDS]; // bits that differ from `first` in any pad event since
static unsigned hits;          // pad presses since reset
static unsigned any_key;       // last key id of any kind
static unsigned any_flags;
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

static void reset(void)
{
    int i;
    for (i = 0; i < WORDS; i++)
        last[i] = first[i] = varied[i] = 0;
    hits = 0;
}

// ev_key: record the event of every pad press, consume nothing.
int pad_probe_key(void *brain, char *ev)
{
    unsigned key = *(unsigned *)(ev + 12), flags = *(unsigned *)(ev + 16);
    const volatile unsigned *w = (const volatile unsigned *)ev;
    int i;
    (void)brain;
    any_key = key;
    any_flags = flags;
    if (key == KEY_NO && (flags & KEY_PRESSED)) {
        reset();
        show_ticks = SHOW_TICKS;
        redraw = 1;
        return 0;
    }
    if (key < KEY_PAD_FIRST || key > KEY_PAD_LAST || !(flags & KEY_PRESSED)) {
        redraw = 1;   // the header still follows every key while the overlay is up
        return 0;
    }
    for (i = 0; i < WORDS; i++)
        last[i] = w[i];
    if (!hits)
        for (i = 0; i < WORDS; i++)
            first[i] = last[i];
    for (i = 0; i < WORDS; i++)
        varied[i] |= last[i] ^ first[i];
    hits++;
    show_ticks = SHOW_TICKS;
    redraw = 1;
    return 0;
}

// ev_tick: ask for a redraw when something changed, and when the overlay ends.
void pad_probe_tick(char *ctrl)
{
    if (redraw) {
        redraw = 0;
        ctrl[0x20] = 1;
    }
    if (show_ticks && !--show_ticks)
        ctrl[0x20] = 1;
}

// ev_draw: the overlay.
void pad_probe_draw(void *bmp, void *ctrl)
{
    (void)ctrl;
    if (!show_ticks)
        return;
    FILLRECT(bmp, 0, 0, 127, 63, 0);
    hex(bmp, any_key, 2, 2, 62);
    hex(bmp, any_flags, 2, 14, 62);
    hex(bmp, hits, 4, 30, 62);
    words_row(bmp, last, 0, 52);
    words_row(bmp, last, 3, 45);
    words_row(bmp, last, 6, 38);
    FILLRECT(bmp, 0, 33, 127, 33, 1);
    words_row(bmp, varied, 0, 29);
    words_row(bmp, varied, 3, 22);
    words_row(bmp, varied, 6, 15);
}
