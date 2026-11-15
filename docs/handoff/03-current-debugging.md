# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Handoff snapshot HEAD:** `17a68fb21294a99e8c11c3ee38f7b79df501033d`
**Latest CI candidate:** Run #236, ID `43701501661`, success
**Latest physical evidence:** Run #233 image; marker `0x104`

## Current state — do not conflate CI with phone evidence

Run #236 built XNU and packaged the RAM-only image successfully. Both Actions jobs passed. Artifacts are available:
- `lavender-fastboot-boot-test` — ID `14156453574`
- `xnu-kernel` — ID `14156363785`
- `xnu-kernel-candidate` — ID `14156289042`
- `xnu-build-log` — ID `14156538354`
- `xnu-buildtree` — ID `14157101726`

Run #235 failed; Run #236 source commit `17a68fb...` repaired the patch and succeeded. The Run #236 image has **not** been confirmed physically tested in the evidence available at this handoff. Do not infer its marker from Run #233.

## Last physical evidence

The Run #233 image showed:
- `arm_init entered (MMU on)`
- `PE_init_platform done`
- `dt_parse_cpu_topology done`
- `cpu_bootstrap done`
- Guardian snapshot: `A53 LAST MARK=0x00000104`

This proves C entry, platform init, topology parsing and completion of `cpu_bootstrap()`. It does not prove clock setup, timer init, later CPU init, VM init, scheduler, interrupts, Apple logo or Darwin userspace.

The guardian polls the shared marker. After 60 seconds without a changed value it displays the last marker for 2.5 seconds and resets. Therefore a frozen marker and later black screen match the designed timeout/reset sequence; the black screen itself is not evidence of a kernel panic.

## Run #236 marker interpretation

Patch `0108-lavender-post-cpubootstrap-trace.patch` splits the calls after stage 4 in `arm_init()`. Marker values use `0x100 | stage`:

| Value | Meaning |
|---|---|
| `0x105` | before `rtclock_early_init()` |
| `0x106` | after `rtclock_early_init()` |
| `0x107` | before `kernel_debug_string_early()` |
| `0x108` | after `kernel_debug_string_early()` |
| `0x109` | before `kernel_startup_bootstrap()` |
| `0x10A` | after `kernel_startup_bootstrap()` |
| `0x10B` | after `timer_call_init()` |
| `0x10C` | after `cpu_init()` |
| `0x10D` | after `processor_bootstrap()` |
| `0x10E` | entering `arm_vm_init()` |

If a test reports 0x105, the call was entered but did not reach its return marker. If 0x106, it returned and the next call is under investigation. Stage 14/0x10E only proves VM-init entry.

## Immediate next operation

1. Confirm the current live branch and latest run again before any action.
2. Retrieve `lavender-fastboot-boot-test` from Run #236 and verify exact filename `iosboot.img`, artifact provenance and manifest/checksums. Do not rename it with a date suffix.
3. Test only through the established RAM-only boot path; preserve Android/recovery.
4. Record the exact final `A53 LAST MARK` value and video/time evidence.
5. Map that value using the table above and inspect only the corresponding source call.
6. Do not trigger a new build until the evidence identifies a necessary source change.

## Supporting comparative investigation — Topaz

Topaz (`MakrSas/ios-topaz`) is a fork of `dipa-studio/ios-sweet`. Its patches provide a strong comparative lead because the SM6225 target uses A53/A73 CPU classes and a Gold-core XNU boot:
- `0080-topaz-cpu-topology-skip-sweet-hw.patch`: MPIDR ranges A53 `0x0–0x3`, A73 `0x100–0x103`; CPU identity Kryo 260; gates inherited SM7150-specific input-driver probes behind `sweethw=1`.
- `0081-topaz-armv8.0-no-lse-no-pan.patch`: Cortex-A53 compiler target, selected LSE instruction substitutions/emulation, no PAN feature define.
- `0082-topaz-boot-on-gold-cpu.patch`: expects boot CPU MPIDR `0x100` and documents Silver's missing 16K granule.

**Caution:** these are not independently proven fixes for Lavender. Topaz is based on Sweet and shares code. The immediate physical test of Run #236 should not be delayed or modified by speculative transplantation. Audit Lavender's actual compiler flags, generated instructions, feature registers and source before adapting any patch. Do not import SM6225/SM7150 peripheral addresses or device-tree values.

## Screen-clear explanation

The split is intentional:
- framebuffer height: 2340;
- `LAVENDER_CONSOLE_TOP = 600`;
- clear begins at y=600 and continues to y=2339.

The shim's top 600-pixel red diagnostics remain visible; the lower 1740 pixels (~74.4%) are the XNU text area. It is not a partial clear that froze.

## Previous memory-alignment correction

The old Stage-9 stall was traced to a 4 MiB physical/virtual offset mismatch with 32 MiB L2 blocks under 16 KiB translation. The payload base was aligned to `0xA2000000`, matching the virtual base's 32 MiB alignment. The Run #233 physical test then passed the old assembly-to-C barrier and reached stage 4. Treat memory ranges as specific to the audited boot configuration; revalidate against current FDT/DTB before changing them.

## Acceptance ladder

- [x] Enter `arm_init()` (Run #233 physical evidence)
- [x] Complete `PE_init_platform`
- [x] Parse CPU topology
- [x] Return from `cpu_bootstrap()`
- [ ] Complete `rtclock_early_init()` (Run #236 image not yet physically tested)
- [ ] Complete early startup hooks
- [ ] Complete timer and CPU initialization
- [ ] Complete `arm_vm_init()`
- [ ] Stable scheduler / interrupt / secondary CPU startup
- [ ] Stable kernel console
- [ ] Apple logo
- [ ] Darwin userspace

Never mark a stage complete from source inspection or CI alone.
