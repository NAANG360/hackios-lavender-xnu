# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Latest accepted diagnostic build:** Run #300 (`43891112657`, commit `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`) completed XNU compilation, linked kernel verification and RAM-only packaging successfully. Artifact `14236569374` (`lavender-fastboot-boot-test`) contains `iosboot.img` (16,601,088 bytes); SHA-256 `772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef`. The artifact manifest confirms the genuine 20H392 AppleLogo and fastboot-boot-only packaging. No physical retest of this exact checksum is recorded. (`43853989024`, commit `fd40a4953708f22e7df492bef699e657dade93ea`) completed XNU compilation, linked verification and RAM-only package job successfully. Artifact `14219764264` (`lavender-fastboot-boot-test`) contains `iosboot.img` (16,601,088 bytes); SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`. The package manifest identifies the genuine iPhone X iOS 16.7.16 (20H392) AppleLogo and stages 18/19 instrumentation. No physical retest of this exact checksum is recorded.
**Diagnostic purpose:** stage 18 follows `PE_consistent_debug_inherit()`; stage 19 follows `serial_init()`; stage 20 remains the kprintf startup completion marker.

## Latest operations — 16 November 2026

- **Run #281** ([43819847178](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43819847178), source `94c2366567aee5df21435a7c4bb91041f161305e`) compiled XNU but failed the required-patch guard. The job log identifies 0115 as corrupt at EOF line 161 and 0116 at EOF line 46; linked verification and packaging were skipped.
- **Root cause:** a trailing whitespace-only context line followed each patch's final hunk, and the final hunk counts were each one old/new line too large. Correct final counts: 0115 `6/12` (not `7/13`); 0116 `28/21` (not `29/22`).
- **Repairs:** commits `ab90b5840b891ee2894e2606f8b0e45b83ef8bd4` and `857e735a046c76c9889ddac429583b4a24e03706` remove the EOF context lines; commits `f84b0c43b94c322bb300bf57b2fde7329fce8ca5` and `596bf0c9690e468ab188d0008e7e9495108c6b11` fix the hunk metadata.
- **Static validation:** all 11 hunks in 0115/0116 have exact declared-to-observed counts; no malformed lines remain.
- **Run #285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), source `596bf0c9690e468ab188d0008e7e9495108c6b11`) is the first fully repaired run. It is in **Build XNU 8796**. Acceptance still requires no required-patch skips, all source guards, linked Mach-O verification and successful RAM-only package job.
- #282–#284 are intermediate commits/runs; use #285 for acceptance.
- **Visual status:** the original AppleLogo itself was physically observed earlier. The post-logo [DARWIN] overlay has not yet been accepted in an image or physically verified; console replay of pre-logo text remains outstanding.



## Physical baseline — Run #261

Run #261 (`43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed XNU build, source guards, linked Mach-O verification and RAM-only packaging. User video `1000317742.mp4` shows physical Lavender displaying the patch-0094 diagnostic Apple-shaped mark with the #261-attributed image. This is user-attributed, not checksum-bound. It confirms the framebuffer logo-rendering stage, not stage 20, root mount, launchd or SpringBoard. The user explicitly requires Apple's actual logo, not the custom approximation.

## Latest accepted package — Run #266

Run #266 (`43802934997`, source `aa07e58663cdbb7c4cc63084c55b6b7798b71cc5`) passed XNU build, kernel verification and RAM-only package:
- artifact ID `14197706518`, `lavender-fastboot-boot-test`;
- `iosboot.img` size 16,551,936 bytes;
- SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`.
This is an old diagnostic-logo baseline and has not been separately physically tested.

## Genuine AppleLogo integration

The exact iPhone X iOS 16.7.16 (20H392) firmware AppleLogo was extracted and hash-pinned:
- IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`;
- decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`.
Patch 0114 replaces the procedural logo pixels; it leaves framebuffer timing/call placement intact. Build-time helper generates the C header; the firmware image is not committed.

## Runs #267–#268 and exact correction

Run #267 (`43804842131`) failed in `prepare-lavender-applelogo.sh`: escaped shell variables caused `mktemp` to receive a literal `${TMPDIR:-/tmp}`. Commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` corrected shell expansion.

