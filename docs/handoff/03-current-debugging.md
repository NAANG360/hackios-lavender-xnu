# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Current accepted build:** Run #290 (`43829764065`, commit `e2beb8e297c498def1c09fd2a32a5b0241c86102`) completed XNU compilation, linked verification and RAM-only fastboot packaging. Its `iosboot.img` is 16,601,088 bytes; SHA-256 `c3396b3a4a41a5dd952074b51183d254e20daa4561f2099663c72efae8b87656`. Physical testing is not checksum-bound to this artifact.
**Current diagnostic validation:** Run #291 (`43843250915`, commit `cb5c444f4071bae4dc84de8d21670d9cab3bfbf3`) adds post-logo checkpoints 18/19 around `PE_consistent_debug_inherit()` and `serial_init()`; build was in progress at handoff update.

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

1. Follow Run #291 and confirm patch 0118 applies and its source-order guard passes.
2. Use the physical bottom-edge marker result: stage 18 means `PE_consistent_debug_inherit()` returned; stage 19 means `serial_init()` returned; stage 20 remains the existing kprintf-startup/Guardian-completion marker.
3. Require linked ARM64 Mach-O verification and RAM-only packaging before producing another test image.
4. Preserve the RAM-only `fastboot boot iosboot.img` path and existing recovery safeguards.

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
