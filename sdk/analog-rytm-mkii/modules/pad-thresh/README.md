# Pad Thresh

A throwaway experiment for the Analog Rytm MKII by @filefolder. Version 0.1.0-experimental, for OS 1.74 on the Modwerk base (core-ar2 0.7). Not for release.

## What stock does

OS 1.74 turns a pad strike's force byte (0-255) into a velocity at `0x400821fa`. It reads a threshold T, a long at `0x40285984` (stock 10, from the initialized data: its only setter, `0x40081bc0`, is never called), then:

- `v = (force - T) * 11 / 4`, then `v = v * 255 / (255 - T)`, at most 255
- `velocity = v * v * 258 >> 16`, and a strike whose velocity is 0 is dropped

So a strike plays only when its force is more than about T + 5, and the squared curve keeps light strikes quiet. Lowering T lets lighter strikes through and makes every strike a little louder. The setter also sends T to the panel, so the panel may gate strikes on its own value as well: only a unit can tell.

The earlier version of this experiment looked for a pad map near `0x40285608`, from rytm-emu's notes. In OS 1.74 that address holds a table of string pointers, so it never found anything (and never wrote).

## Use

Hold **FUNC** and turn **encoder H**. A small box in the top right corner shows T for about 3 s after each turn. Each step is two encoder counts, from 0 to 40. A cross in the box means the stock value 10 was not at the address, and nothing is written.

Lower T (try 6, 3, 0), tap the pads lightly and say how it feels.

## Safety

Nothing is written unless the address holds the stock value 10 at the first turn. The change lives in RAM: a power cycle restores stock. FUNC + turn H is the only gesture taken; every other key and turn reaches the OS untouched.

## Build

```sh
MODWERK_CROSS=m68k-linux-gnu- node scripts/link-image.mjs --stock /path/to/Analog-Rytm_MKII_OS1.74.syx \
  --version PT02 --out pad-thresh.syx \
  --elemod vendor/elekloader/catalog/core-ar2-0.7.elemod --module analog-rytm-mkii:pad-thresh
```

Send `pad-thresh.syx` with Elektron Transfer over USB and press YES on the unit; sending the stock `.syx` the same way restores stock.
