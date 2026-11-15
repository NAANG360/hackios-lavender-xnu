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

## Current task — Run #252 failure
XNU Run #252 (43733584216, source e463e56ca9d9c5b1d78c541ecfd751fe890f7a24) failed while assembling osfmk/arm/commpage/commpage_asm.s: two CASA instructions at source lines 276 and 326 require LSE, despite -mcpu=generic -march=armv8-a. These are handwritten assembly and are not controlled by the C compiler CPU default. The corrective patch 0112-lavender-commpage-armv8a-llsc.patch replaces each acquire CASA with ARMv8.0 LDAXR + STXR retry logic and retains the existing lock-busy failure path.

The workflow must require patch 0112 and verify both helpers contain LL/SC and no CASA. This is a targeted ISA compatibility fix; subsequent builds may expose additional handwritten LSE instructions, which must be fixed narrowly rather than weakening the ARMv8-A target.

Read Chapter 03. Run #252's raw kernel candidate is not verifier-approved and has no paired boot image; do not use it as a device image.

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
