# Lavender XNU — master handoff

**Read this file only at session start.** Resolve the live branch HEAD and Actions state before changing anything. Load only the chapter needed for the task.

- Repo: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Xiaomi Redmi Note 7/7S, `lavender`, Qualcomm SDM660, ARM64
- Goal: physical XNU 8796 / iOS 16-generation kernel bring-up; later Darwin userspace.

## Current physical frontier

The #233 `iosboot.img` was tested on the actual handset. The 73-second recording confirms:
1. `arm_init entered (MMU on)`
2. `PE_init_platform done`
3. `dt_parse_cpu_topology done`
4. `cpu_bootstrap done`

Near the end, the A53 guardian reports `A53 LAST MARK=0x00000104` (C checkpoint 4). The marker does not advance beyond `cpu_bootstrap done`; the guardian's configured 60-second no-progress timeout then snapshots the value and resets. This is the current failure frontier, not the old Stage 9 assembly-to-C failure.

## Screen-clear explanation

The split screen is intentional, not a partial clear that got stuck. In the Lavender early console:
- framebuffer height = 2340 pixels;
- `LAVENDER_CONSOLE_TOP = 600`;
- `lavender_console_clear()` clears rows 600–2339 only.

That preserves the top 600 pixels (about 26%) containing the shim's red handoff diagnostics, while clearing the lower 1740 pixels (about 74%) for XNU's verbose text. The four C-stage lines are rendered in that lower panel.

## Latest accepted build

- XNU source commit: `f2a4cfd4a65829352736783def33c6a5b61e4a68`
- XNU Run #233: `43695509652` — build, kernel verification and package jobs passed.
- Shim/ADT Run #47: `43695509648` — passed.
- Fastboot artifact: `lavender-fastboot-boot-test`, artifact ID `14154570502`, exact image name `iosboot.img`.
- Android memory audit supports the configured `0xA2000000` aligned aperture through `0xEA848000`, with panic reservation ending at `0xEA948000`, for the observed boot configuration.

## Immediate next action

1. Trace and split the operations immediately after `cpu_bootstrap()` in the exact XNU source.
2. Patch `0108-lavender-post-cpubootstrap-trace.patch` adds distinct markers:
   - 5 before / 6 after `rtclock_early_init()`;
   - 7 before / 8 after `kernel_debug_string_early()`;
   - 9 before / 10 after `kernel_startup_bootstrap()`;
   - 11 after `timer_call_init()`;
   - 12 after `cpu_init()`;
   - 13 after `processor_bootstrap()`;
   - 14 immediately before `arm_vm_init()`.
3. CI must require patch 0108 and verify marker order. Cache invalidation must touch `arm_init.c` so the changed object is rebuilt.
4. Build once, inspect verifier and package results, then test exact `iosboot.img` using RAM-only `fastboot boot`.
5. Use the next guardian marker to pick the next focused source-level fix. Do not guess which call is failing until the new checkpoints identify it.

**Safety:** recovery is built into the shim per user confirmation. Never flash, erase, repartition or write boot-critical partitions.

## Selective chapter index
- [01 — Device and safety](docs/handoff/01-device-and-safety.md)
- [02 — Architecture and build](docs/handoff/02-architecture-and-build.md)
- [03 — Current debugging](docs/handoff/03-current-debugging.md)
- [04 — Evidence and history](docs/handoff/04-evidence-and-history.md)

## Session protocol
- Read this file first; inspect live branch HEAD and newest relevant Actions run.
- For current work, open Chapter 03 only. Read Chapter 02 before build/workflow changes; Chapter 01 before device tests; Chapter 04 only for historical comparisons.
- Current source, Actions logs/artifacts and physical recordings outrank stale prose.
- Keep observed facts separate from hypotheses. Never claim Apple logo or Darwin userspace until seen on device.
