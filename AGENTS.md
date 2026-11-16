# Lavender XNU — agent entrypoint

**Start here.** Read `PROJECT_HANDOFF.md`, inspect the live branch and newest Actions runs, then open only the routed chapter. Resume autonomously from live evidence.

## Mission
Bring XNU 8796 (iOS 16 generation) and ultimately the iOS 16.7.16 (20H392) Darwin userspace to the Xiaomi Redmi Note 7/7S (`lavender`, SDM660 / Kryo 260, ARM64).

## Current milestone — 16 November 2026
- XNU workflow **#261** (run `43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed both kernel build/verification and RAM-only image packaging.
- Packaged image artifact: `lavender-fastboot-boot-test`, ID `14193115710`; exact file `iosboot.img`, 16,551,936 bytes, SHA-256 `ee96e9273a4699ca1da78d85e761cc8947ee1c520757fe2f87d2c314d9ea445c`.
- User-supplied recording `1000317742.mp4` (20.317 s) shows the red XNU diagnostic display, then a white Apple-shaped silhouette on the phone's black display for the remainder of the clip. The user identifies this as the Run #261 image. This is the first reported physical Lavender logo-stage result. The video has no device-side image checksum and shows no Darwin userspace.
- **Logo correction:** patch `0094-lavender-logo-and-stage-markers.patch` implements a hand-drawn diagnostic approximation using ellipses, a bite, leaf and stem. It is not Apple's actual boot-logo asset. The user explicitly wants the genuine Apple logo artwork. Keep the successful kernel/startup path; replace only the artwork after extracting and validating the AppleLogo resource from the matching iPhone X iOS 16.7.16 IPSW.

## Live CI
- Run **#262** (`43795080764`, source `71bb3a0dc0a45a7d5482fe45de869af6281bca41`) compiled XNU successfully, then failed the Lavender entry guard; verification and packaging were skipped.
- Source-level cause: the inline guard used an explicit next symbol for enqueue but `None` for dequeue, causing the second body scan to extend to end-of-file and potentially reject unrelated later symbols.
- Corrective commits: `276fb31395998538afcbb620d25c647ad9f5f47e` updates the reusable guard to require CLREX on the busy-lock return path; `b6879a835d61a3382740737254a5c54b2eeeb2ee` makes the workflow use the symbol-bounded validator.
- Run **#263** (`43800607832`) and **#264** (`43800747274`) passed the source-level generic-ISA, commpage LL/SC+CLREX and SHA2-scope checks but failed an unconditional compiler-log grep on exact cache hits.
- Commit `f3526da7461c804188f7395ad964cda9c80542da` made that grep cache-aware without relaxing source or linked-binary validation.
- **Run #265** (`43801366044`, source `f3526da7461c804188f7395ad964cda9c80542da`) passed XNU build, source guards, linked Mach-O verification and RAM-only package validation. Artifact `14196716179`, exact `iosboot.img` SHA-256 `c1dfc841fab6545e0c4892c7f01dba4ecd7c52f972e9ff7c00ca6230f6a8537d`. Its kernel is identical to #261; its per-run ADT seed changes the full image hash.
- **Next:** replace the hand-drawn logo with the exact AppleLogo resource; then determine the post-logo kernel frontier and prioritize removable SDHC2/root storage. Do not run another handset test until the power-button-independent recovery path is physically verified.

## Mandatory protocol
1. Resolve live branch HEAD and newest relevant Actions run.
2. Follow the single current chapter route; do not load every chapter.
3. Inspect exact XNU source, workflow logs/artifacts and physical evidence before edits.
4. Make the smallest evidence-backed fix; preserve source and binary guards.
5. Keep build, Mach-O verification, package, physical kernel stages, logo, root mount, launchd and SpringBoard as separate acceptance milestones.
6. After meaningful milestones update this file, `PROJECT_HANDOFF.md`, and the affected chapter.

## Safety and truth
- Preserve Android and recovery. RAM boot only (`fastboot boot iosboot.img`); never flash, erase, repartition or write boot-critical partitions.
- The power button is broken. Do not request a handset test until a power-button-independent recovery/reset route is physically verified.
- The volume-up + volume-down chord is documented as a reset request, not a verified recovery selector.
- Do not use QEMU as proof of Lavender compatibility.
- Never claim Darwin userspace, SpringBoard or a complete iOS boot without direct evidence.
- Do not import Sweet/Topaz UFS, Goodix, display, PMIC or other SoC-specific constants into Lavender without SDM660 evidence.
- Do not commit private dumps, serials, calibration data or firmware blobs.

## Active routes
- Current CI guard / build acceptance: `docs/handoff/03-current-debugging.md`.
- Logo replacement, post-logo kernel, removable SDHC2/root storage and Darwin userspace: `docs/handoff/05-post-logo-userspace.md`.
- Architecture/patch/build details only as needed: `docs/handoff/02-architecture-and-build.md`.
- Before any physical-device action: `docs/handoff/01-device-and-safety.md`.
- Historical run and provenance details: `docs/handoff/04-evidence-and-history.md`.

Repository: https://github.com/NAANG360/hackios-lavender-xnu
Branch: `ci/lavender-machine-profile`
