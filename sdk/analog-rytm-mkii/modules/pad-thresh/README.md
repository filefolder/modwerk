# Pad Thresh

A throwaway experiment for the Analog Rytm MKII by @filefolder. Version 0.1.0-experimental, for OS 1.74 with core-ar2 0.3. Not for release.

Stock ignores a pad strike whose force byte (0-255) is at or below a threshold, 10. [rytm-emu's notes](https://github.com/trigcondition/rytm-emu/blob/main/docs/RYTM-PORT.md) (measured in the emulator, mixing OS 1.73 and 1.74) put it at `0x40285624`, `0x1c` bytes after the pad map at `0x40285608` (the identity: the bytes 0 to 11). OS 1.74's data may have moved from there, so at the first turn the mod looks for the pad map within `0x2000` bytes of that address, takes the threshold at the same distance from it, and uses it only if it holds the stock value, 10, as a 1, 2 or 4 byte number.

## Use

Hold **FUNC** and turn **encoder H**. A full-screen overlay shows for about 10 s after each turn, and each step is two encoder counts, in the range 0 to 40. The overlay reads, in hex:

- **Top line:** the value's width in bytes (1, 2 or 4; 0 means nothing was found, and nothing is ever written), the value found at the first turn, the value now in memory, and the low 16 bits of the threshold's address.
- **Rows 1-3:** 8 words from 8 bytes before the pad map, 3 a row.
- **A line, then rows 5-7:** the next 8 words. The threshold is at the start of this block's second word.

Lower the value (try 1, 2, 4), tap the pads lightly and say how it feels.

## Safety

Nothing is written unless the pad map is found and the stock value sits at its offset. The change lives in RAM: a power cycle restores stock. FUNC + turn H is the only gesture taken; every other key and turn reaches the OS untouched.

## Build (elekloader)

This folder's `build.json` is Modwerk's; elekloader's builder reads `elekloader/mod.json`. `elekloader/build.sh` stages both and runs elekloader's build, lint and patch steps (needs Python 3.9+ and Debian's `m68k-linux-gnu` binutils and GCC):

```sh
git clone --branch core-ar2-v0.3 https://github.com/irpina/elekloader
# core-ar2-0.3.elemod: from https://github.com/irpina/elekloader/releases/tag/core-ar2-v0.3
sdk/analog-rytm-mkii/modules/pad-thresh/elekloader/build.sh \
    elekloader /path/to/Analog-Rytm_MKII_OS1.74.syx /path/to/core-ar2-0.3.elemod
```

It writes `pad-thresh-out/pad-thresh.syx`. Send it with Elektron Transfer over USB and press YES on the unit; sending the stock `.syx` the same way restores stock.
