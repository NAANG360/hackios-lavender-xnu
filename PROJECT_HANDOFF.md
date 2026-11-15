# Lavender XNU — seamless project handoff

**Resume command:** “Continue from latest operations.”

A new session must read this file, inspect live GitHub branch/Actions state, then load only the chapter named below. Repository and live evidence outrank this snapshot if they differ.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Xiaomi Redmi Note 7/7S, `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Mission: physical XNU 8796 (iOS 16 generation) bring-up; Darwin userspace is a later milestone.

## Current task / chapter route

**Active task:** test the newly instrumented Run #236 `iosboot.img` on the physical handset using the established RAM-only path, then identify the last completed post-`cpu_bootstrap()` marker. In parallel, audit Topaz's ARMv8.0 CPU compatibility changes against Lavender source/configuration; this is comparative research, not permission to transplant patches.

**Read now:** [Chapter 03 — Current debugging](docs/handoff/03-current-debugging.md).

**Read additionally only if needed:**
- Build/patch/cache/packaging change → [Chapter 02](docs/handoff/02-architecture-and-build.md).
- Before any device action or safety decision → [Chapter 01](docs/handoff/01-device-and-safety.md).
- Historical comparison/regression investigation → [Chapter 04](docs/handoff/04-evidence-and-history.md).

Do not read every chapter by default.

## Live state verified for this handoff

- Branch HEAD: `17a68fb21294a99e8c11c3ee38f7b79df501033d`
- Latest XNU workflow: **Run #236**, ID `43701501661`, conclusion **success**, source SHA matches HEAD.
- Run #236 jobs: XNU build and “Package RAM-only Lavender fastboot image” both succeeded.
- Run #236 artifacts (not expired at inspection):
  - `lavender-fastboot-boot-test`, artifact ID `14156453574`
  - `xnu-kernel`, artifact ID `14156363785`
  - `xnu-kernel-candidate`, artifact ID `14156289042`
  - `xnu-build-log`, artifact ID `14156538354`
  - `xnu-buildtree`, artifact ID `14157101726`
- Run #235 (ID `43700842842`) failed; Run #236 commit `17a68fb...` repaired the post-`cpu_bootstrap` diagnostic patch and completed both jobs.
- Run #236 is a successful build/package result, **not a physical test**. Do not assume its marker output or claim a later boot stage until the handset is tested.

## Last confirmed physical frontier

The latest documented physical recording is still the Run #233 image, which displayed `A53 LAST MARK=0x00000104` after:
1. `arm_init()` entry
2. `PE_init_platform`
3. CPU topology parsing
4. `cpu_bootstrap()`

That proves the earlier Stage-9 assembly-to-C transition was fixed and XNU reached C initialization. It does not establish that Run #236 has been tested. The guardian's 60-second no-progress timeout then snapshots the marker, displays it for 2.5 seconds and resets.

## Run #236 diagnostic map

Patch `xnu/patches/0108-lavender-post-cpubootstrap-trace.patch` is now in the build chain. Its C marker namespace is `0x100 | stage`:

| Marker | Meaning |
|---:|---|
| `0x105` | entering `rtclock_early_init()` |
| `0x106` | `rtclock_early_init()` returned |
| `0x107` | entering `kernel_debug_string_early()` |
| `0x108` | early debug-string call returned |
| `0x109` | entering `kernel_startup_bootstrap()` |
| `0x10A` | kernel startup bootstrap returned |
| `0x10B` | `timer_call_init()` returned |
| `0x10C` | `cpu_init()` returned |
| `0x10D` | `processor_bootstrap()` returned |
| `0x10E` | entering `arm_vm_init()` |

Stage 4 (`0x104`) remains the last confirmed phone marker until a newer recording proves otherwise. Stage 14 is an entry marker, not proof that VM initialization completed.

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
