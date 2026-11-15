# Agent entrypoint — Lavender XNU

1. Read `PROJECT_HANDOFF.md` only.
2. Query live branch HEAD and newest relevant GitHub Actions run before acting.
3. Follow its **Immediate next action**. Read only the linked chapter needed for the task; never bulk-read all chapters.
4. Inspect current source, actual logs and artifacts. Validate fixes locally against retained artifacts before spending a full CI build.
5. Keep changes narrow. Do not weaken guards, invent success, or overlap Actions builds.
6. After a milestone, update the short current-state block and only the affected chapter.

## Mission
Port XNU 8796 (iOS 16 generation) to physical Xiaomi Redmi Note 7/7S (`lavender`, SDM660, ARM64); long-term goal is iOS 16.5-era Darwin userspace. Real device bring-up, not an Android skin or emulator demo.

## Hard constraints
- Preserve Android/recovery: no flashing, erasing, repartitioning or boot-critical writes. RAM-only `fastboot boot iosboot.img` only; permanent writes require explicit approval.
- User's power button is broken. No physical test until recovery/reboot independent of that button is established.
- QEMU only for isolated tests, never compatibility proof.
- Exact deliverable filename: `iosboot.img` (no suffix).
- No raw dumps, serials, calibration/private data or firmware blobs in Git.
- Never claim compile = verified kernel = packaged image = physical boot = Apple logo = Darwin userspace.

## Current pointer
Run #231 (`43677707091`) compiled but failed official Mach-O verification: non-executable file-backed `__CTF,__ctf` metadata is rejected because segment VM size is zero. Fix `tools/verify-lavender-kernel.py` narrowly, test retained candidate `14147107920` locally, then run one corrected CI build. Full details: [Chapter 03](docs/handoff/03-current-debugging.md).

## Selective chapters
- [01 Device and safety](docs/handoff/01-device-and-safety.md)
- [02 Architecture and build](docs/handoff/02-architecture-and-build.md)
- [03 Current debugging](docs/handoff/03-current-debugging.md)
- [04 Evidence and history](docs/handoff/04-evidence-and-history.md)
