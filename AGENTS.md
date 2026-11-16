# Lavender XNU — agent entrypoint

**Start here.** Read `PROJECT_HANDOFF.md`, inspect live branch HEAD and newest Actions runs, then open only the routed chapter. Resume autonomously from live evidence.

## Mission
Bring XNU 8796 and eventually iPhone X iOS 16.7.16 (20H392) Darwin userspace to Xiaomi Redmi Note 7/7S (`lavender`, Qualcomm SDM660 / Kryo 260, ARM64).

## Current milestone — 16 November 2026

## Latest operations — 16 November 2026

- **Run #274** ([43814890363](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814890363), source `5493d479039821941e0608a5e6191539eb11d52c`) **passed**: XNU build, entry-guard checks, linked Mach-O/kernel verification and RAM-only fastboot image packaging.
- **Run #275** ([43814903820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814903820), source `5b0d4121240d9573a10c3771afbef9f08e4a5729`) compiled XNU but failed at step 12, **Require Lavender XNU entry-guard patch**. The package job was consequently skipped.
- **Root cause:** the newly added source assertion used basic-regex `grep -q` for a literal C string containing parentheses. In basic regular expressions, the unescaped parentheses are grouping operators, so the pattern did not match the literal source. This is a false-negative CI guard, not an XNU compile or patch-application failure.
- **Correction:** use fixed-string matching: `grep -Fq 'lavender_console_string("LAVENDER XNU VERBOSE BOOT' dxb/xnu/osfmk/arm64/sweet_early.c`. Keep the assertion; do not remove the guard.
- **Acceptance:** #274 remains a valid green baseline. #275 is not a packaged-image acceptance. A fresh run after this correction must pass all guards, linked Mach-O verification and RAM-only packaging before its `iosboot.img` is accepted.
- **Visual caveat:** patch 0116 appends post-logo `[DARWIN]` trace messages through the verbose console without clearing the logo. Earlier pre-logo console lines are currently painted before the logo's full-panel clear and are not replayed over it. Do not claim the full early log is overlaid until that is implemented and checked. The overlay has CI evidence, not yet a separate physical handset confirmation.

- Run **#261** (`43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed XNU build/verify/package. User-supplied video `1000317742.mp4` shows physical Lavender reaching the logo-rendering stage with the #261-attributed image. The displayed mark is patch-0094's custom diagnostic approximation, not Apple's original; recording is not checksum-bound to the image.
- Run **#266** (`43802934997`) passed full XNU build, linked verifier and RAM-only package. Artifact `14197706518` contains `iosboot.img` (16,551,936 bytes, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`). It is not separately physically tested.
- The exact iPhone X iOS 16.7.16 (20H392) AppleLogo IM4P is extracted and hash-pinned: IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`; decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`. Patch 0114 integrates the original raster after `arm_vm_init()`.
- Run **#267** (`43804842131`) failed at AppleLogo preparation due escaped shell parameter expansion; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` corrected it.
- **Run #268** (`43805363291`) compiled XNU and passed AppleLogo asset/header, commpage, SHA2 and framebuffer source checks, then failed in a stale startup-trace verifier boundary: it searched for removed `lavender_logo_ellipse`. Linked verification and packaging were not reached.
- Commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` replaces that stale sentinel with the next function boundary `sweet_early_logo(void)`. Run #269 (`43809333900`, source `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664`) is now running the corrected verifier; at handoff refresh, the XNU build step is still in progress. A local check against #268's patched buildtree confirms the new function boundary and trace-scope assertions. Require linked verification and RAM-only packaging before accepting the authentic-logo image.

## Storage facts — Build 4 embedded FDT
- **iOS root target: internal eMMC via SDHC1**, base `0x0c0c4000`, 8-bit, non-removable, CQE/ICE resources. The intended HFS+ iOS root replaces Android's contents of the existing `userdata` partition; removable SDHC2 is not the planned root.
- SDHC2 at `0x0c084000` is a separate removable 4-bit card host (GPIO54, host IRQ125, power IRQ221, PM660L L5/L2, GCC SDCC2 clocks 93/94). It is not the root-storage plan.
- Topaz's `userdata` + HFS+ root deployment is the architectural reference, but Topaz uses UFS 3.0. Do not port its UFS transport to Lavender; implement the SDM660 SDHCI/eMMC path and validate SDHC1-specific clocks, rails, IRQs, CQE/ICE and reset state from Lavender sources.
- Topaz driver references: patches 0113–0130 show a Qualcomm modem/Wi-Fi path (PAS, GLINK/QRTR, modem host services, WLFW, WMI/HTT and WPA2 data path); patches 0092–0110 show display scanout/cache/brightness patterns; patches 0131–0139 show IOSurface/CoreAnimation integration. Treat these as architecture/algorithm references only. Topaz's README explicitly lists cellular data, Bluetooth, audio, camera, sensors and GPS as not working; it is not a completed cellular-driver reference.
- Lavender's public runtime dmesg identifies eMMC on `c0c4000` as an HS400 device and WCN3990 WLAN on the SNOC/ath10k path. Confirm all firmware, memory, IRQ, clock and pinctrl details from the exact Lavender DT/vendor sources before implementing XNU drivers. Topaz's FocalTech touch and Adreno 610 register work do not match Lavender's Novatek touch and Adreno 512.
- Initial SDHC1 bring-up is read-only: identify the host/card, read GPT and HFS+ metadata using bounded PIO. Only after this works should controlled writes be enabled for the designated `userdata` image. Never write other eMMC partitions.
- The Qualcomm MSM V5 power-event handshake and regulator/pinctrl ordering must be audited for SDHC1 too; do not assume the SDHC2 GPIO/IRQ/rail values apply to SDHC1.
- A 16 GiB logical HFS+ root cannot fit in the ~1.10 GiB remaining contiguous ADT RAMDisk aperture. The payload builder rejects oversized RAMDisks; use `userdata` as the persistent root target, not a RAMDisk.

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
