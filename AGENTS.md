# Agent instructions — Lavender XNU port

**Read `PROJECT_HANDOFF.md` first.** It is the canonical engineering record. At every session start, re-resolve live branch HEAD, the newest Actions run, its attempt, jobs, artifacts, and source SHA before acting.

Repository: `NAANG360/hackios-lavender-xnu`  
Branch: `ci/lavender-machine-profile`  
Target: Xiaomi Redmi Note 7 / Redmi Note 7S, codename `lavender`, Qualcomm Snapdragon 660 (SDM660).

## Non-negotiable engineering rules

- The objective is a working Darwin/XNU boot on physical Lavender, ultimately reaching a usable Darwin userspace. A compile, Mach-O, package, Apple-like logo, emulator, or static source assertion is not a complete boot.
- Keep milestones distinct: source committed; patch applied; XNU compiled; source guard passed; ARM64 Mach-O verified; RAM-only image packaged; image structurally verified; physical stage observed; Apple logo observed; Darwin userspace reached.
- Never claim a physical stage without actual device evidence. Separate observed facts, hypotheses and unverified assumptions.
- Read the newest handoff section and then inspect current GitHub Actions. Historical summaries below older headings are not authoritative over live CI.
- Preserve the physical target. Do not transplant MMIO, IRQ, CPU topology, display, regulator, touch or storage assumptions from unrelated devices.
- Do not spend time on emulator work as a substitute for physical bring-up. QEMU may be used only for isolated tests where useful.
- Preserve Android and the recovery path. Never flash or erase boot-critical partitions for this experiment. Prefer temporary RAM-only `fastboot boot`. The user's power button is broken; do not request another handset test until a recovery path independent of that button is actually established.
- Make small, testable changes. Inspect actual build logs and generated source before modifying code. Do not respond to an assertion failure by weakening/removing the assertion; fix the faulty check or the implementation.
- After every meaningful milestone, record exact source SHA, workflow run ID/attempt, job IDs and conclusions, artifact IDs, byte sizes, SHA-256 hashes, and physical observations in `PROJECT_HANDOFF.md`.
- Use the exact output filename **`iosboot.img`** for any user-facing image. Do not append dates, timestamps, build numbers or suffixes to the filename.

## Latest verified state — 2026-11-14, Asia/Kolkata

- Latest known successful XNU + RAM-only package remains run #203, ID `43594864654`, source commit `e9890318f144df763a3ff655a16ca5b5b5447eb5`. Its image artifact was `14114386476`; its extracted `iosboot.img` was 16,527,360 bytes with SHA-256 `172f6cb8b39241dc9997ecf5da1d1eaf5d18d2edb447711baff407c44136ac5f`. It is the last known packaged physical-test image, not proof of a successful boot.
- Current XNU instrumentation source commit: `6b5c477d820656f4b029b005d63596cb54705c82`. Patch 0107 adds assembly stages 8/9 immediately before the return into `arm_init()`, a namespaced C marker scheme (`0x100` entry, `0x100 | stage` C progress), marker publication before framebuffer console output, and explicit `dc cvac` then `dsb sy` sequence. Patch 0106 is the verbose framebuffer diagnostic implementation. The latest cropped run #203 image is geometrically consistent with five green tiles (stages 3–7), but blur prevents an exact marker readback.
- Run #213, ID `43606468548`, attempt 2, source commit `6b5c477d820656f4b029b005d63596cb54705c82`, **completed with failure in the source guard, not in XNU compilation**. Build job `132487666362` reports `Build XNU 8796` success and collected a 16,587,304-byte `kernel.development.lavender`. The guard then raised `ValueError: substring not found` because it searched the assembly comment `// Set KVA of boot args as first arg` in `arm_init.c`, rather than `start.s`. Kernel verification and package were skipped; no new `iosboot.img` exists from this run. See PROJECT_HANDOFF section 44.32.
- Workflow fix commit: `4cb4eec5fd57c6bc678520e8178cf805ef36eb10`. It moves stage 8/9 assertions to a separately loaded `start.s` source; checks required patch skips for both 0106 and 0107; adds `id: xnu-build`; removes workflow YAML from the cache hash; adds compatible cache restore prefixes; and saves compiled cache when the XNU build succeeded even if a later source guard failed.
- Current candidate run #220: ID `43611119178`, attempt 1, source commit `4cb4eec5fd57c6bc678520e8178cf805ef36eb10`; build job `132496159699`. At last check it was in progress at runner setup. No XNU compile, validator, Mach-O, artifact, or package success has yet been established for #220.
- The run #213 log shows a cold cache miss. Runner reported 4 physical CPUs, 4 logical CPUs and 15,032,385,536 bytes RAM; the build used `XNU_MAKE_JOBS=4`. This is already all reported logical CPUs. Do not blindly raise `-j` beyond memory/CPU capacity. Judge cache improvements only from actual cache restore output and measured build duration.
- Run #213 artifacts retained: build log `14120260161`; build tree `14119559638`. The build-tree package excluded `*.o`, `*.a`, and the kernel, so it cannot be treated as an incremental object cache or bootable artifact.

## Immediate next actions

1. Inspect live branch HEAD and run #220 state. Confirm its source SHA matches the workflow-fix commit or identify newer commits.
2. Check the XNU cache restore logs: exact hit, restore-prefix hit, or cold miss. Record the cache key and duration.
3. Inspect the XNU build log for explicit application of 0106 and 0107; ensure neither is skipped.
4. Confirm `Build XNU 8796` succeeds, then inspect the corrected source guard. It must read C milestones from `arm_init.c` and assembly handoff markers from `start.s`.
5. Require the ARM64 Mach-O verification to pass. Confirm the generated kernel identity and hash.
6. Confirm the cache-save step ran after successful compilation where applicable.
7. Confirm the package job succeeds, extracts the expected kernel/ADT payload, and creates exactly `iosboot.img`. Verify its structure and SHA-256 independently. Present the user a download named exactly `iosboot.img` — no date in the filename.
8. Do not claim the build-time fix worked until a cache hit and reduced elapsed time are evidenced.
9. Do not request a physical test until the broken-power-button recovery route is independently established. Then use RAM-only boot and record the last visible tile/marker, C console and Apple-logo status.

## Required reporting

Be direct and precise. The user expects the image, not another generic progress explanation. Report exactly what passed and failed. Never substitute a partial build tree or unverified kernel for `iosboot.img`, and never claim Darwin userspace without evidence.
