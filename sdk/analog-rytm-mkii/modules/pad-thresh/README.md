# Pad Thresh

A throwaway experiment for the Analog Rytm MKII by @filefolder. Version 0.1.0-experimental, for OS 1.74 with core-ar2 0.3. Not for release.

Stock ignores a pad strike whose force byte (0-255) is at or below a threshold, 10, kept at `0x40285624` ([rytm-emu notes](https://github.com/trigcondition/rytm-emu/blob/main/docs/RYTM-PORT.md), measured in the emulator, not confirmed on a unit). This mod shows the memory there and changes the threshold live, to find out whether it is what makes the pads need a hard strike.

## Use

Hold **FUNC** and turn **encoder H**. A full-screen overlay shows for about 10 s after each turn, and each step is two encoder counts. The overlay reads, in hex:

- **Top line:** the value's width in bytes (1, 2 or 4; 0 means the stock value was not found, so nothing is written), the value found at the first turn, and the value now in memory.
- **Rows 1-3:** the 8 words at `0x40285600`, 3 a row (the pad map is near `0x40285608`).
- **A line, then rows 5-7:** the 8 words at `0x40285620`. The threshold is at the start of the second word.

Lower the value (try 1, 2, 4), tap the pads lightly and say how it feels. The range is 0 to 40.

## Safety

Nothing is written until the first turn finds the stock value, 10, at the address as a 1, 2 or 4 byte number. Anything else leaves the width at 0 and writes nothing. The change lives in RAM: a power cycle restores stock. FUNC + turn H is the only gesture taken; every other key and turn reaches the OS untouched.
