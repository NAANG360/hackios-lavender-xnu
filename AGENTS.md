# Lavender XNU — agent entrypoint

**Start here.** Read `PROJECT_HANDOFF.md`, inspect live branch HEAD and newest Actions runs, then open only the routed chapter. Resume autonomously from live evidence.

## Mission
Bring XNU 8796 and eventually iPhone X iOS 16.7.16 (20H392) Darwin userspace to Xiaomi Redmi Note 7/7S (`lavender`, Qualcomm SDM660 / Kryo 260, ARM64).

## Current milestone — 16 November 2026

Run #297 is the latest accepted diagnostic package. See the update immediately below for exact artifact identity and next action.



## Latest accepted operations — Run #297 (16 November 2026)

- Run #297 ([43853989024](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43853989024), commit `fd40a4953708f22e7df492bef699e657dade93ea`) passed both XNU build/verification and RAM-only packaging jobs.
- Artifact `14219764264` (`lavender-fastboot-boot-test`) contains `iosboot.img` (16,601,088 bytes), SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`.
- Package manifest confirms the genuine 20H392 AppleLogo raster. Patch 0118 supplies stage 18 after `PE_consistent_debug_inherit()` and stage 19 after `serial_init()`; stage 20 is KPRINTF startup completion.
- New physical clip `1000317779.mp4` shows the shim's A73/16K handoff, the AppleLogo screen and four accumulated lower-edge green bars. Source maps the four bars to `sweet_early_mark` stages 17–20 (logo complete, PE consistency returned, `serial_init` returned, KPRINTF startup complete), strongly indicating stage 20. The clip is not bound to Run #297's SHA-256; do not claim exact-image validation, BSD/root mount, launchd, SpringBoard or a complete iOS boot.
- Next route: `docs/handoff/03-current-debugging.md` for the exact-image marker result; Chapter 05 for subsequent BSD/root/userspace interpretation.
- Physical test remains gated on a verified power-button-independent recovery/reset route. RAM-only `fastboot boot iosboot.img`; never flash.

## Earlier operations — 16 November 2026

- **Run #281** ([43819847178](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43819847178), source `94c2366567aee5df21435a7c4bb91041f161305e`) compiled XNU but failed the required-patch guard. Its log explicitly says 0115 is corrupt at EOF line 161 and 0116 at EOF line 46; verification and packaging were skipped.
- **Root cause:** each patch had a trailing whitespace-only context line after its final hunk, and each final hunk header overstated its body by one old and one new line. The true final counts are 0115 `6/12` (not `7/13`) and 0116 `28/21` (not `29/22`).
- **Fix committed:** `ab90b5840b891ee2894e2606f8b0e45b83ef8bd4` and `857e735a046c76c9889ddac429583b4a24e03706` remove the stray EOF context lines; `f84b0c43b94c322bb300bf57b2fde7329fce8ca5` and `596bf0c9690e468ab188d0008e7e9495108c6b11` correct the final hunk counts.
- **Patch validation:** every hunk in 0115 and 0116 was independently counted; all declared old/new counts match the actual bodies, with no malformed lines.
- **Run #285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), source `596bf0c9690e468ab188d0008e7e9495108c6b11`) is the first run on the fully corrected patch set. It has reached **Build XNU 8796**; patch application, source guards, linked Mach-O verification and the separate RAM-only package job remain mandatory acceptance gates.
- Runs #282–#284 were triggered by intermediate repair commits; use #285 as the acceptance run.
- **Visual status:** the genuine AppleLogo itself was physically observed in an earlier handset test. The post-logo [DARWIN] overlay is not yet accepted in a packaged image or physically verified. Pre-logo console replay over the logo remains a separate gap.

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


## Latest CI correction — Run #286 (16 November 2026)

- **#285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), commit `596bf0c9690e468ab188d0008e7e9495108c6b11`) compiled XNU but failed the required-patch guard because `0115-lavender-post-logo-darwin-trace.patch` was skipped. The build log gives the actual apply errors: `osfmk/kern/startup.c:744` and `bsd/kern/bsd_init.c:1044`. The earlier “only malformed hunk counts” diagnosis was incomplete; those hunks also used stale source context relative to the actual XNU tree after the preceding Lavender patches.
- **Corrective change:** regenerated patch 0115 against the exact post-0106…0114 source tree extracted from #285's `xnu-buildtree` artifact. The new unified diff includes the actual context around scheduler startup, IOKit, BSD bootstrap, VFS initialization, root mount, and `bsd_utaskbootstrap`. It was locally validated with `patch -p1 --dry-run` against that exact build tree: all three files checked successfully.
- **#286** ([43825403975](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43825403975)) is the fresh validation run for commit `b1b434dfc9d3668bbea9a4704a85a324ff0f9887`. It is in progress at the time of this update. Require 0115 and 0116 not to be skipped, then require kernel verification and RAM-only package completion. Do not distribute its candidate unless the full run is green.
- **Important correction to prior reports:** #281–#285 were not valid green builds for the new trace changes. The reliable previously packaged test image remains #274. The 0115 patch fix is now tested against the actual XNU source context rather than by hunk-count lint alone.


## Latest CI correction — Run #290 (16 November 2026)

- Run #289 ([43826819680](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43826819680), source `ed8941c52741e510c2035033a6e109c1e155f332`) compiled XNU but failed the required-patch guard: patch 0115's first `startup.c` hunk did not apply at line 658. Linked verification and packaging were skipped.
- Root cause was four comment-context lines with an extra tab in `xnu/patches/0115-lavender-post-logo-darwin-trace.patch`. Reproduced against Run #289's buildtree artifact: original hunk failed; normalized context passed `patch -p1 --dry-run`.
- Fix commit: `e2beb8e297c498def1c09fd2a32a5b0241c86102`; only patch context changed, not XNU behavior.
- Run #290 ([43829764065](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43829764065)) validates the correction. Do not accept raw candidates; require patch guards, linked Mach-O verification and RAM-only package success.
