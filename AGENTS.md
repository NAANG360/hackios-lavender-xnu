## Latest accepted CI image — Run #400 (20 November 2026)

- **Branch:** `ci/lavender-machine-profile`; source commit `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`.
- **Workflow:** [Run #400 / 44706182280](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280). Both jobs passed: XNU 8796 build/source verification/kernel artifact verification and downstream RAM-only fastboot packaging.
- **Image artifact:** `lavender-fastboot-boot-test`, artifact ID `14615937969`; `iosboot.img`, **16,621,568 bytes**.
- **Image SHA-256:** `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`. Independently extracted from the artifact ZIP and matched `BUILD-MANIFEST.txt` / `SHA256SUMS`.
- **Kernel SHA-256:** `24bfe515b6e6bfab70f53602a00384cdcc8f76fe3c7505916b7021fc6c33bb4e`; **payload SHA-256:** `8f3fdd5fc187b7ac18691af5952d4b0228f011ac95fa2cd1de3a327532bf7cf2`; **DTB SHA-256:** `bcb3848e080bcf058f0b32ebf2ec440b6f638fed144c182766aa80a45bb9bb22`.
- **Bootargs:** `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. Keep `cpus=1`; physical test only with `fastboot boot iosboot.img`.
- **QUP source/build milestone:** native `LavenderQUPI2C` compiled and linked into the ARM64 Mach-O kernel. The ADT preflight, QUP protocol host test, QUP source guard, IOKit/early-init guards, independent kernel verification, image construction and payload identity checks all passed. The lifecycle follow-up (balanced workloop retain and interrupt-source removal) is included in this run.
- **Limitations:** this is not proof of a working physical I2C bus. The current QUP implementation is FIFO/tagged mode only; block mode for the Novatek 66-byte report plus two-byte QUP input tag (68 bytes total), TLMM pinctrl for QUP GPIO2/3, bus-clear recovery, and a native `vdd_dig` vote remain to be implemented. No SDHCI/eMMC, Novatek input, or MDSS/DSI panel driver is yet accepted. No physical test has been performed.
- **Interpretation:** Run #400 proves source/build integration, ARM64 kernel linkage and a structurally verified RAM-only image. It does **not** prove physical boot, QUP transaction success, eMMC reads, touchscreen input, display output, BSD init, root mount, launchd, SpringBoard, calling or iMessage.

---

## Latest accepted diagnostic image — Run #358 (18 November 2026)

- **Latest accepted image:** [Run #358 / 44339488789](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44339488789), built from source commit `f1f633f9ac1887736f17937c38eb6c32de489160`. XNU build/verification and downstream RAM-only package jobs both passed.
- **Artifact:** `lavender-fastboot-boot-test`, ID `14445642030`; extracted `iosboot.img` is **16,617,472 bytes**, SHA-256 `b6361d9d03147570fb0a5407413d1b25c623d77c20649bcba63c35a3426f508f`. The downloaded artifact's `SHA256SUMS` passes and matches `BUILD-MANIFEST.txt`. Artifact ZIP digest: `723a50d977358f356febe994429fcaf74cfcf85d0c652e6faa3fdf8348ef3eb5`.
- Patch 0127 root-nub events 126–132 passed patch/source guards, splitter/label checks, IOKit verifier, XNU 8796 compile, linked ARM64 Mach-O verification and image packaging.
- Bootargs: `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. Preserve `cpus=1`; the no-fbcon argument gates the ordinary `fbcon_probe()` path only, not direct early framebuffer checkpoints.
- This is a verified RAM-only diagnostic package, not a complete Darwin/userspace image. A fresh physical recording `1000318056.mp4` has been reviewed, but it is not checksum-bound to this hash; do not call it an exact Run #358 test.
- The recording shows shim-to-XNU handoff, `ARM_INIT` console/platform milestones and generic post-platform event rows clearly readable through event 106. The later lower-console rows are obscured/blurred; events 126–132 cannot be transcribed reliably. Do not infer root-nub success/failure or attribute the footage to Run #358 until the host-side `iosboot.img` SHA-256 is confirmed.
- Physical test only via `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/`userdata`/any partition. Preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard or complete iOS userspace without direct evidence.
- Physical test only via `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/`userdata`/any partition. Preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard or complete iOS userspace without direct evidence.

---

# Lavender XNU — agent entrypoint

**Start here.** Inspect the live branch HEAD and newest Actions runs, then open only the routed chapter. Use the handoff chapters as the active engineering record; do not consult `PROJECT_HANDOFF.md` unless the user explicitly asks.

## Mission
Bring XNU 8796 and eventually iPhone X iOS 16.7.16 (20H392) Darwin userspace to Xiaomi Redmi Note 7/7S (`lavender`, Qualcomm SDM660 / Kryo 260, ARM64).

## Current milestone — 20 November 2026

- Run #400 ([44706182280](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280), source commit `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`) passed both XNU 8796 build/verification and the downstream RAM-only packaging job.
- Artifact `14615937969` (`lavender-fastboot-boot-test`) contains `iosboot.img`, **16,621,568 bytes**, SHA-256 `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`. Manifest kernel SHA-256: `24bfe515b6e6bfab70f53602a00384cdcc8f76fe3c7505916b7021fc6c33bb4e`.
- The native `LavenderQUPI2C` IOKit translation unit is compiled and linked into the ARM64 Mach-O. QUP protocol host tests, ADT checks, source guards, IOKit platform-match/early-init checks, independent kernel verification and RAM-only image structure/payload-identity checks all passed.
- This is a **source/build milestone**, not physical peripheral verification. The QUP driver currently implements FIFO/tagged transactions, repeated-start construction, interrupt completion and bounded error/timeout handling, and publishes a native transfer interface. The lifecycle correction in Run #400 balances workloop ownership and interrupt-event-source removal.
- Remaining QUP work before calling the bus functional: block-mode transfer path for the 68-byte Novatek read envelope when FIFO capacity is insufficient; TLMM pinmux for QUP GPIO2/3; bus-clear recovery; native `vdd_dig` clock/voltage vote; runtime transaction instrumentation and physical verification.
- Linux reference commit `da9b19cb9ec452281cbe1270c5e733e20d69475a` is pinned. The verified NVT source reads `POINT_DATA_LEN + 1 = 66` bytes; the QUP input tag adds two bytes. GPIO67 is a TLMM interrupt (DTS flags `0x2008`), not a direct GIC interrupt.
- No SDHCI/eMMC, Novatek HID/input, or MDSS/DSI/Tianma panel implementation is yet accepted. No physical test has been performed. Do not infer storage, touch, display, BSD init, root mount, launchd or SpringBoard operation from CI.
- Physical test remains RAM-only via `fastboot boot iosboot.img`; preserve `cpus=1`, volume-key recovery and Android partitions. Never flash, erase, format, repartition or write any partition.

## Previous accepted operations — Run #300 (superseded by Run #302)

- **Run #300** ([43891112657](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43891112657), source commit `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`) completed successfully. Both the XNU build/verification job and the separate RAM-only packaging job passed.
- Build job: XNU 8796 compilation, expected cache-aware build-path guard, Lavender entry-guard patch check, and kernel artifact verification all passed. Package job: matching kernel retrieval, Mach-O presence, OTA/DTB extraction, Lavender ADT generation, payload construction, fastboot image creation, image-structure/payload-identity verification, and artifact upload all passed.
- Artifact **`14236569374`** (`lavender-fastboot-boot-test`) contains `iosboot.img`, **16,601,088 bytes**. Extracted image SHA-256: **`772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef`**.
- The artifact manifest identifies source commit `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`, genuine iPhone X iOS 16.7.16 (20H392) AppleLogo raster (194×238), target Redmi Note 7/lavender/SDM660, and fastboot-boot-only mode. The package is a verified CI image, not evidence of a complete Darwin userspace boot.
- Runs #298 (`43890854367`) and #299 (`43890920180`) failed at the required-patch guard after XNU compilation; their linked verification and packaging were skipped. Run #300 is the succeeding full build + verify + package acceptance run for the corrected source state.
- The user's clip `1000317779.mp4` shows the XNU handoff, genuine AppleLogo, and four lower-edge green markers consistent with simple stages 17–20. It is not checksum-bound to this Run #300 image. Do not claim the clip proves this exact image reached KPRINTF, BSD init, root mount, launchd, SpringBoard, or complete iOS userspace.
- Run #300's artifact manifest is stale on recovery status. The user confirms the power-button-independent volume-key recovery/reset route works; the physical test recovery gate is cleared. The Guardian watchdog-first fallback remains separately unverified.
- **Next:** preserve this artifact as the newest accepted diagnostic image. Continue Chapter 03's post-KPRINTF trace investigation, then use Chapter 05 only when interpreting later BSD/root/userspace markers. RAM-only `fastboot boot iosboot.img`; never flash.

## Latest accepted operations — Run #297 (16 November 2026)

- Run #297 ([43853989024](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43853989024), commit `fd40a4953708f22e7df492bef699e657dade93ea`) passed both XNU build/verification and RAM-only packaging jobs.
- Artifact `14219764264` (`lavender-fastboot-boot-test`) contains `iosboot.img` (16,601,088 bytes), SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`.
- Package manifest confirms the genuine 20H392 AppleLogo raster. Patch 0118 supplies stage 18 after `PE_consistent_debug_inherit()` and stage 19 after `serial_init()`; stage 20 is KPRINTF startup completion.
- New physical clip `1000317779.mp4` shows the shim's A73/16K handoff, the AppleLogo screen and four accumulated lower-edge green bars. Source maps the four bars to `sweet_early_mark` stages 17–20 (logo complete, PE consistency returned, `serial_init` returned, KPRINTF startup complete), strongly indicating stage 20. The clip is not bound to Run #297's SHA-256; do not claim exact-image validation, BSD/root mount, launchd, SpringBoard or a complete iOS boot.
- Next route: `docs/handoff/03-current-debugging.md` for the exact-image marker result; Chapter 05 for subsequent BSD/root/userspace interpretation.
- The user confirms the power-button-independent volume-key recovery/reset route works; that gate is cleared. RAM-only `fastboot boot iosboot.img`; never flash.

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
6. Update this file and the routed chapter after meaningful milestones. Leave `PROJECT_HANDOFF.md` untouched unless the user explicitly asks.

## Safety and truth
- Preserve Android/recovery. RAM boot only (`fastboot boot iosboot.img`); never flash, erase, repartition or write boot-critical partitions.
- The power button is broken. The user confirms the independent volume-key recovery/reset route works; handset tests are no longer blocked by the recovery gate.
- The volume-key recovery/reset route is confirmed working by the user; do not downgrade it to a reset-only path.
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


## Post-KPRINTF trace CI hardening — 16 November 2026

- Run #297 remains the latest accepted image: artifact `14219764264`, `iosboot.img` SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`. The physical clip `1000317779.mp4` strongly correlates with simple stages 17–20, but is not checksum-bound to this image; do not claim BSD/root/userspace.
- Added workflow source guards for the distinct `sweet_early_trace_startup` event namespace 17–29: ordered scheduler, IOKit, early-boot, interrupt, BSD-init, VFS, root-mount and utask-bootstrap boundaries; plus the transparent `[DARWIN]` console renderer and post-logo header ordering.
- Change commit: `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`. The guard was executed locally against the exact patched XNU source extracted from Run #297's `xnu-buildtree` artifact and passed. A fresh Actions run is the next acceptance gate; no new image is accepted until XNU build, linked Mach-O verification and RAM-only package all pass.
- The volume-key recovery/reset route is confirmed working; the recovery gate is cleared. Never flash; RAM-only fastboot boot only.


## Current continuation checkpoint — Run #300 verified (16 November 2026)

- Latest accepted image remains Run #300 / artifact `14236569374`, `iosboot.img` SHA-256 `772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef` (16,601,088 bytes). It has been independently downloaded and hash-checked.
- The matching buildtree artifact `14238555047` confirms the compiled XNU contains the [DARWIN] startup trace renderer and event markers 17–29. Next debug boundary is scheduler begin/done (17/18), followed by IOKit (19/20), early boot/interrupts (21/22), BSD/VFS/root (23–27), and utask bootstrap (28/29).
- Instrumentation presence is not physical execution evidence. Do not claim BSD/root/launchd/SpringBoard. The latest user recording is not bound to Run #300's checksum.
- Continue through Chapter 03. The user-confirmed volume-key recovery route clears the prior physical-test gate. RAM-only `fastboot boot iosboot.img`; never flash.

## Current diagnostic correction — Run #300

- Read Chapter 03 before changing startup tracing. Event 17 is after idle_thread_create, not at bootstrap-thread entry.
- Patch 0119 separates load_context entry (33), bootstrap-thread entry (30), idle_thread_create begin/done (31/32), and preserves scheduler begin/done (17/18). Require Run #302's full build and package acceptance before distributing an image.
- Chapter 01's independent volume-key recovery/reset route is confirmed working; the recovery gate is cleared. Never flash.


## Current engineering update — 16 November 2026

- Run #302 is green through XNU build, linked verification and RAM-only package. Artifact `14245608682`: `iosboot.img`, 16,601,088 bytes, SHA-256 `2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98`.
- Patch 0120 adds post-stage-20 `arm_init()` events 34–39, true `kernel_bootstrap()` events 41–46, Lavender early panic text, and first kernel synchronous-exception ESR/FAR/ELR/SPSR output. Local exact-buildtree apply and source-order checks passed.
- Workflow commit `dc0603b6e6a64e117b04b447f82de37ecc36ed3f` adds required-patch and source guards. Run #303/#304 were started before the 0120 hunk correction; preferred guarded run #305 includes the corrected patch.
- Recovery path is confirmed working by the user; physical testing is no longer blocked by that gate. Use only accepted images with RAM-only `fastboot boot iosboot.img`; never flash.
- The final FASTBOOT frame in the latest recording is a video glitch. The handset restarts normally; reset cause is unknown.

- **Patch-format correction:** commit `b457583a2e8b6d42406bf44d692c612225d0c88d` fixes 0120's `kernel_bootstrap()` hunk count. All 16 hunk counts now validate, and 0119+0120 apply cleanly to the exact Run #300 buildtree. Run #305 (`43910208977`) is the preferred acceptance run.

## Lavender early-source split checkpoint — 17 November 2026

- The user requested a dedicated Lavender early console implementation rather than continuing to keep Lavender code inside SWEET's source file.
- tools/split-lavender-early.py now separates the Lavender branch from the patched XNU source after the full patch stack: generated osfmk/arm64/lavender_early.c and lavender_early.h, SWEET-only sweet_early.c, compatibility aliases in sweet_early.h, and a files.arm64 entry for lavender_early.c.
- .github/workflows/xnu.yml invokes the splitter before compilation, invalidates the generated source in the build path, hashes the generator in the cache key, and checks the split, board routing, Lavender source symbols and source-list registration.
- Current branch HEAD at this checkpoint: 22590d814dc0770b2740d413b52600e0c32d622e (ci/lavender-machine-profile). Runs #307–#310 were triggered by incremental workflow commits; #310 is the intended latest guarded validation candidate. They are in progress at the time of this note; do not accept an image until one complete run passes XNU build, source guards, linked Mach-O verification and RAM-only package.
- Run #306 failed during XNU compilation; its raw kernel candidate is not accepted. Run #302 artifact 14245608682 remains the latest accepted diagnostic image, SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98.
- The source split preserves Lavender's verified framebuffer geometry and all current logo, guard, checkpoint, panic and trace behavior. It does not import Sweet's framebuffer or GIC/timer assumptions.
- Continue only with RAM-only fastboot boot iosboot.img. Never flash or write boot-critical partitions.


## Latest Lavender early-source split correction — Run #318 / Run #320 (17 November 2026)

- Run #318 (44016311664, source 9041148e40462d7eea7a32b9fd176f7ec29d0f28) compiled its translation units but failed at the final Mach-O link for kernel.development.lavender.unstripped: unresolved _sweet_early_trace_startup references remained in startup.o, lock_mtx.o, lock_group.o and related objects.
- Cause: the splitter renamed the implementation to lavender_early_trace_startup and the macro redirect lived in sweet_early.h, but several patched callers declare the old symbol directly without including that header. Those callers therefore retain the original ABI symbol at link time.
- The same run's Step 13 source guard still searched lavender_early.c for old sweet_early_* function names and the old single-line putc signature. After the split, these definitions are lavender_early_* and lavender_early_putc(char c) is formatted across lines.
- Fix commit: 0abe1270618c7a5d3a303dd1c2603e107c94bf26 (fix: preserve Lavender trace ABI and update source guards). tools/split-lavender-early.py now appends an ABI-compatible sweet_early_trace_startup forwarding wrapper that calls lavender_early_trace_startup. The wrapper is also ensured in both already-split/idempotent paths, so an existing generated source cannot skip the compatibility symbol.
- .github/workflows/xnu.yml Step 13 now slices the generated source using lavender_early_guard_init, lavender_early_mark, lavender_early_putc(char c), lavender_early_logo and lavender_early_trace_startup. The duplicate checkpoint assertion was removed.
- Python syntax and YAML parsing passed before commit. No other patched caller files needed modification because the original ABI symbol is provided by the wrapper.
- Run #320 (44020121980) is the validation run for commit 0abe1270618c7a5d3a303dd1c2603e107c94bf26. At the latest inspection, runner setup, Xcode/tooling selection, machine-profile validation, cache restore, capacity detection, dependency install and legacy-cache invalidation passed; Build XNU 8796 is in progress. Step 12/13 guards, linked Mach-O verification, artifact collection and RAM-only package are still pending.
- No new iosboot.img is accepted. Run #302 artifact 14245608682 remains the latest accepted diagnostic image (16,601,088 bytes; SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98). Raw candidates from failed runs are not bootable acceptance artifacts.
- Next: monitor Run #320 through Step 13, linked verification and package verification. If the linker still reports the old symbol, inspect the generated lavender_early.c in the buildtree and confirm the wrapper is present exactly once and exported as a global symbol. Keep all device tests RAM-only; never flash.


## Proactive GIC and CPU topology hardening — 17 November 2026

- Latest accepted diagnostic image remains Run #331 / artifact `14325018991`, `iosboot.img` SHA-256 `f2b2bb70d236213f02fb8b3e25780a6b337ea8de26890c24ae11bb85a1fd806b`.
- Run #332 was cancelled before build completion; do not use its candidate. Run #333 (`44086398531`) is still executing its XNU build as of last check; its job has no recorded failure, but updated timestamps are stale.
- Commit `0fb0e5c300b393bc4b5323783c930f0a38e9b958` adds patch0125 (comments only) for GICR security banking and updates the GIC verifier so DS=0 firmware preconditions and DS=1 conditional paths are guarded. It also adds ADT `cluster-type` E/P metadata for the two Kryo260 clusters and a regression test run before XNU compilation.
- The current physical boundary is event87 after GICR MMIO mapping. The fix for the future Group-1 timer delivery path does not explain that stop. Events88–105 remain the immediate diagnosis path.
- Keep `cpus=1`; preserve RAM-only `fastboot boot iosboot.img`. Never flash, repartition, erase or write Android boot-critical partitions/userdata.


## eMMC ADT contract preflight — 17 November 2026

- Commit `474abdc2daa0070251ac137d8571bd0f45eec4f9` strengthens both `mkadt-lavender.py` self-validation and `tests/test_lavender_adt.py`: exact three SDHC1/eMMC register windows, six interrupt cells and names, all three clocks, 8-bit width, non-removable flag, and enabled status are checked against the Build 4 hardware profile.
- This only validates the ADT descriptor. The read-only SDHC1/eMMC probe, Qualcomm V5 power-state handshake, bounded PIO data path, GPT validation, BSD block device and HFS+ read-only root remain unimplemented. Do not claim they are solved or add write/DMA/CQE/ICE code without separate hardware evidence.


## Physical footage review — corrected root-nub boundary (18 November 2026)

- The two supplied videos are reported by the independent review to show the same physical boot session. Their exact tested image SHA-256 is not established; do not attribute the footage to Run #357.
- Correct the earlier transcription: checkpoint [0084] reads **`IOKIT ROOT NUB ALLOCATED`**, not `IOKIT ROOT MAP ALLOCATED`.
- Do not treat the Gemini-proposed `initWithArgs()`, workloop creation, `registerService()`, or platform-expert matching as the operation immediately after event 120. In the actual Run #357 buildtree, the next call is `gRootNub->init(dtTop)`; that enters `IOPlatformExpertDevice::init(dtRoot)`, which calls `IODeviceTreeAlloc(dtRoot)` then `super::init(dt, gIODTPlane)`, or fallback `super::init()`. Workloop setup and `registerService()` occur later in `startIOServiceMatching()`.
- The current Run #357 renderer's event 107–125 strings do not all match the strings transcribed in this older clip. Keep it an unbound physical observation. `lavender_no_fbcon=1` gates ordinary `fbcon_probe()`; it does not disable the independent direct-framebuffer early checkpoint renderer.
- Follow-up patch 0127 adds event 126–132 around the root-nub init, device-tree allocation, registry superclass init, and fallback branch. It is required by the XNU workflow; the updated verifier checks brackets and trace-label mapping. Existing panic and synchronous-exception diagnostics are already present; do not add duplicate hooks.
- Patch application passed a dry-run against the clean Run #357 XNU buildtree. Full CI compile, linked verification, package and physical test remain outstanding. Keep the acceptance gates separate.
- Preserve `cpus=1`; RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition, or write Android eMMC/userdata; preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard or complete Darwin userspace without direct evidence.


## ROM preservation rule — LIOS Lavender driver work (21 November 2026)

The user's confirmed-booting Android 15/Lucid ROM is a read-only hardware reference. Do not alter its drivers, kernel modules, DTB, vendor image, boot image or Android configuration. Port observed hardware behavior into native XNU implementations in this repository only. The last accepted XNU baseline is Run #400; newer source changes must remain labelled unaccepted until their own CI build, linked ARM64 verification, package integrity checks and (where relevant) physical hardware test are complete.

## Working QUP source follow-up — 21 November 2026

- **Latest accepted CI remains Run #400** (`44706182280`, source `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`, artifact `14615937969`). Do not call later source commits accepted until their own XNU compile, linked ARM64 verification, RAM-only package verification and physical checks pass.
- **Latest QUP source checkpoint:** `b94e90b480cb51121776e663c5a369afe3dd56ce`; current handoff update: `aae69bfa476d1e446905349a43496d6e1c0b171f`.
- The follow-up extends native QUP FIFO/BLOCK counters, 68-byte Novatek block reads and the four-byte tail drain, TLMM GPIO2=SDA/GPIO3=SCL pin states, bounded nine-pulse recovery, a 100ms BUS_ACTIVE STOP fence, saved pre-reset failure status, bounded transfer timeout and first-success/first-error runtime diagnostics.
- Host protocol tests and source guards have been extended, but their execution and the new XNU build have **not** yet been verified. Run #400 remains the only accepted compiled/linked/package artifact.
- The confirmed-booting Android 15/Lucid ROM remains read-only. Do not modify its kernel modules, vendor drivers, DTB, boot image, vendor partition or Android configuration.
- QUP's `vdd_dig` vote remains a hard dependency: the pinned SDM660 path uses the RPM `rpm_requests` service over GLINK/RPMSG, with `pm660l_s3_level` / `rwcx` resource, ID 0 and `vlvl` KVP. Do not substitute a guessed PMIC MMIO write or assume a voltage vote survives RAM-only boot.
- The QUP driver is statically compiled into the XNU kernel and matched/started by IOKit; it is not a `.kext`. It publishes `lavender-i2c-transfer`, but no Novatek client has yet been accepted or observed making a real transfer. Do not claim the provider is physically used until that client transfer is evidenced.
- Follow Chapter 05 for the detailed reference-source audit, QUP state and the RPM/GLINK dependency. Preserve RAM-only `fastboot boot iosboot.img`, `cpus=1`, volume-key recovery and all Android partitions.


## Native RPM/GLINK rpm_requests bring-up rules — 21 November 2026

- Keep the confirmed-booting Android 15/Lucid ROM read-only. Use it and the pinned Lavender Linux tree only as hardware-behavior references; never modify ROM drivers, modules, DTB, boot image or Android partitions.
- The current LIOS source implements the SDM660 qcom,glink-rpm transport over RPM message RAM and APCS mailbox, then opens the remote-created GLINK/RPMSG rpm_requests channel. This is a native XNU endpoint for the one RPM service, not transplanted Linux code and not a generic GLINK framework.
- Use the pinned hardware values and protocol constants recorded in Chapter 05. In particular, RPM resource type is the little-endian resource-name string rwcx, resource ID 0; the vlvl KVP value 64 is LOW_SVS. Do not confuse resource-name encoding with the regulator-type enum.
- QUP must not enable its GCC clock until a matching successful RPM ACK confirms the LOW_SVS active-set vote. If the channel, GLINK handshake, or RPM ACK is unavailable, fail closed. Do not write PMIC registers directly.
- The first implementation uses bounded polling and retains the active LOW_SVS floor. The pinned regulator DTS enables active and sleep sets, but Linux sends a sleep-set vote only when its aggregate differs from the active vote; do not add an unnecessary sleep request while both are LOW_SVS. Do not add an unvote or independent per-driver voltage writes. Future RPM consumers must share a regulator-vote aggregator.
- The RPM/GLINK follow-up is source-only until its own host tests, source guard, XNU 8796 compile/link, RAM-only package checks and physical test pass. Run #400 remains the latest accepted baseline.
- Batch related GitHub changes into one atomic tree/commit. The workflow now uses one concurrency group per branch with cancel-in-progress enabled, so a newer source commit supersedes earlier runs created under the new group. Legacy runs created with the former per-run groups remain unaffected; preserve the accepted Run #400 artifact and do not claim those legacy runs were cancelled.
- Read Chapter 05 for exact transport, message layout, source references, implementation state and acceptance gates.


## RPM/GLINK run 448714 failure checkpoint — 21 November 2026

- Latest RPM/GLINK source is `a74f05bfcfc82becccb24a9cb2189db462a3fb57` (implementation commit `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`). Run `44871413803` completed but **failed at kernel link**; package job was skipped. Run #400 / artifact `14615937969` remains the last accepted image.
- Both QUP and RPM/GLINK host protocol tests passed in Run 448714. The linker reported undefined `_lavender_rpm_vote_vdd_dig_low_svs` from `LavenderQUPI2C::start`.
- Root cause shown in the log: `0130-lavender-rpm-glink-v0.patch` was skipped as malformed at hunk line 12 (`patch fragment without header`). As a result `LavenderRPMGLINK.cpp` was absent from the IOKit object list. Repair the patch/staging integration; do not add a weak stub or remove QUP's fail-closed voltage-vote gate.
- Add a hard build guard that checks the staged RPM source, `LavenderRPMGLINK.cpo` object-list entry, platform expert registration and rpm-glink nub publication before link.
- Workflow concurrency is one group per branch with `cancel-in-progress: true`. Reported cancelled runs: `44871035770`, `44871146744`, `44871215312`; preserved legacy run: `44868695080`; accepted Run #400: `44706182280`; RPM/GLINK run `44871413803`: link failure. Legacy runs from the old per-run group are not automatically cancelled by the new policy; verify individual statuses before claiming cancellation.
- Keep all related fixes in one coherent source update and avoid a build flood. Do not start another workflow until patch/staging and object-list guards are repaired and reviewed.
- The confirmed-booting Android 15/Lucid ROM remains read-only. No ROM modules, drivers, DTB, boot image or Android partitions may be modified. Physical tests remain RAM-only.
- See `docs/handoff/05-native-lavender-iokit-drivers.md` for the full run log findings, artifact IDs, RPM protocol notes and next acceptance gates.


## Run 44871413803 verified outcome and mandatory next step — 21 November 2026

- Run 44871413803 is **completed with failure**, not pending. QUP and RPM/GLINK host protocol tests passed. XNU link failed on undefined `_lavender_rpm_vote_vdd_dig_low_svs`; RAM-only image packaging was skipped.
- The build log identifies the cause: `0130-lavender-rpm-glink-v0.patch` was rejected as malformed at line 12 (`patch fragment without header`). The RPM source was not added to the IOKit object list, so the missing vote symbol is an integration/staging failure. Do not infer RPM implementation compile or runtime success.
- Before the next workflow, repair the patch or use an idempotent staging script, and add hard pre-link checks for the staged `LavenderRPMGLINK.cpp`, its `.cpo` object-list entry, platform-expert registration and `/soc/rpm-glink` nub publication. Verify the C-linkage declaration/definition and final link-list inclusion of `lavender_rpm_vote_vdd_dig_low_svs`.
- Do not use a weak stub, remove QUP's fail-closed voltage-vote gate, or modify the confirmed-booting Android/Lucid ROM. Batch the repair and guards into one coherent source update; avoid per-file commits/build floods.
- Run #400 / artifact `14615937969` remains the latest accepted bootable baseline. Run 448714's artifacts `14696531915` and `14696800920` are diagnostic logs/buildtree only, not bootable outputs.
- Full evidence and artifact digests are in `docs/handoff/05-native-lavender-iokit-drivers.md`.


## RPM/GLINK queue reconciliation and mandatory staging repair — 21 November 2026

- Latest RPM source: `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`; follow-up: `a74f05bfcfc82becccb24a9cb2189db462a3fb57`.
- The supplied branch message's “Run 44871413803 — latest RPM/GLINK build” / “Build pending” status is stale. The live GitHub run record is **completed / failure** for head `a74f05bfcfc82becccb24a9cb2189db462a3fb57`. Run #400 `44706182280` remains the latest accepted build and image.
- Verified queue records: `44871035770`, `44871146744`, and `44871215312` are cancelled; `44868695080` is completed/failure (legacy); `44871413803` is completed/failure; Run #400 is successful. The branch concurrency group is `xnu-${{ github.ref }}` with cancel-in-progress. Legacy runs from the former per-run groups are not automatically cancelled; do not claim otherwise.
- Run 448714's host QUP and RPM/GLINK tests passed, but final XNU link failed on undefined `_lavender_rpm_vote_vdd_dig_low_svs`. Root cause: patch `0130-lavender-rpm-glink-v0.patch` was malformed at line 12 and skipped, so the RPM implementation was not added to `files.arm64` or the staged IOKit object list. RAM-only packaging was skipped; diagnostic artifacts are not bootable.
- **Next task: repair the staging patch or replace it with a deterministic idempotent staging script.** Add hard pre-link guards for staged `LavenderRPMGLINK.cpp`, `LavenderRPMGLINK.cpo` in the IOKit object list, platform-expert service registration, `/soc/rpm-glink` nub publication, and matching C-linkage declaration/definition/final link inclusion for `lavender_rpm_vote_vdd_dig_low_svs`.
- Do not use a weak stub, remove QUP's fail-closed voltage-vote gate, bypass RPM ACK, or perform direct PMIC writes. Do not trigger another build until the patch and guards have been repaired and reviewed. Batch all source changes into one coherent update and then trigger one workflow only.
- Preserve Run #400 and its accepted artifact. The Android 15/Lucid ROM is read-only; never modify its drivers, modules, DTBs, boot images or partitions. Physical testing remains RAM-only.
- Full evidence and next-step procedure are in `docs/handoff/05-native-lavender-iokit-drivers.md`, section “Branch queue and RPM/GLINK status reconciliation”.


## RPM/GLINK current handoff — 21 November 2026

- Latest source: `a74f05bfcfc82becccb24a9cb2189db462a3fb57`; implementation commit: `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`.
- Run `44871413803` is completed with failure, not pending. QUP and RPM/GLINK host tests passed, but XNU final link failed on undefined `_lavender_rpm_vote_vdd_dig_low_svs`; packaging was skipped.
- Root cause in the build log: `0130-lavender-rpm-glink-v0.patch` was rejected as malformed at line 12, so `LavenderRPMGLINK.cpp` did not enter the IOKit object list and platform-expert registration/nub publication were not staged.
- Next: repair patch 0130 as a valid unified diff or use deterministic idempotent staging. Add pre-link guards for staged source, `LavenderRPMGLINK.cpo` object-list inclusion, platform-expert registration, `/soc/rpm-glink` publication, C-linkage vote declaration/definition and final link input. Never use a weak stub or remove QUP's fail-closed RPM ACK gate.
- Run #400 (`44706182280`, artifact `14615937969`) remains the latest accepted image. Run 448714 diagnostic artifacts are not bootable.
- Workflow concurrency is one group per branch with `cancel-in-progress: true`. The supplied queue report says runs `44871035770`, `44871146744`, and `44871215312` were cancelled; `44868695080` was preserved as a legacy run. The new group does not automatically cover old per-run groups. Do not claim manual cancellation of legacy runs; the available GitHub connection lacks that action.
- Avoid a build flood: make the repair, guards, tests and documentation one coherent source update, inspect the staged tree, then trigger one build.
- The confirmed-booting Android 15/Lucid ROM remains read-only. No ROM drivers, modules, DTB, boot image or partitions may be changed. Physical tests remain RAM-only.
- See `docs/handoff/05-native-lavender-iokit-drivers.md` for the full protocol design, failure evidence, artifact digests, queue state and acceptance sequence.


## RPM/GLINK patch-accounting repair — 21 November 2026

- Live branch HEAD at inspection: `6eebcfabdbe65e76c1c4863c6181e4cad36ab902`; this is a documentation-only advance beyond the RPM source commit `a74f05bfcfc82becccb24a9cb2189db462a3fb57`.
- Run `44871413803` failed because the first hunk of `0130-lavender-rpm-glink-v0.patch` declared 5 old / 6 new lines while its body contained 6 old / 7 new lines. The later `processTopLevel` hunk declared 4 old lines while its body contained 6. The first hunk has been narrowed to its three include-context lines (3/4), and the later hunk count is corrected to 6/19. Other hunk counts are checked automatically by the host-test script.
- The RPM/GLINK staged-source guard now checks that the implementation exports `lavender_rpm_vote_vdd_dig_low_svs` with C linkage and that QUP declares the same C-linkage signature. The workflow runs the staged-source guard immediately after patch/staging, before XNU compilation.
- A post-build audit requires `LavenderRPMGLINK.cpo` in the XNU build tree and the voltage-vote symbol in the captured build/link transcript. This supplements (but does not replace) the actual successful ARM64 link and binary verification.
- The QUP fail-closed voltage-vote gate remains unchanged. No weak stub, direct PMIC write, ACK bypass, Android/Linux driver, ROM, DTB, boot image or vendor partition change is permitted.
- This is source/build integration only. No physical GLINK OPEN, RPM ACK, voltage transition or QUP transaction is claimed.


## RPM/GLINK staging repair — current verified checkpoint, 21 November 2026

- Live branch HEAD at the start of this checkpoint was `6eebcfabdbe65e76c1c4863c6181e4cad36ab902`; repair commit `e280830dc978dc6b32c677bfcb88715a1b36670e` is now the branch HEAD (parent: `6eebcfabdbe65e76c1c4863c6181e4cad36ab902`). Use the live branch ref, not the older supplied SHA, as the next integration baseline.
- Commit `e280830dc978dc6b32c677bfcb88715a1b36670e` (`fix(lavender): repair RPM GLINK staging and link guards`) updates patch 0130, the RPM test/verification scripts, the XNU workflow, and these engineering records as one six-file repair.
- Patch hunk accounting was repaired: PlatformExpert include hunk narrowed to 3/4; `processTopLevel` corrected to 6/19; remaining hunk declarations retained after review. The host test runner now independently checks unified-diff hunk counts.
- Added C-linkage declaration/definition checks, staged-source guard invocation after staging and before XNU compilation, registration/nub/source-list checks, post-build `LavenderRPMGLINK.cpo` audit, and link-transcript vote-symbol audit.
- **Not yet accepted:** the newly extended host protocol tests have not been rerun; the new patch has not yet been validated by a fresh full staged-XNU build; no new Actions run was triggered or observed; no new linked kernel or `iosboot.img` exists. The object audit is post-build, not a true pre-link hook. Do not describe it as pre-link enforcement.
- Run #400 / artifact `14615937969` remains the last accepted kernel/image. Run 448714 remains failed; its prior QUP/RPM host tests passed but its link failed due the malformed patch. Preserve Run #400 unchanged.
- Next: add a genuine pre-link object-presence check if the build system permits; run both host regressions; validate patch application against the exact staged XNU source; inspect the IOKit object list; trigger exactly one workflow; require successful ARM64 link, RAM-only packaging, manifest and SHA-256 before any handset test.
- **Display safety:** RPM/GLINK and the initial `vdd_dig` vote do not program MDSS, DSI, panel timing, brightness or Tianma display commands. No direct display-damage mechanism is expected from this stage, but this is not a guarantee against unrelated electrical/firmware faults. Keep display bring-up separate; use conservative, source-backed rails/timings and bounded sequencing. A black screen can be a recoverable initialization failure; do not assume it means panel damage. No display driver or panel sequence is being enabled by this RPM repair.
- Android 15/Lucid OS remains strictly read-only. No Android drivers, modules, DTBs, boot images or partitions may be modified. No physical test until build/package acceptance; then RAM-only `fastboot boot iosboot.img`, never flash/write partitions.
- For the active engineering scope, read only this file and `docs/handoff/05-native-lavender-iokit-drivers.md`. Do not load other handoff chapters or `PROJECT_HANDOFF.md` unless the user explicitly changes scope.
