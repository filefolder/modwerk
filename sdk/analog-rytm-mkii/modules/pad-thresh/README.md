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

Each step is two encoder counts. After the first such turn, a full-screen log shows for about 10 s after every turn and every key or pad event the panel driver posts (`0x4008232c` for pad strikes and pressure, `0x40082492` for key presses and releases, pads included). The top line shows T, C and P (pad pressure events counted, not logged). Below it are the last 8 events, newest at the top:

| Column | Shows |
| --- | --- |
| K or S | a key event, or a pad strike |
| 2 hex digits | the key or pad code |
| 2 hex digits | a key's flags (bit 0: pressed), or a strike's velocity (0 means stock dropped it) |
| 3 digits | a strike's force byte, as the pad sent it |
| 4 digits | panel ticks (probably ms) since the event before |

PT03 showed that a light tap which toggles a mute posts no strike at all: the panel drops it. This log shows whether a strike comes before or after its pad's press, which decides whether a press without a strike can play at once.

## Light taps

PT04 on the unit (17 November 2026, reported): a firm hit posts the pad's key press (keys `0x90`-`0x9b` are pads 0-11) and its strike in the same millisecond, press first; a light tap posts a press and, 9-14 ms later, a release, and no strike. Hits already reach full velocity (255) at a force of about 103.

**FUNC + turn F** sets a light tap's velocity, 0-248 in steps of 8 (V on the log's top line; 0, the default, is off). When on, a pad press with no strike 3 ms later gets one at that velocity: the record the panel driver builds (type 3, the pad's strike code from `*0x40285968`, the velocity, the time stamp at `0x8000fe54`), posted to the driver's queue (`*0x40285980`) through `0x400013b2`, from the panel task (`0x40081944`, about every millisecond). It shows in the log as L.

## Safety

Nothing is written unless the address holds the stock value 10 at the first turn. The change lives in RAM: a power cycle restores stock. FUNC + turn H is the only gesture taken; every other key and turn reaches the OS untouched.

## Build

```sh
MODWERK_CROSS=m68k-linux-gnu- node scripts/link-image.mjs --stock /path/to/Analog-Rytm_MKII_OS1.74.syx \
  --version PT05 --out pad-thresh.syx \
  --elemod vendor/elekloader/catalog/core-ar2-0.7.elemod --module analog-rytm-mkii:pad-thresh
```

Send `pad-thresh.syx` with Elektron Transfer over USB and press YES on the unit; sending the stock `.syx` the same way restores stock.
