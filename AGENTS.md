# Lavender XNU — agent entrypoint

**Start here.** Read PROJECT_HANDOFF.md, inspect live branch HEAD and newest Actions runs, then open only the chapter selected by its routing table. “Continue from latest operations” means resume autonomously from live evidence.

## Mission
Bring XNU 8796 (iOS 16 generation) up on Xiaomi Redmi Note 7/7S (lavender, SDM660 / Kryo 260, ARM64); Darwin userspace is later.

## Mandatory protocol
1. Read PROJECT_HANDOFF.md, resolve live branch HEAD and newest relevant Actions runs.
2. Follow the single current chapter route; do not load every chapter.
3. Inspect exact source, CI logs/artifacts and physical evidence before edits.
4. Continue the latest unfinished operation; avoid duplicate/overlapping builds.
5. Make the smallest evidence-backed fix. Validate source and generated instructions; never weaken guards/verifiers.
6. After meaningful milestones update this file, the handoff and active chapter.

## Safety and truth
- Preserve Android/recovery. RAM boot only: fastboot boot iosboot.img; never flash, erase, repartition or write boot-critical partitions.
- Power button is broken. No handset retest until a power-button-independent recovery/reset route is physically verified.
- QEMU is not proof of Lavender compatibility.
- Keep compile → Mach-O verification → package → physical XNU stages → Apple logo → Darwin userspace separate.
- Never claim physical boot/logo/userspace without direct device evidence.
- Do not commit raw dumps, serials, calibration data or firmware blobs.

## Device facts
Lavender, Qualcomm SDM660 / Kryo 260, ARM64. Observed Silver MPIDRs 0x0–0x3, Gold MPIDRs 0x100–0x103; boot path uses Gold 0x100 due the A53 16K-granule limitation observed so far. Revalidate before changing.

## Current task — repair malformed ARMv8-A commpage patch and validate fresh build

**#252 root cause:** XNU #252 (`43733584216`, source `e463e56ca9d9c5b1d78c541ecfd751fe890f7a24`) failed because handwritten LSE `CASA` instructions at `commpage_asm.s:276` and `:326` were not supported by the ARMv8-A target.

**#254 follow-up result:** XNU #254 (`43736404806`, source `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) also failed. The build log proves patch `0112-lavender-commpage-armv8a-llsc.patch` was skipped: `patch fragment without header at line 24`. Its two hunk headers declared 10/15 lines each, but the actual first hunk contains 11 old / 16 new lines and the second contains 9 old / 14 new lines. Since 0112 was skipped, both original CASA instructions remained and the assembler failed with `instruction requires: lse`. Verification and RAM-only packaging were skipped. Raw candidate artifact `14171510404` is unapproved; no `iosboot.img` exists for #254.

**Fix committed:** commit `24f81dc92b1b9bbe13a9cf15a0a5757ae5ed06ac` corrects the hunk headers to `@@ -272,11 +272,16` and `@@ -322,9 +327,14`. This is a patch-format correction only; it preserves the ARMv8.0 `LDAXR/STXR` loops, busy-lock failure return and release-store unlock. The push matches the XNU workflow's `xnu/patches/**` trigger. A new run must prove the patch actually applies, both source guards pass, the linked Mach-O verifier passes, and the RAM-only package job uploads `iosboot.img`. Do not assume the new run has passed until its live status and artifacts are checked.

**Next:** inspect the new Actions run for commit `24f81dc92b1b9bbe13a9cf15a0a5757ae5ed06ac`. First confirm there is no `Skipping patch 0112` line and the commpage source has no CASA. Then require successful XNU compilation, source guards, linked-kernel ARMv8-A verification and RAM-only image packaging. Do not re-enable LSE globally, weaken checks, use #254's raw candidate, or start overlapping builds.

The Run #240 callback mapping remains the later physical debugging lead. Do not bypass or reorder the mutex initializer. No handset retest until a power-button-independent recovery/reset route is physically verified; RAM boot only, never flash.

## Topaz reference
MakrSas/ios-topaz is a useful iOS 16.7.16 / XNU 8796 comparison for Kryo 260-class topology, ARMv8.0 limitations, Gold-core handoff, GIC setup and boot-image-v4 shim. It targets Redmi Note 12 4G / SM6225 and is Sweet-derived, not a Lavender port. Never copy its MMIO/DT/ADT constants or cherry-pick patches without SDM660 evidence and linked-binary verification. See Chapter 02.

## Chapter router
- docs/handoff/README.md — router and resume protocol.
- docs/handoff/01-device-and-safety.md — physical safety, hardware, memory/display.
- docs/handoff/02-architecture-and-build.md — architecture, patches, CI, packaging, Topaz/Sweet.
- docs/handoff/03-current-debugging.md — active failure frontier and next operation.
- docs/handoff/04-evidence-and-history.md — historical evidence and regressions.

Repository: https://github.com/NAANG360/hackios-lavender-xnu
Branch: ci/lavender-machine-profile
