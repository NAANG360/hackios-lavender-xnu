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

## Current task — ARMv8-A commpage assembly correction

XNU Run #252 (43733584216, source `e463e56ca9d9c5b1d78c541ecfd751fe890f7a24`) failed because handwritten `CASA` instructions in `osfmk/arm/commpage/commpage_asm.s` lines 276 and 326 require LSE. The generic ARMv8-A C compiler flags do not rewrite handwritten assembly. The failure is confirmed in the build log; kernel verification and RAM-image packaging were skipped.

Patch `0112-lavender-commpage-armv8a-llsc.patch` replaces both acquire-CAS sequences in `_pfz_trylock_and_enqueue` and `_pfz_trylock_and_dequeue` with `LDAXR/STXR` retry loops, preserving the busy-lock failure path. The first patch attempt had incorrect hunk/source-guard details; commit `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba` corrects those and the workflow requires 0112 plus source checks.

**Current validation:** XNU #254 (`43736404806`, exact HEAD `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) is in progress at “Build XNU 8796”; setup, Xcode selection, profile/cache checks and dependency install passed. XNU #253 (`43736362523`) used the earlier pre-correction commit and has been cancellation-requested as superseded. Do not count either as a successful kernel or package until verifier and artifact steps pass.

Next: monitor #254, resolve any further ARMv8-A assembly failures narrowly, then require linked-kernel ISA verification and RAM-only image package checks. Preserve `-mcpu=generic -march=armv8-a`; do not re-enable LSE. Run #252's raw kernel candidate is not verifier-approved and has no paired boot image; never use it as a device image.

Read Chapter 03. Keep the Run #240 callback mapping as the physical debugging lead after a new verified image is produced. No handset test until the power-button-independent recovery/reset safety gate is satisfied.

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
