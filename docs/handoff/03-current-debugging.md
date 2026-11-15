# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Latest tested source:** `f2a4cfd4a65829352736783def33c6a5b61e4a68`
**Latest physical test:** Run #233 image, tested on the Redmi Note 7.

## Current physical evidence

The 73-second recording shows sequentially:
- `arm_init entered (MMU on)`
- `PE_init_platform done`
- `dt_parse_cpu_topology done`
- `cpu_bootstrap done`

Near the end, the A53 guardian prints `A53 LAST MARK=0x00000104`. The C marker encoding is `0x100 | stage`; therefore `0x104` is stage 4, the last completed checkpoint `cpu_bootstrap done`. The guardian source sets a 60-second no-progress timeout, prints the last marker, waits 2.5 seconds, then requests reset. The recording is consistent with that intentional timeout/reset path.

**What this proves:** XNU crossed the old assembly-to-`arm_init()` barrier and completed `PE_init_platform`, CPU topology parsing and `cpu_bootstrap()`.

**What it does not prove:** progress beyond stage 4, timer setup, startup hooks, CPU initialization, VM initialization, scheduler, interrupt controller, logo or userspace.

## Why only ~3/4 of the screen cleared

This is deliberate console layout, not a failed full-screen clear:
- `LAVENDER_FB_HEIGHT = 2340`
- `LAVENDER_CONSOLE_TOP = 600`
- `lavender_console_clear()` iterates from y=600 through y=2339 and paints black.

Thus the lower 1740/2340 pixels (~74.4%) are cleared for XNU's text console; the upper 600 pixels (~25.6%) retain the red shim diagnostic panel. The stage lines are written in the lower panel. The later screenshot of a bright upper section is camera exposure/glare; the persistent geometric split matches the explicit y=600 clear boundary.

## Exact next source path

In the exact Run #233 build tree, `osfmk/arm64/cpu.c` defines `cpu_bootstrap()` as an empty function. In `osfmk/arm/arm_init.c`, the next operations after marker 4 are:

1. `rtclock_early_init()`
2. `kernel_debug_string_early("kernel_startup_bootstrap")`
3. `kernel_startup_bootstrap()`
4. `timer_call_init()`
5. `cpu_init()`
6. `processor_bootstrap()`
7. Preparatory setup, then `arm_vm_init(xmaxmem, args)`

The existing trace had only one later marker after `kernel_startup_bootstrap()`, so the 0x104 snapshot could not distinguish these operations.

## Diagnostic change for next build

Patch `0108-lavender-post-cpubootstrap-trace.patch` inserts markers in `arm_init()`:
- stage 5 before / stage 6 after `rtclock_early_init()`;
- stage 7 before / stage 8 after `kernel_debug_string_early()`;
- stage 9 before / stage 10 after `kernel_startup_bootstrap()`;
- stage 11 after `timer_call_init()`;
- stage 12 after `cpu_init()`;
- stage 13 after `processor_bootstrap()`;
- stage 14 entering `arm_vm_init()`.

The CI source guard must verify this exact ordering. Because the XNU object cache can restore prior objects, the workflow explicitly touches `osfmk/arm/arm_init.c` after patch application so this object is rebuilt. Do not accept a build unless logs show the normal verifier and image package steps succeeded.

## Previous assembly-mapping correction

Run #232 stopped at assembly marker 9, immediately before `ret` to `arm_init()`. The earlier correction committed as `f2a4cfd4a65829352736783def33c6a5b61e4a68`:
- 16 KiB-granule L2 block size is 32 MiB;
- old `physBase=0xA0400000` was offset by 4 MiB while `virtBase=0xFFFFFFE006000000` was block-aligned;
- new `physBase=0xA2000000` is 32 MiB aligned;
- payload validator requires matching 32 MiB offsets for virtual and physical bases.

The phone's Android memory report supports `0xA2000000..0xEA848000` as available below the first upper reserved range beginning at `0xEA948000`; a 1 MiB panic reservation occupies the gap. Treat these as device/boot-configuration-specific values.

## CI evidence — Run #233

Run ID `43695509652` on source `f2a4cfd4...`: success.
- XNU 8796 build and Lavender kernel verification passed.
- Stock boot/DTB extraction, ADT generation, XNU payload construction and non-flashing fastboot image construction passed.
- Boot-image structure and payload identity checks passed.
- Artifact `lavender-fastboot-boot-test`, ID `14154570502`, contains `iosboot.img`.
- This validates CI gates only; the physical recording independently supplies the four C-stage observations above.

## Acceptance ladder

- [x] Enter `arm_init()`.
- [x] Complete `PE_init_platform`.
- [x] Parse CPU topology.
- [x] Return from `cpu_bootstrap()`.
- [ ] Complete clock/timebase early initialization.
- [ ] Complete early kernel startup hooks.
- [ ] Complete timer and CPU initialization.
- [ ] Complete `arm_vm_init()`.
- [ ] Stable scheduler / interrupt / secondary CPU startup.
- [ ] Stable kernel console.
- [ ] Apple logo.
- [ ] Darwin userspace.

Never report an unchecked stage as complete.
