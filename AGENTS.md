# Lavender XNU — agent entrypoint

**Start here.** Read `PROJECT_HANDOFF.md`, inspect live branch HEAD and newest Actions runs, then open only the routed chapter. Resume autonomously from live evidence.

## Mission
Bring XNU 8796 and eventually iPhone X iOS 16.7.16 (20H392) Darwin userspace to Xiaomi Redmi Note 7/7S (`lavender`, Qualcomm SDM660 / Kryo 260, ARM64).

## Current milestone — 16 November 2026
- Run **#261** (`43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed XNU build/verify/package. User-supplied video `1000317742.mp4` shows physical Lavender reaching the logo-rendering stage with the #261-attributed image. The displayed mark is patch-0094's custom diagnostic approximation, not Apple's original; recording is not checksum-bound to the image.
- Run **#266** (`43802934997`) passed full XNU build, linked verifier and RAM-only package. Artifact `14197706518` contains `iosboot.img` (16,551,936 bytes, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`). It is not separately physically tested.
- The exact iPhone X iOS 16.7.16 (20H392) AppleLogo IM4P is extracted and hash-pinned: IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`; decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`. Patch 0114 integrates the original raster after `arm_vm_init()`.
- Run **#267** (`43804842131`) failed at AppleLogo preparation due escaped shell parameter expansion; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` corrected it.
- **Run #268** (`43805363291`) compiled XNU and passed AppleLogo asset/header, commpage, SHA2 and framebuffer source checks, then failed in a stale startup-trace verifier boundary: it searched for removed `lavender_logo_ellipse`. Linked verification and packaging were not reached.
- Commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` replaces that stale sentinel with the next function boundary `sweet_early_logo(void)`. Inspect the resulting build, linked verification and package run before accepting the authentic-logo image.

## Storage facts — Build 4 embedded FDT
- SDHC2 removable host: base `0x0c084000`, size `0x1000`, `qcom,sdhci-msm-v5`, enabled, 4-bit bus, card detect GPIO54, host IRQ125, power IRQ221, PM660L L5/L2 VMMC/VQMMC and GCC SDCC2 AHB/APPS clocks (IDs 93/94).
- Internal SDHC1 eMMC: `0x0c0c4000`, 8-bit, non-removable with CQE/ICE. Never use it for iOS root; preserve Android data.
- Qualcomm V5 power IRQ is part of the protocol: it services BUS_ON/OFF and IO_LOW/HIGH, switches regulator/pinctrl state and acknowledges the core. See Chapter 05.
- A 16 GiB logical HFS+ root cannot fit in the ~1.10 GiB remaining contiguous ADT RAMDisk aperture. The payload builder now rejects oversize images before reading them. Prefer a read-only removable SD root.

## Mandatory protocol
1. Resolve live branch HEAD and newest Actions runs.
2. Follow the single current chapter route; do not load every chapter.
3. Inspect exact source, FDT, CI logs/artifacts and physical evidence before edits.
4. Make the smallest evidence-backed fix; preserve all source and linked-binary guards.
5. Keep build, Mach-O verification, package, physical stages, genuine logo, root mount, launchd, daemons and SpringBoard as separate acceptance gates.
6. Update this file, `PROJECT_HANDOFF.md` and the routed chapter after meaningful milestones.

## Safety and truth
- Preserve Android/recovery. RAM boot only (`fastboot boot iosboot.img`); never flash, erase, repartition or write boot-critical partitions.
- The power button is broken. Do not request a handset retest until a power-button-independent recovery/reset route is physically verified.
- Volume Up + Volume Down is documented as a reset request, not a verified recovery selector.
- QEMU is not proof of Lavender compatibility.
- Never claim Darwin userspace, SpringBoard or a complete iOS boot without direct evidence.
- Do not import Sweet/Topaz UFS, Goodix, display, PMIC or other SoC-specific constants.
- Do not commit private dumps, serials, calibration data or firmware blobs.

## Active routes
- Current build/logo CI: `docs/handoff/03-current-debugging.md`.
- Genuine AppleLogo, post-logo kernel, SDHC2/root and Darwin userspace: `docs/handoff/05-post-logo-userspace.md`.
- Architecture/build mechanics: `docs/handoff/02-architecture-and-build.md`.
- Physical safety: `docs/handoff/01-device-and-safety.md`.
- Historical evidence: `docs/handoff/04-evidence-and-history.md`.

Repository: https://github.com/NAANG360/hackios-lavender-xnu
Branch: `ci/lavender-machine-profile`
