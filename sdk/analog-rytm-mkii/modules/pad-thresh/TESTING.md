# Pad Thresh testing

Evidence tier: **none**. A diagnostic prototype, never submitted for release.

## Build

- 17 November 2026: compiled with Debian's `m68k-linux-gnu-gcc` and linked with core-ar2 0.7 by `scripts/link-image.mjs` against stock OS 1.74 (the stock file's hash matches Modwerk's identity). Build PT02, sha256 `ead6afff59c5480f71f8313a3b6a4590f978dd899c268894ad98ad0a2d550ebd`; the built image keeps 10 at `0x40285984`.
- The threshold's address and the velocity curve were read from OS 1.74's disassembly (`0x400821fa`, `0x40081bc0`), not observed running.
- `npm run module:doctor -- pad-thresh`: green but "browser builder", which fails by design (no pinned `.elemod` for a diagnostic), and the board claim, not checked (the board was not reachable).
- Not run in rytm-emu.

## Hardware

Not tested. Results below are recorded only as the developer reports them.
