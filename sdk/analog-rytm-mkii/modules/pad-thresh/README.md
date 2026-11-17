# Pad Thresh

A throwaway experiment for the Analog Rytm MKII by @filefolder. Version 0.1.0-experimental, for OS 1.74 on the Modwerk base (core-ar2 0.7). Not for release.

## What stock does

OS 1.74 turns a pad strike's force byte (0-255) into a velocity at `0x400821fa` and posts it as a strike event. It reads a threshold T, a long at `0x40285984` (stock 10, from the initialized data: its only setter, `0x40081bc0`, is never called), then:

- `v = (force - T) * 11 / 4`, then `v = v * 255 / (255 - T)`, at most 255
- `velocity = v * v * 258 >> 16`, and a strike whose velocity is 0 is dropped

The square makes T less a threshold than a dead zone: a strike whose force is within about 5 of T is dropped, and one below T plays softly. The setter also sends T to the panel, so the panel may gate strikes on its own value as well. Mutes and pad selection use the pad's key press, a separate event, so they work on taps that never make a strike.

The earlier version of this experiment looked for a pad map near `0x40285608`, from rytm-emu's notes. In OS 1.74 that address holds a table of string pointers, so it never found anything (and never wrote).

## Use

- **FUNC + turn H:** T, 0 to 40 (stock 10), written only if the address held 10 at the first turn.
- **FUNC + turn G:** the curve. 0 is stock's, 1 linear (the scaled force as it is), 2 soft (`255 * (2x - x^2)`). With 1 and 2, nothing at or below T plays.

Each step is two encoder counts. After the first such turn, a box in the top right shows for about 10 s after every turn and every strike:

| Row | Shows |
| --- | --- |
| T | the threshold (0 with a cross: the stock value was not found, nothing is written) |
| C | the curve |
| F | the last strike's force byte, as the pad sent it |
| V | its velocity, 0-255; 0 means stock dropped it |
| N | strikes counted, 0-99 |

A tap that changes none of F, V and N never reached the OS as a strike: the panel dropped it.

## Safety

Nothing is written unless the address holds the stock value 10 at the first turn. The change lives in RAM: a power cycle restores stock. FUNC + turn H is the only gesture taken; every other key and turn reaches the OS untouched.

## Build

```sh
MODWERK_CROSS=m68k-linux-gnu- node scripts/link-image.mjs --stock /path/to/Analog-Rytm_MKII_OS1.74.syx \
  --version PT03 --out pad-thresh.syx \
  --elemod vendor/elekloader/catalog/core-ar2-0.7.elemod --module analog-rytm-mkii:pad-thresh
```

Send `pad-thresh.syx` with Elektron Transfer over USB and press YES on the unit; sending the stock `.syx` the same way restores stock.
