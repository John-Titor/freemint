# mintboot

Standalone bootloader and ROM_* syscall emulator scaffolding.

Design split:
- `common/`: ROM calling conventions, vector setup, trap/exception handling,
  kernel image loading, and boot flow.
- `board/`: board-specific startup and device drivers (console, timers, etc.).

This is a scaffold only; no build targets are defined yet.

Documentation for the ROM functions being emulated is available at
[https://freemint.github.io/tos.hyp/en/index.html#UDOTOC](https://freemint.github.io/tos.hyp/en/index.html#UDOTOC).