Run #268 (`43805363291`) compiled XNU and passed:
- generic ARMv8-A target check;
- both commpage LL/SC source guards;
- file-scoped SHA2 architecture check;
- exact 20H392 AppleLogo header hashes/dimensions and source integration check;
- pre-VBAR and deferred-logo source checks.

It then failed in the startup-trace verifier with `ValueError: substring not found`. The trace helper guard still sliced through `early.index("lavender_logo_ellipse", helper)`, but patch 0114 correctly removed that old custom-logo helper. This is a stale verifier boundary, not a compile or AppleLogo asset failure. Linked Mach-O verification and RAM-only packaging were skipped; raw candidate artifact `14198891426` is unverified and must not be booted.

**Fix committed:** `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` changes the slice end to the next function, `sweet_early_logo(void)`, keeping the trace-body check scoped to the trace helper. A local check against the #268 patched buildtree confirms the new boundary exists, the trace helper still contains events 5/7, and the helper slice does not include logo-renderer code or touch Guardian state. **Run #269** (`43809333900`) is executing the corrected workflow; at last inspection, `Build XNU 8796` was in progress. Its linked Mach-O verifier and RAM-only package must both pass before accepting the resulting image.

## Immediate next operations

1. The next physical test must use the exact Run #297 image (SHA above) to make marker observations attributable.
2. Record the last visible bottom-edge marker: 17 = logo completed; 18 = PE consistency returned; 19 = serial initialization returned; 20 = KPRINTF startup completed.
3. Do not infer BSD/root/userspace from marker 20. Continue only after the observed boundary is recorded.
4. Preserve RAM-only `fastboot boot iosboot.img`; no flashing. Do not request a handset test until the power-button-independent recovery/reset path is physically verified.

## Storage frontier — exact Build 4 embedded FDT

The Build 4 boot image's own appended FDT has been extracted and confirms:
- removable SDHC2 at `0x0c084000/0x1000`, compatible `qcom,sdhci-msm-v5`, enabled, 4-bit;
- card-detect GPIO54, host IRQ125, power IRQ221;
- `iface_clk`/`core_clk` use GCC SDCC2 AHB/APPS clock IDs 93/94;
- PM660L L5 and L2 are the VMMC/VQMMC supplies;
- SDHC1 at `0x0c0c4000` is internal 8-bit non-removable eMMC with CQE/ICE and must not be used for iOS root.

The matching Xiaomi SDM660 downstream driver defines V5 power-control offsets 0x240/0x244/0x248/0x24c and services BUS_ON/OFF and IO_LOW/HIGH with PMIC rails and active/sleep pinctrl transitions before acknowledging IRQ221. The removable host is not ready for command I/O until this state is handled.

## Next operations

1. Inspect Run #268's full XNU build, official-logo generated-header guard, linked verifier and RAM-only package result. Do not use #267 raw candidate.
2. If #268 passes, preserve the artifact and do not physically retest until the independent recovery/reset gate is satisfied.
3. Start with a no-write SDHC2 register/GPIO/power-state probe, then implement the V5 power IRQ/regulator/pinctrl handshake, then low-speed PIO SD-card reads. No DMA/CQE/ICE/write path.
4. Expose a bounded read-only BSD block device and `rd=sdroot0`; validate GPT and HFS+ before launching Darwin userspace.

## Device safety

The power button is broken. Volume-up + volume-down is a reset request only, not a verified recovery selector. No handset test until an independent recovery/reset route is physically verified. RAM-only `fastboot boot iosboot.img` only. Never flash, erase, repartition or write internal Android eMMC/userdata.


