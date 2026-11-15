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

## Current task — clear handwritten LSE from ARMv8-A commpage

**Failed run / root cause:** XNU #252 (`43733584216`, source `e463e56ca9d9c5b1d78c541ecfd751fe890f7a24`) failed in `Build XNU 8796`. The assembler rejected two handwritten LSE `CASA` instructions in `osfmk/arm/commpage/commpage_asm.s`: line 276 (`[x3]`) in `_pfz_trylock_and_enqueue`, and line 326 (`[x2]`) in `_pfz_trylock_and_dequeue`, both with `instruction requires: lse`. `-mcpu=generic -march=armv8-a` cannot rewrite explicit assembly. Verification and the RAM-only package job were skipped. The raw candidate is unapproved; #252 produced no `iosboot.img`.

**Corrective change:** `0112-lavender-commpage-armv8a-llsc.patch` replaces both acquire-CASA operations with ARMv8.0 `LDAXR/STXR` retry loops. It retains the failed-lock return path and existing release-store unlock. The corrected patch/source guards are in commit `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`. The XNU workflow requires 0112 and checks that these helpers contain the expected LL/SC sequence and no CASA.

**Current validation:** XNU #254 (`43736404806`, source `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) is **in progress**. Runner setup, Xcode selection, pinned tooling, machine-profile validation, cache restore/invalidation and dependencies passed. `Build XNU 8796` remains the active step. Build-path guards, linked-kernel verification, artifact collection and RAM-image packaging are pending. The latest log tail is still ordinary compile/header work; it does not yet establish that commpage assembly passed. #253 (`43736362523`) is cancelled as superseded; #252 remains failed.

**Next:** monitor #254 through completion. If another explicit LSE assembly instruction fails, inspect that exact source site and replace it with a semantics-preserving ARMv8.0 sequence; do not globally re-enable LSE or weaken checks. Require source guards, linked-kernel ISA verification and the package job before calling an image ready. Never boot #252's raw candidate. No handset retest until the power-button-independent recovery/reset safety gate is physically verified; RAM boot only, never flash.

The Run #240 callback mapping remains the physical debugging lead after a newly verified image exists. Do not bypass or reorder the mutex initializer.
 
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
