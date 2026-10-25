# Pad Probe testing

Evidence tier: **none**. A diagnostic prototype, never submitted for release.

## Build

- Host `gcc -fsyntax-only -Wall -Wextra -Werror` on `src/main.c` with `-DFILLRECT_AT=0x40070726`: passes. This is a syntax check on the host, not a ColdFire build.
- Not compiled with `m68k-elf` or `m68k-linux-gnu` GCC, not linked with core-ar2 0.3, not linted: no toolchain in the development workspace.
- `npm run module:doctor -- pad-probe`: manifest, category guide and module rules pass. "browser builder" fails by design: no pinned `.elemod` exists for a diagnostic.

## Hardware

Not tested. Results below are recorded only as the developer reports them.