## Latest CI correction — Run #286 (16 November 2026)

- **#285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), commit `596bf0c9690e468ab188d0008e7e9495108c6b11`) compiled XNU but failed the required-patch guard because `0115-lavender-post-logo-darwin-trace.patch` was skipped. The build log gives the actual apply errors: `osfmk/kern/startup.c:744` and `bsd/kern/bsd_init.c:1044`. The earlier “only malformed hunk counts” diagnosis was incomplete; those hunks also used stale source context relative to the actual XNU tree after the preceding Lavender patches.
- **Corrective change:** regenerated patch 0115 against the exact post-0106…0114 source tree extracted from #285's `xnu-buildtree` artifact. The new unified diff includes the actual context around scheduler startup, IOKit, BSD bootstrap, VFS initialization, root mount, and `bsd_utaskbootstrap`. It was locally validated with `patch -p1 --dry-run` against that exact build tree: all three files checked successfully.
- **#286** ([43825403975](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43825403975)) is the fresh validation run for commit `b1b434dfc9d3668bbea9a4704a85a324ff0f9887`. It is in progress at the time of this update. Require 0115 and 0116 not to be skipped, then require kernel verification and RAM-only package completion. Do not distribute its candidate unless the full run is green.
- **Important correction to prior reports:** #281–#285 were not valid green builds for the new trace changes. The reliable previously packaged test image remains #274. The 0115 patch fix is now tested against the actual XNU source context rather than by hunk-count lint alone.


## Latest CI correction — Run #290 (16 November 2026)

