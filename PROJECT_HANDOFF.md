# Lavender XNU — seamless project handoff

**Resume command:** “Continue from latest operations.”

A new session must read this file, inspect live GitHub branch/Actions state, then load only the chapter named below. Repository and live evidence outrank this snapshot if they differ.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Xiaomi Redmi Note 7/7S, `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Mission: physical XNU 8796 (iOS 16 generation) bring-up; Darwin userspace is a later milestone.

## Current task / chapter route

**Active task:** Run #236's physical test is now recorded at marker `0x109` (entered `kernel_startup_bootstrap()`, but did not observe its return marker `0x10A`). The next narrow task is to add non-invasive internal trace points around startup-section validation, `qsort`, and the individual `STARTUP()` callbacks through `STARTUP_SUB_LOCKS`. Do not repeat the Run #236 test or start a blind rebuild.

**Read now:** [Chapter 03 — Current debugging](docs/handoff/03-current-debugging.md).

**Read additionally only if needed:**
- Build/patch/cache/packaging change → [Chapter 02](docs/handoff/02-architecture-and-build.md).
- Before any device action or safety decision → [Chapter 01](docs/handoff/01-device-and-safety.md).
- Historical comparison/regression investigation → [Chapter 04](docs/handoff/04-evidence-and-history.md).

Do not read every chapter by default.

## Run #236 candidate and physical evidence

- Tested candidate: Run #236, workflow ID `43701501661`, source SHA `17a68fb21294a99e8c11c3ee38f7b79df501033d`; both XNU build and RAM-only package jobs succeeded.
- Artifact `lavender-fastboot-boot-test`, ID `14156453574`; exact delivered filename `iosboot.img`, 16,531,456 bytes, SHA-256 `726a0708d2c47b21e26ddb2b7331b303013fc6b0bf3f0517f9a0cfd1d02758f8`.
- Physical recording: `1000317701.mp4`, 87.8 seconds. Its checkpoint labels match the Run #236 post-`cpu_bootstrap` instrumentation. No device-side image checksum was captured, so the video is consistent with the delivered Run #236 image, not cryptographically tied to it.
- The video shows markers 1–9, with the last visible line `[09] entering kernel_startup_bootstrap`. Marker `0x00000109` is the latest observed phone marker. There is no visible marker 10 / `0x10A` return.
- The display goes black near the end and a charging/battery indication appears. This does not establish a kernel panic or prove why the display went dark.

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

## Next diagnostic target — XNU 8796 startup hooks

The exact Run #236 buildtree contains XNU 8796 `osfmk/kern/startup.c`. `kernel_startup_bootstrap()` computes the number of `startup_entries`, panics if the table is empty or malformed, sorts it using `qsort(..., startup_entry_cmp)`, then calls `kernel_startup_initialize_upto(STARTUP_SUB_LOCKS)`.

The initializer asserts `startup_phase < upto`, then invokes each eligible `cur->func(cur->arg)` in a loop and advances `startup_entry_cur` only after the callback returns. The current marker cannot distinguish a section-validation problem, a `qsort` problem, or a callback that stalls.

Next patch should print compact console-only events before/after each boundary and each callback, including callback index, subsystem, rank and function address where safe. The early Lavender console has 108 text rows; keep the trace bounded and verify the final linked hook-table order rather than assuming source grep order.

**Do not update the Guardian's shared marker cell for these sub-events.** Any cell change resets its no-progress timer, and the high bit at stage 20 ends the early guard. Use a separate console-only trace helper. Do not reuse stage 16, 17 or 20. Preserve the existing `0x109` Guardian timeout semantics.

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
