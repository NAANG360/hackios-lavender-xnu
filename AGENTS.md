# Lavender XNU — agent entrypoint

**Start here.** Read PROJECT_HANDOFF.md, inspect live branch HEAD and newest Actions runs, then open only the chapter selected by its routing table. “Continue from latest operations” means resume autonomously from live evidence.

## Mission
Bring XNU 8796 (iOS 16 generation) up on Xiaomi Redmi Note 7/7S (lavender, SDM660 / Kryo 260, ARM64); Darwin userspace is later.

## Mandatory protocol

When a difficult XNU/ARM issue benefits from independent review, the user may bring analysis from Gemini. Treat it as a second opinion: verify every proposed change against the exact XNU source, ARM semantics and linked instructions before applying it. The Gemini review of the commpage LL/SC conversion supports LDAXR/STXR and recommends CLREX on the busy-lock exit; that refinement is staged on fix/lavender-commpage-prebuild-guard, not yet merged into the active branch.
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

## Current task — resolve optional SHA2 assembly failure after commpage fix

**Run #255 result:** XNU run `43743932922`, source `24f81dc92b1b9bbe13a9cf15a0a5757ae5ed06ac`, failed during `Build XNU 8796`. The log confirms `0112-lavender-commpage-armv8a-llsc.patch` was applied; no `Skipping patch 0112` warning appears. The malformed-hunk/CASA blocker is therefore cleared at patch application.

**New exact blocker:** repeated `instruction requires: sha2` errors in `osfmk/arm64/corecrypto/sha256_compress_arm64.s` for SHA-256 crypto instructions. The compiler command targets `-march=armv8-a -mcpu=generic -arch arm64`. Do not globally enable `+crypto` without checking per-core hardware support.

**Artifact status:** raw candidate artifact `14174241207` was uploaded by an always-run diagnostic collection step. It is not verifier-approved. Kernel verification and the RAM-only package job were skipped; no #255 `iosboot.img` exists.

**Next:** inspect XNU 8796 corecrypto build selection for `CCSHA2_VNG_ARM`, the portable SHA-256 fallback, and whether the optimized assembly is built/selected unconditionally. Obtain `ID_AA64ISAR0_EL1.SHA2` evidence on both A53 and A73 cores; the prior MMFR0 16K-granule observation is not SHA2 evidence. If SHA2 is not present uniformly, exclude the accelerated assembly for Lavender and select a portable implementation. If it is supported uniformly, prove runtime dispatch before enabling only the required assembly feature. Do not patch by adding a global `-march=armv8-a+crypto` flag.

The staged commpage guard branch remains separate; do not merge it or launch a competing build until the SHA2 source-selection fix is evidence-backed. Keep the Run #240 mutex callback as a separate physical debugging lead. No handset retest until a power-button-independent recovery/reset route is physically verified; RAM boot only, never flash.

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


Run #257 (workflow `43789910990`, commit `c68f68c52bd3f227c0909e7728173632cebad27b`) failed in `Require Lavender XNU entry-guard patch` after the XNU build stage had completed. The exact fatal diagnostic was `unsupported executable LSE CASA remains in commpage assembly`. The guard stripped comments but scanned the entire `osfmk/arm/commpage/commpage_asm.s` file, so an unrelated executable CASA in another commpage routine caused a false positive; patch 0112's two target lock helpers were separately checked for LDAXR/STXR and CLREX. This is a verifier-scope bug, not evidence that the LL/SC conversion failed. The RAM-only package job was skipped, so no iosboot.img was produced. A later log line about the optional Xcode Kernel.framework PrivateHeaders/platform copy was non-fatal (`|| true`) and not the failure cause.
