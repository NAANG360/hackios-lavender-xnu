# Agent entrypoint — Lavender XNU

1. Read `PROJECT_HANDOFF.md` only.
2. Resolve the live branch HEAD and newest relevant Actions run.
3. Follow **Immediate next action**; load only the linked chapter.
4. Inspect current source, logs, artifacts and physical evidence. Validate narrowly before CI.
5. Keep fixes targeted. Never weaken guards, overlap builds, or claim unobserved boot stages.
6. Update the current pointer and affected handoff chapter after each milestone.

## Mission
Bring XNU 8796 (iOS 16 generation) up on physical Xiaomi Redmi Note 7/7S (`lavender`, SDM660, ARM64). Long-term target: Darwin userspace.

## Hard constraints
- Preserve Android/recovery. RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write boot-critical partitions.
- Recovery is built into the shim (user-confirmed); do not treat it as a blocker.
- Power button is broken; use the shim recovery path.
- QEMU is not hardware proof.
- Exact deliverable name: `iosboot.img`.
- No raw dumps, serials, calibration/private data or firmware blobs in Git.
- Keep compile, verified Mach-O, packaged image, physical boot, Apple logo and Darwin userspace as separate acceptance gates.

## Current pointer
Run #233 (`43695509652`) built and packaged successfully. Its image reached XNU C initialization on the phone:
`arm_init` → `PE_init_platform` → CPU topology parse → `cpu_bootstrap`.
The 73-second recording ends with guardian snapshot `A53 LAST MARK=0x00000104`, so no later marker was observed. The guardian is configured to snapshot after 60 seconds without progress, then reset.

The apparent 3/4-screen clear is intentional: `lavender_console_clear()` starts at y=600 on a 2340-pixel framebuffer, preserving the top 600-pixel red shim panel and clearing the lower 1740 pixels for XNU text.

Next diagnostic patch: `0108-lavender-post-cpubootstrap-trace.patch`. It splits the next calls in `arm_init()` into markers 5–14, starting before `rtclock_early_init()`. Read [Chapter 03](docs/handoff/03-current-debugging.md).

## Selective chapters
- [01 Device and safety](docs/handoff/01-device-and-safety.md)
- [02 Architecture and build](docs/handoff/02-architecture-and-build.md)
- [03 Current debugging](docs/handoff/03-current-debugging.md)
- [04 Evidence and history](docs/handoff/04-evidence-and-history.md)
