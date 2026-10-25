# Pad Probe

A throwaway diagnostic for the Analog Rytm MKII by @filefolder. Version 0.1.0-experimental, for OS 1.74 with core-ar2 0.3. Not for release.

It answers one question: does a pad press carry its velocity or pressure inside the `ev_key` event, and in which word? Press a pad and a full-screen overlay shows, for about 10 s:

- **Top line:** the last key id seen (any key), its flags, and the number of pad presses since reset, all in hex.
- **Rows 1-3:** the first 8 words (32 bytes) of the last pad event, 3 words a row, in hex.
- **Rows 5-7 (below the line):** the bits that differed from the first pad press since reset, in the same layout.

Tap a pad lightly, then hard. Words whose lower rows light up between the two taps are the candidates for the velocity. Press NO to reset the baseline.

The mod consumes no event: pads, mutes and sounds behave as on stock. It takes no key gesture and no stock screen, other than drawing the overlay.
