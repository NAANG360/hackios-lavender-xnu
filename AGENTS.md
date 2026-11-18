## Latest accepted diagnostic image — Run #357 (18 November 2026)

- **Latest accepted image:** [Run #357 / 44271639569](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44271639569), source commit `fc9da9a76444fafc2a2a2d40bd1fd71c2104a1ad`. Both jobs passed: XNU build/source verification and separate RAM-only fastboot packaging.
- The build job passed the Lavender machine profile, ADT memory/CPU topology, entry-guard, non-secure Group-1 virtual timer, IOKit platform-match/early-init trace, linked kernel artifact verification and upload gates. The package job passed kernel retrieval, Mach-O presence, OTA/DTB verification, Lavender ADT generation, payload construction, boot-image construction, structural/payload identity checks and image upload.
- **Artifact:** `lavender-fastboot-boot-test`, ID `14414465679`, uploaded 18 November 2026. Extracted `iosboot.img`: **16,617,472 bytes**; independently computed SHA-256: `97d2fb54ea8367b6236f3e41d13a43ac1cdcce4c28bb26632e020cb89dc5ad7f`. The artifact `BUILD-MANIFEST.txt` and `SHA256SUMS` agree. Android boot-image structure was independently identified (`file`); manifest records kernel, payload and DTB hashes.
- Bootargs: `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. **Framebuffer console is explicitly disabled**; do not expect the earlier on-screen KBOOT console trace from this image. Preserve `cpus=1`.
- This is a verified **RAM-only diagnostic boot package**, not a complete Darwin/userspace image and not evidence of a physical boot. CI success is not handset success.
- User clarified that the newly supplied ~four-minute recording was filmed using an older image. It is therefore not evidence for Run #357 and must not be checksum-attributed to it. Keep footage/image attribution explicit; request a fresh recording after testing this exact image if physical progress is to be assessed.
- **Previous run #355** ([44264884787](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44264884787)) failed and produced no accepted image. Run #357 is the succeeding green build after the subsequent verifier identity-check correction.
- Physical test only with `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard or complete iOS boot without direct evidence.

# Lavender XNU — agent entrypoint

**Start here.** Inspect the live branch HEAD and newest Actions runs, then open only the routed chapter. Use the handoff chapters as the active engineering record; do not consult `PROJECT_HANDOFF.md` unless the user explicitly asks.

## Mission
Bring XNU 8796 and eventually iPhone X iOS 16.7.16 (20H392) Darwin userspace to Xiaomi Redmi Note 7/7S (`lavender`, Qualcomm SDM660 / Kryo 260, ARM64).

## Current milestone — 18 November 2026

- **Latest accepted diagnostic image: Run #357** ([44271639569](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44271639569)), source commit `fc9da9a76444fafc2a2a2d40bd1fd71c2104a1ad`. Artifact `14414465679` (`lavender-fastboot-boot-test`) contains `iosboot.img`, 16,617,472 bytes, SHA-256 `97d2fb54ea8367b6236f3e41d13a43ac1cdcce4c28bb26632e020cb89dc5ad7f`. The extracted image was independently hashed; its hash matches both `BUILD-MANIFEST.txt` and `SHA256SUMS`.
- Run #357 passed the XNU build/source-verification job and the separate RAM-only packaging/upload job. The IOKit platform-match/early-init trace verifier, linked kernel verification, boot-image structure and kernel/payload/DTB identity checks all passed.
- Exact bootargs: `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. Framebuffer console is disabled in this image; do not expect KBOOT text on screen. Preserve `cpus=1`.
- This is a verified RAM-only diagnostic package, not a complete Darwin/userspace image and not evidence of a physical boot.
- User clarified the approximately four-minute recording was filmed with an older image. It is not evidence for Run #357 and is not checksum-bound to it. Require a fresh test recording of this exact image before making physical-progress claims.
- Run #355 ([44264884787](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44264884787)) failed and produced no accepted image; Run #357 is the succeeding accepted build.
- Physical test only via `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard or complete iOS userspace without direct evidence.

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


## New physical footage review — IOKit progress, image attribution unresolved (18 November 2026)

- Reviewed user uploads `VID-20261118-WA0004.mp4` (98.38 s) and `VID-20261118-WA0002.mp4` (5.78 s). The longer clip visibly shows the framebuffer diagnostic console and an Apple logo overlay. Readable lower-console milestones include `PE_INIT_IOKIT DONE`, `PE PREPARE IMAGES DONE`, `PE PROGRESS SETUP DONE`, `PE ROOT TIMER DONE`, `IOKIT INIT CALL BEGIN`, `IOKIT SYMBOLS DONE`, `IOKIT GLOBALS DONE`, `IOKIT MACH PORTS DONE`, `IOKIT GROUPS DONE`, and finally `IOKIT ROOT MAP ALLOCATED`. The exact final visible IOKit marker in the reviewed frame is `IOKIT ROOT MAP ALLOCATED`; no later completion marker is established by this review.
- This is evidence of progress substantially beyond the earlier `KBOOT IOKIT BEGIN` / missing event-20 boundary in the Run #349-era recording. It does not establish that `InitIOKit()` or `PE_init_iokit()` returned, that BSD init/root mount/launchd/SpringBoard ran, or that userspace booted.
- **Do not attribute these recordings to Run #357.** Run #357's accepted bootargs include `lavender_no_fbcon=1`, while this footage visibly contains the framebuffer console trace. No checksum, bootargs capture, or other provenance in the submitted clips binds them to Run #357's exact `iosboot.img`. The Run #357 artifact remains the latest accepted CI image; the footage is an unbound physical observation until the tested image is identified.
- The footage alone does not expose a readable panic/reset reason. Do not infer a specific fault from the static final marker. Next evidence should identify the exact image used (preferably the extracted image SHA-256 and the recorded fastboot command) and capture the next diagnostic boundary after `IOKIT ROOT MAP ALLOCATED`.
- Preserve all safety gates: `cpus=1`; RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition, or write Android eMMC/userdata; preserve volume-key recovery; no speculative GIC changes. Keep CI image acceptance separate from physical boot observations.
