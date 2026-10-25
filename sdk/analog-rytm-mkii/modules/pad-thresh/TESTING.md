# Pad Thresh testing

Evidence tier: **none**. A diagnostic prototype, never submitted for release.

## Build

- Host `gcc -fsyntax-only -Wall -Wextra -Werror` on `src/main.c` with the OS 1.74 `-D` flags from `build.json` (and `-Wno-int-to-pointer-cast`, as the host's pointers are 64-bit): passes. This is a syntax check on the host, not a ColdFire build.
- Not compiled with `m68k-elf` or `m68k-linux-gnu` GCC, not linked with core-ar2 0.3, not linted: no toolchain in the development workspace.
- `npm run module:doctor -- pad-thresh`: "browser builder" fails by design, as no pinned `.elemod` exists for a diagnostic.

## Hardware

Not tested. Results below are recorded only as the developer reports them.
