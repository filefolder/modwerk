# Pad Thresh testing

Evidence tier: **none**. A diagnostic prototype, never submitted for release.

## Build

- 17 November 2026, 0.4.0-experimental: build PT05, sha256 `ee5fa30779e8b7decab319c80f92167746c74462259d25c21a7812e0d8b15757`, compiled and linked the same way; all five sites call their hooks in the built image. `main.c` compiles with `-Wall -Wextra` and no warnings.
- 17 November 2026, 0.3.0-experimental: build PT04, sha256 `b713cfd5ffabe35ec7da79114e19c8fcdf41c6f4aed82904748aeea44932dde8`, compiled and linked the same way; the two post sites call the loggers in the built image. `main.c` compiles with `-Wall -Wextra` and no warnings.
- 17 November 2026, 0.2.0-experimental: compiled and linked the same way as build PT03, sha256 `d5bd543bfab5bab887d979cd5fd0326b1716661721df3a0bc43677d261120c71`. In the built image the two sites call the hooks (`jsr`) and pad the rest with `nop`; curve 0 runs stock's own instructions.
- 17 November 2026, 0.1.0-experimental: compiled with Debian's `m68k-linux-gnu-gcc` and linked with core-ar2 0.7 by `scripts/link-image.mjs` against stock OS 1.74 (the stock file's hash matches Modwerk's identity). Build PT02, sha256 `ead6afff59c5480f71f8313a3b6a4590f978dd899c268894ad98ad0a2d550ebd`; the built image keeps 10 at `0x40285984`.
- The threshold's address and the velocity curve were read from OS 1.74's disassembly (`0x400821fa`, `0x40081bc0`), not observed running.
- `npm run module:doctor -- pad-thresh`: green but "browser builder", which fails by design (no pinned `.elemod` for a diagnostic), and the board claim, not checked (the board was not reachable).
- Not run in rytm-emu.

## Hardware

Results below are recorded only as the developer reports them.

- 17 November 2026, build PT02 (0.1.0-experimental), @filefolder's unit: with T at 0, light pad taps still played nothing; the same taps toggled mutes as on stock (reported).
- 17 November 2026, build PT03 (0.2.0-experimental), @filefolder's unit: a light tap left N unchanged, so no strike reached the OS, while the same tap toggled a mute (reported).
- 17 November 2026, build PT04 (0.3.0-experimental), @filefolder's unit, from a photo of the log: a firm hit on pad 6 posted key `96` pressed, then strike `06` (velocity `FF`, force 117) in the same tick, and the release 14 ticks later; light taps posted key `96` pressed and released 9-14 ticks apart, with no strike (reported).
- 17 November 2026, build PT05 (0.4.0-experimental), @filefolder's unit (reported): with V at 0 (off), light taps played nothing, as designed. With V at 8 and VELOCITY TO VOL off, a light tap played the pad. With VELOCITY TO VOL on it was inaudible at first; that sound had its filter envelope on, and with the filter envelope off, light taps played "just as I would expect" with VELOCITY TO VOL off and on.
