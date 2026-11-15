# Lavender XNU — seamless project handoff

**Resume command:** “Continue from latest operations.”

A new session must read this file, inspect live GitHub branch/Actions state, then load only the chapter named below. Repository and live evidence outrank this snapshot if they differ.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Xiaomi Redmi Note 7/7S, `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Mission: physical XNU 8796 (iOS 16 generation) bring-up; Darwin userspace is a later milestone.

## Current task / chapter route

**Active task:** Run #240 now builds and packages patch `0109-lavender-startup-hook-trace.patch`. The next diagnostic is the bottom-row trace from that image, which distinguishes startup-table validation, `qsort`, subsystem logging and the individual `STARTUP()` callback where execution stops. The old physical frontier remains Run #236 marker `0x109`. The device power button is broken; do not request another boot until a reboot/recovery path independent of that button is established. Do not start another build before the diagnostic line is captured.

**Read now:** [Chapter 03 — Current debugging](docs/handoff/03-current-debugging.md).

**Read additionally only if needed:**
- Build/patch/cache/packaging change → [Chapter 02](docs/handoff/02-architecture-and-build.md).
- Before any device action or safety decision → [Chapter 01](docs/handoff/01-device-and-safety.md).
- Historical comparison/regression investigation → [Chapter 04](docs/handoff/04-evidence-and-history.md).

Do not read every chapter by default.

## Latest diagnostic candidate — Run #240

- Live branch HEAD and Run #240 source SHA: `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`.
- Workflow ID `43709690089`, conclusion `success`; XNU build and RAM-only package jobs both passed, including the Lavender entry guard, Mach-O verifier, and boot-image structure/payload verification.
- `0109-lavender-startup-hook-trace.patch` was applied and both `startup.c` and `sweet_early.c` compiled into the accepted kernel.
- Fastboot artifact `lavender-fastboot-boot-test`, ID `14160128075`. Image exact name `iosboot.img`, size 16,531,456 bytes; SHA-256 `e21d513f42f288e2cbd1705111507c9ba9ab398c7366b397227efb6b9227922a`. The ZIP contains the same bytes at `shim/out/shim-boot.img`.
- Kernel artifact `xnu-kernel`, ID `14159918757`; kernel size 16,587,456 bytes; SHA-256 `cba785aa6ccf6916ca86fde8ff97b1b4f583e888b5b57b8ba159b1e03743e712`. Official verifier passes ARM64 Mach-O, Lavender identity and compiled stage-8/stage-9 sequences.
- Buildtree artifact ID `14160182439`; build log artifact ID `14160376884`.
- Runs #238 and #239 failed only in the temporary source-guard check while its ordering/delimiter assertions were being corrected. Run #240 is the corrected passing run. Run #237 also failed the patch guard before the patch was added.
- This is a successfully compiled and packaged diagnostic candidate, **not** a newer physical test. The only observed phone result remains Run #236 at marker `0x109`.

## Run #236 physical recording

- Video `1000317701.mp4`, 87.8 seconds. Its checkpoint labels match Run #236's instrumentation. No device-side checksum was captured, so its association with Run #236 is consistent with the labels, not byte-for-byte proven.
- The final visible line is `[09] entering kernel_startup_bootstrap` (marker `0x109`); marker `0x10A` is absent. The display later goes black and shows a charging/battery indication; the cause is unknown and must not be described as a confirmed kernel panic.

## Current physical frontier

Confirmed on the handset: `arm_init()`, `PE_init_platform`, CPU-topology parsing, `cpu_bootstrap()`, return from `rtclock_early_init()`, return from `kernel_debug_string_early()`, and entry to `kernel_startup_bootstrap()`.

Not confirmed: return from `kernel_startup_bootstrap()`, timer/CPU initialization, `arm_vm_init()`, stable scheduler/interrupt operation, Apple logo or Darwin userspace.

## Run #236 marker map

| Marker | Meaning |
|---:|---|
| `0x105` | entering `rtclock_early_init()` |
| `0x106` | `rtclock_early_init()` returned |
| `0x107` | entering `kernel_debug_string_early()` |
| `0x108` | early debug-string call returned |
| `0x109` | entering `kernel_startup_bootstrap()` |
| `0x10A` | `kernel_startup_bootstrap()` returned |
| `0x10B` | `timer_call_init()` returned |
| `0x10C` | `cpu_init()` returned |
| `0x10D` | `processor_bootstrap()` returned |
| `0x10E` | entering `arm_vm_init()` |
| `0x10F` | `arm_vm_init()` returned |

## Patch 0109 — current diagnostic logic

The new trace is a console-only rolling line at the bottom of the XNU framebuffer. It does **not** update the Guardian's shared progress cell and does not change startup callback order or skip any hook.

Events shown by the line:
- `TABLE n=...`: table count, byte size and start/end addresses, before validation.
- `QSORT BEGIN` / `QSORT DONE`: the sorting call boundary.
- `INIT BEGIN` / `INIT DONE`: `kernel_startup_initialize_upto(STARTUP_SUB_LOCKS)` boundary.
- `SUBSYS LOG BEGIN/DONE`: subsystem-name early logging boundary.
- `HOOK idx=... sub=... rank=... fn=... arg=...`: callback about to execute. If this remains the last line, that callback (or the immediate post-call path) is the next source to inspect.

## Exact Run #240 startup-table audit

The accepted Run #240 kernel has `__BOOTDATA,__init_entry_set` at file offset `0xb1cd58`, size `0x17e20` (97,824 bytes), exactly 4,076 entries of 24 bytes each. The same table extracted from the binary was sorted using the XNU 8796 `bsd/kern/qsort.c` algorithm against the actual `startup_entry_cmp` fields.

- XNU qsort simulation: 10,790 comparisons; maximum recursion depth 7.
- Simulated sorted entries: TUNABLES 242, TIMEOUTS 18, LOCKS 614 (874 callbacks total before `STARTUP_SUB_LOCKS` returns).
- Every entry's function pointer fell inside a mapped executable segment; every non-null argument fell inside a mapped kernel segment.
- The simulated result had zero subsystem/rank ordering violations.
- Linked symbols in the accepted kernel include `_sweet_early_trace_startup`, `_kernel_startup_bootstrap`, `_kernel_startup_initialize_upto` and `_qsort`.

These checks make a malformed/empty startup table and pathological host-side sort workload less likely. They do **not** prove the handset's physical qsort path completes or identify the blocking callback. The next useful evidence is the last visible Run #240 trace line.

## Safety gate

The device power button is broken. Do not request another physical boot until a recovery/reboot path independent of that button is established. Preserve Android/recovery; no flashing, erasing or repartitioning. The Volume Up + Volume Down chord has not been verified as a recovery selector.

## Comparative reference lead

Topaz (`MakrSas/ios-topaz`) is a **fork of** `dipa-studio/ios-sweet`, not an independent codebase. Its verified public patches are useful:
- `0080-topaz-cpu-topology-skip-sweet-hw.patch`: changes Sweet CPU matching to MPIDR Aff1 cluster / Aff0 core ranges `0x0–0x3` A53 and `0x100–0x103` A73; names CPU Kryo 260; gates inherited SM7150-specific keys/touch/USB probes behind `sweethw=1`.
- `0081-topaz-armv8.0-no-lse-no-pan.patch`: selects `-mcpu=cortex-a53`, replaces LSE atomics in selected paths with LL/SC loops and adds user-mode LSE/LDAPR emulation; removes the PAN-available define.
- `0082-topaz-boot-on-gold-cpu.patch`: requires boot logical CPU 0 to have physical MPIDR `0x100`, documenting that the shim boots XNU on the Gold core because the Silver cluster lacks 16K granule support.

These are reference evidence, not drop-in Lavender patches. Topaz inherits Sweet; its boot/driver/ADT and SoC constants are not Lavender facts. Audit exact source, compiler flags, XNU build configuration and Lavender hardware observations before proposing any adaptation. Do not describe patch 0080 as merely a “topology skip” or assume it changes Lavender's topology correctly.

## Safety and acceptance gates

- Keep Android/recovery intact. Use RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write boot-critical partitions.
- The device power button is broken. The shim recovery/reset path is the known route; do not assume a volume-key chord selects recovery without physical proof.
- Exact output filename: `iosboot.img`.
- Do not commit private/raw dumps, serials, calibration data or firmware blobs.
- Separate build, verifier, package, physical kernel stages, Apple logo and Darwin userspace. No logo/userspace claims without direct evidence.

## Session completion rule

After the next physical test, record the exact artifact/run/source SHA tested, exact marker and timestamp/video evidence, what it proves, and the next narrow task. Update this file, AGENTS.md, Chapter 03 and Chapter 04. Then the next session can resume with only “Continue from latest operations.”