- **Run #289** ([43826819680](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43826819680), source `ed8941c52741e510c2035033a6e109c1e155f332`) compiled XNU successfully, then failed the required-patch guard because patch 0115 could not apply its first `osfmk/kern/startup.c` hunk at line 658. Linked Mach-O verification and RAM-only packaging were skipped.
- **Root cause:** the startup.c hunk context in `0115-lavender-post-logo-darwin-trace.patch` had one extra tab in four comment-context lines (`\\t\\t *` instead of the source's `\\t *`). Hunk counts were not the issue. Reproduced locally against Run #289's `xnu-buildtree` artifact: the original hunk fails; correcting the four context lines passes `patch -p1 --dry-run`.
- **Fix committed:** `e2beb8e297c498def1c09fd2a32a5b0241c86102` changes only those four startup.c patch-context lines. No XNU source logic or diagnostic behavior changed.
- **Run #290** ([43829764065](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43829764065), source `e2beb8e297c498def1c09fd2a32a5b0241c86102`) is the validation run. Require 0115/0116/0117 to apply without skips, then all source guards, linked Mach-O verification and RAM-only packaging. Do not use Run #289's raw kernel candidate.


## Physical boot recordings and post-logo boundary — 16 November 2026

- The two user-supplied recordings show the shim handoff screen, the explicit `A73 supported; launching XNU` message, then the genuine AppleLogo. This rules out a simple failure before the XNU launch handoff.
- The recordings show no readable Darwin console transcript after the AppleLogo. The small bottom-edge green marks are consistent with the existing stage-16/17 markers, but the video is not checksum-bound to Run #290 image; treat the stage attribution as provisional.
- Run #290 source places stage 17 immediately after `sweet_early_logo()`. Stage 20 is not written until after `serial_init()` and `kernel_startup_initialize_upto(STARTUP_SUB_KPRINTF)`. The next useful split is the post-logo interval, not another AppleLogo change.
- **Patch 0118** adds stage 18 immediately after `PE_consistent_debug_inherit()` and stage 19 immediately after `serial_init()`. The markers are raw framebuffer bars and Guardian words, so they do not depend on the unverified text-overlay path. Stage 20 semantics are unchanged.
- **Run #291** (`43843250915`, commit `cb5c444f4071bae4dc84de8d21670d9cab3bfbf3`) is the validation build. If stage 18 is absent, investigate the interval from stage 17 through PE consistency setup; if 18 appears but 19 does not, isolate `serial_init()`; if 19 appears but 20 does not, isolate KPRINTF startup.


## Run #291 failure and patch 0118 correction — 16 November 2026

- **Run #291** (`43843250915`, commit `cb5c444f4071bae4dc84de8d21670d9cab3bfbf3`) successfully compiled XNU 8796, then failed the required-patch guard. The exact log says `0118-lavender-post-logo-boundary-trace.patch` was skipped as `corrupt patch at line 20`; linked verification and RAM-only packaging were skipped. Do not boot its raw candidate.
- **Root cause:** the first 0118 diff had incorrect unified-diff hunk counts and incomplete source context. The actual XNU 8796 `arm_init.c` context was recovered from Run #290 buildtree: `WITH_CLASSIC_S2R` block, `PE_consistent_debug_inherit()`, `serial_init()`, and the existing stage-20 marker.
- **Correction:** commit `4b91491e3ced22cda444c46161f72a05738a58fd` replaces 0118 with the exact context and corrected hunk metadata. `patch -p1 --dry-run` against the Run #290 XNU buildtree succeeds (one hunk, fuzz 2).
- **Next:** obtain a fresh CI run for commit `4b91491e3ced22cda444c46161f72a05738a58fd`; confirm 0118 is applied, its stage-order source guard passes, then require linked ARM64 Mach-O verification and RAM-only packaging. No new image is accepted before all gates pass.


## Run #293–#296 post-logo trace patch repair — 16 November 2026

- **Run #293** (commit `b3e852cc15cba5e2b527a7e13de2333703166e60`) compiled XNU, then failed the required-patch guard: patch 0118 did not apply at `osfmk/arm/arm_init.c:549`. Linked verification and packaging were skipped.
- **Run #294** (commit `71ff3e91eff4877d8e57d7adf6627fc2c826566e`) reproduced the same failure. The actual patch log confirms `PE_consistent_debug_inherit()` context did not match after patch 0108 had modified earlier code in the same file. XNU compilation itself succeeded; this is a patch-context failure, not a compiler failure.
- **Correction:** patch 0118 was reduced to single-line anchors on the exact `PE_consistent_debug_inherit();` and `serial_init();` statements, with only the stage 18/19 additions. Hunk offsets were then aligned for the +8-line insertion made by patch 0108: commit `52d0ac77316b3ba880b543985d215104f4a9c527`.
- **Run #295** (commit `c1cddaa5ee4bcb536073f2cb915c9620c1ab11ad`) validates the minimal-anchor patch; it may complete independently.
- **Run #296** (commit `52d0ac77316b3ba880b543985d215104f4a9c527`) is the preferred validation run for the corrected hunk offsets. Require patch 0118 to apply, the source-order guard to pass, linked ARM64 Mach-O verification to pass, and the RAM-only package job to finish before accepting any image.
- Raw kernel candidates from #293/#294 are not bootable test artifacts. Preserve #290 as the last accepted package until #295 or #296 passes every gate.


## Run 296 failure and 0118 correction — 16 November 2026

- Run #296 (`43850609298`, commit `52d0ac77316b3ba880b543985d215104f4a9c527`) compiled XNU successfully, then failed the required-patch guard because patch 0118 was skipped: its first hunk targeted line 557, while the source after patches 0115–0117 had `PE_consistent_debug_inherit()` at line 553 and `serial_init()` at line 556. The failure was patch context mismatch, not a compiler error.
- Corrected 0118 to use the final post-0117 source context around lines 553–563. Verified the exact patch with `git apply --check` and applied it cleanly against the Run 296 buildtree's `osfmk/arm/arm_init.c`.
- Run #297 (`43853989024`, commit `fd40a4953708f22e7df492bef699e657dade93ea`) is queued to validate the corrected patch. Do not use the Run 296 raw kernel candidate; its packaging job was skipped after the required-patch guard failed.


## Latest accepted diagnostic package — Run #297 (16 November 2026)

- Run #297: [43853989024](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43853989024), commit `fd40a4953708f22e7df492bef699e657dade93ea`.
- Both jobs completed successfully: `build` and `Package RAM-only Lavender fastboot image`. This run is green through XNU build, linked verification and RAM-only package.
- Artifact `14219764264`, name `lavender-fastboot-boot-test`, contains `iosboot.img` (16,601,088 bytes). Verified SHA-256: `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`.
- The artifact's `BUILD-MANIFEST.txt` identifies XNU workflow run 43853989024, source commit fd40a495…, genuine 20H392 AppleLogo raster (194×238), and the early stage diagnostics. The artifact's own `SHA256SUMS` agrees with the extracted image hash.
- Patch 0118 provides stage 18 after `PE_consistent_debug_inherit()` and stage 19 after `serial_init()`. Stage 20 remains after `kernel_startup_initialize_upto(STARTUP_SUB_KPRINTF)`.
## Physical recording update — 16 November 2026

- User clip `1000317779.mp4` shows the red shim handoff screen reporting the initial CPU16K rejection, successful A73 fallback (`TGRAN=1`), `BOOT CPU guardian active`, `A73 16K supported: launching XNU`, `BOOT MPI=0x100`, and the XNU entry address.
- Later frames show the centered AppleLogo on black and four discrete green bars accumulated along the lower edge. The actual firmware AppleLogo asset is integrated by patch 0114 in Run #297's accepted patch chain.
- The visible bars strongly match stages 17–20: stage 17 follows `sweet_early_logo()`; stage 18 follows `PE_consistent_debug_inherit()`; stage 19 follows `serial_init()`; stage 20 follows `kernel_startup_initialize_upto(STARTUP_SUB_KPRINTF)` and the `kprintf initialized` call. Thus the clip strongly suggests that XNU reached KPRINTF startup completion, beyond the earlier shim-entry-only evidence.
- **Attribution limit:** the phone recording contains no device-side image digest. It is not checksum-bound to Run #297 artifact `14219764264` / `iosboot.img` SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`; treat stage 20 as a strong source-correlated inference, not exact-image proof.
- No readable `[DARWIN]` event trace is visible after the four bars. Do not infer scheduler completion, IOKit, BSD init, root mount, launchd, SpringBoard or Darwin userspace from stage 20. The separate `sweet_early_trace_startup` event IDs 17–29 in patch 0115 are a different trace namespace from the simple stage markers 17–20.
- **Next:** preserve Run #297 as the accepted diagnostic image. Focus on post-KPRINTF startup and make the patch-0115 events 17–29 observable; do not change the AppleLogo or early handoff without new evidence. Any further physical boot remains RAM-only and gated on the power-button-independent reset/recovery procedure being verified. Never flash.




## Post-KPRINTF trace CI hardening — 16 November 2026

- Run #297 remains the latest accepted image: artifact `14219764264`, `iosboot.img` SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`. The physical clip `1000317779.mp4` strongly correlates with simple stages 17–20, but is not checksum-bound to this image; do not claim BSD/root/userspace.
- Added workflow source guards for the distinct `sweet_early_trace_startup` event namespace 17–29: ordered scheduler, IOKit, early-boot, interrupt, BSD-init, VFS, root-mount and utask-bootstrap boundaries; plus the transparent `[DARWIN]` console renderer and post-logo header ordering.
- Change commit: `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`. The guard was executed locally against the exact patched XNU source extracted from Run #297's `xnu-buildtree` artifact and passed. A fresh Actions run is the next acceptance gate; no new image is accepted until XNU build, linked Mach-O verification and RAM-only package all pass.
- Physical retest remains blocked until the independent power-button-free recovery/reset route is physically verified. Never flash; RAM-only fastboot boot only.
