# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Latest accepted diagnostic build:** Run #331 ([44075155032](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44075155032), source commit `748d7d1c75fa0af5e3c178d510167b3314c21c12`). Artifact `14325018991` (`lavender-fastboot-boot-test`) contains `iosboot.img`, 16,617,472 bytes; SHA-256 `f2b2bb70d236213f02fb8b3e25780a6b337ea8de26890c24ae11bb85a1fd806b`. This is a structurally accepted RAM-only diagnostic image, not proof of complete Darwin userspace.
**Current source candidate:** commit `61410e063a3dcfb3eac54c48832c677932cdb178`, Run #333 queued. Run #332 was cancelled; no accepted image came from it.
**Diagnostic purpose:** event 87 is the last visible physical checkpoint after GICR mapping. Patches 0123/0124 split post-map redistributor/IRQ setup through event 105. Stage 18/19/20 meanings remain separate from this event namespace.
## Latest operations — Run #318 diagnosis and Run #320 validation (17 November 2026)

- Run #318 (44016311664, commit 9041148e40462d7eea7a32b9fd176f7ec29d0f28) compiled translation units, then failed at the final Mach-O link of kernel.development.lavender.unstripped with unresolved _sweet_early_trace_startup references. This is a link-time ABI issue, not a C compilation failure.
- The splitter renames the implementation to lavender_early_trace_startup and the sweet_early.h macro redirect only affects translation units that include that header. startup.c, lock_mtx.c, lock_group.c and related patched callers have raw extern declarations without the header, so they still require a real global sweet_early_trace_startup symbol.
- Run #318 also exposed stale Step 13 guard anchors: the verifier searched the generated Lavender source for sweet_early_guard_init, sweet_early_mark, sweet_early_putc, sweet_early_logo and sweet_early_trace_startup. The split output uses lavender_early_* names; the putc definition is formatted as a multi-line function signature.
- Fix commit: 0abe1270618c7a5d3a303dd1c2603e107c94bf26. The splitter now appends a forwarding sweet_early_trace_startup ABI wrapper calling lavender_early_trace_startup, and ensures it on both fresh-split and already-split paths. Step 13 now uses the post-split Lavender names and searches for lavender_early_putc(char c). The duplicated stage marker assertion was removed.
- Pre-commit validation: Python syntax and YAML parsing both passed. The four raw-extern caller files were not modified; the wrapper preserves their existing ABI.
- Run #320 (44020121980) is the validation run for commit 0abe1270618c7a5d3a303dd1c2603e107c94bf26. At the latest poll, all setup/toolchain/profile/cache/capacity/dependency steps passed and Build XNU 8796 was in progress. Steps 12 and 13, linked Mach-O verification and package/artifact gates remain pending.
- No new image is accepted. Run #302 artifact 14245608682 remains the latest accepted diagnostic image, 16,601,088 bytes, SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98.
- Next: inspect Run #320 through Step 13, linked verification and RAM-only package. If the link still fails, inspect generated lavender_early.c in the buildtree and verify the wrapper is present exactly once and emitted as a global symbol. Never flash; physical tests remain RAM-only.

## Latest operations — 17 November 2026

- Run #314 (`44013809798`, source `e49691eaa88dca38e5b344b8981b12e10665a874`) passed setup and entered the BSD compile stage, but failed because exported `osfmk/arm64/sweet_early.h` included `<arm64/lavender_early.h>`, which the XNU exported-header set did not contain. Exact compiler error: `sweet_early.h:10:10: fatal error: 'arm64/lavender_early.h' file not found`.
- Root cause: the splitter generated a new board header after the build's exported-header phase; that generated header was not available under `EXPORT_HDRS/osfmk/arm64`.
- Fix committed as `d36eabe3fb4a1d9d001de0c1cfbc3a1ffc9b2183`: keep Lavender function prototypes directly in the already-exported `sweet_early.h` compatibility facade and include that existing facade from generated `lavender_early.c`. The split helper no longer requires the generated header to be exported for compilation.
- Run #315 (`44016227221`) is testing this exact fix. It has completed checkout and is selecting Xcode; build, linked verification and RAM-only package remain pending. Do not accept or boot any raw candidate until those gates pass.
- The previous #314 raw candidate artifact is unverified and is not an `iosboot.img` acceptance artifact.

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
4. Preserve RAM-only `fastboot boot iosboot.img`; no flashing. The user confirms the power-button-independent volume-key recovery/reset route works; the recovery gate is cleared. Use RAM-only `fastboot boot iosboot.img` only.

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
2. If #268 passes, preserve the artifact and the independent volume-key recovery/reset route is now confirmed working, so that gate is cleared.
3. Start with a no-write SDHC2 register/GPIO/power-state probe, then implement the V5 power IRQ/regulator/pinctrl handshake, then low-speed PIO SD-card reads. No DMA/CQE/ICE/write path.
4. Expose a bounded read-only BSD block device and `rd=sdroot0`; validate GPT and HFS+ before launching Darwin userspace.

## Device safety

The power button is broken. Volume-up + volume-down is a reset request only, not a verified recovery selector. The user confirms the volume-key recovery/reset route works; the recovery gate is cleared. RAM-only `fastboot boot iosboot.img` only. Never flash, erase, repartition or write internal Android eMMC/userdata.


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
- **Next:** preserve Run #297 as the accepted diagnostic image. Focus on post-KPRINTF startup and make the patch-0115 events 17–29 observable; do not change the AppleLogo or early handoff without new evidence. The recovery gate is cleared by the user's confirmation; physical testing remains RAM-only and must use an accepted `iosboot.img`. Never flash.




## Post-KPRINTF trace CI hardening — 16 November 2026

- Run #297 remains the latest accepted image: artifact `14219764264`, `iosboot.img` SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`. The physical clip `1000317779.mp4` strongly correlates with simple stages 17–20, but is not checksum-bound to this image; do not claim BSD/root/userspace.
- Added workflow source guards for the distinct `sweet_early_trace_startup` event namespace 17–29: ordered scheduler, IOKit, early-boot, interrupt, BSD-init, VFS, root-mount and utask-bootstrap boundaries; plus the transparent `[DARWIN]` console renderer and post-logo header ordering.
- Change commit: `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`. The guard was executed locally against the exact patched XNU source extracted from Run #297's `xnu-buildtree` artifact and passed. A fresh Actions run is the next acceptance gate; no new image is accepted until XNU build, linked Mach-O verification and RAM-only package all pass.
- The independent volume-key recovery/reset route is confirmed working; physical testing is no longer blocked by that gate. Never flash; RAM-only fastboot boot only.


## Run #300 source/artifact forensic check — 16 November 2026

- Re-downloaded Run #300 artifact `14236569374` (`lavender-fastboot-boot-test`) from workflow run `43891112657`. Extracted `iosboot.img` is exactly **16,601,088 bytes** and independently hashes to **`772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef`**, matching this handoff's recorded SHA-256. This is the accepted diagnostic image; RAM-only fastboot boot only, never flash.
- Downloaded the matching Run #300 `xnu-buildtree` artifact `14238555047` and inspected its patched XNU sources. The compiled tree contains the Lavender `sweet_early_trace_startup()` renderer and event calls 17–29: scheduler begin/done (17–18), IOKit begin/done (19–20), early-boot/interrupts (21–22), BSD init (23–24), VFS (25), root mount (26–27), and BSD utask bootstrap (28–29). The [DARWIN] renderer appends to the existing lower-panel console and does not intentionally clear the logo after it has been drawn.
- This confirms that the trace instrumentation is present in the Run #300 buildtree; it does **not** establish that the handset executed any of these events. Stage 20 is still only the earlier kprintf-startup boundary. The first post-KPRINTF observable event is 17, emitted when `kernel_bootstrap_thread()` begins scheduler startup.
- **Next diagnostic split:** look for `[DARWIN] KBOOT SCHED BEGIN` (17), then `KBOOT SCHED DONE` (18). If 17 is absent, investigate entry into `kernel_bootstrap_thread()` after KPRINTF; if 17 appears without 18, isolate `sched_startup()`. Only then proceed to IOKit (19–20), early boot/interrupt enable (21–22), BSD init (23–24), VFS/root (25–27), and utask bootstrap (28–29).
- The user confirms the independent volume-key recovery/reset procedure works; that gate is cleared. Test only accepted images with RAM-only fastboot boot. The user's clip is not checksum-bound to Run #300. Do not claim BSD/root/launchd/SpringBoard from the current video.

## Run #300 physical result and bootstrap-entry trace correction — 16 November 2026

- User recording 1000317806.mp4 shows the red shim handoff diagnostics before the genuine AppleLogo. Those red-screen lines are shim/loader handoff output, not the post-logo XNU/Darwin verbose transcript.
- After the logo appears, four green lower-edge markers remain visible and the small LAVENDER XNU VERBOSE BOOT header is visible. No readable [DARWIN] event lines or scrolling XNU log are visible after the logo. The apparent FASTBOOT screen at the end is a recording glitch; the phone actually restarts normally. The video does not identify the reset cause.
- The four green markers are consistent with the existing stage 17–20 sequence and strongly suggest the simple early marker path reached KPRINTF startup completion. This remains provisional because the video has no device-side checksum and is not bound to Run #300's image. It does not prove BSD, root mount, launchd or SpringBoard.
- Correction to the Run #300 source interpretation: in the exact compiled startup.c, event 17 (KBOOT SCHED BEGIN) is emitted only after idle_thread_create(processor) returns. It is not at the beginning of kernel_bootstrap_thread(). Therefore absence of visible [DARWIN] lines cannot yet isolate sched_startup(); the bootstrap thread may not have been entered, or may stall in current_processor() / idle_thread_create(), or the trace output may not be visible.
- Added patch 0119-lavender-bootstrap-thread-entry-trace.patch (commit 56c427f8a506cdc6ba250c536810df95dfb01ae4) with four non-Guardian trace events:
  - 33: immediately before load_context(thread);
  - 30: first statement of kernel_bootstrap_thread();
  - 31/32: immediately before/after idle_thread_create(processor);
  - existing 17/18 remain the before/after sched_startup() boundaries.
- Patch 0119 was dry-run validated against the exact Run #300 compiled buildtree, and the resulting source-order check passed. Workflow guard update commit 34a85dd88d13cc4b30e6ccf45a92aae9f685cce8 requires patch 0119 and validates the new ordering.
- Runs #301 and #302 were created by the patch and guard commits; #302 includes both changes and is the preferred acceptance run. They were queued at the time of this update. Require the complete XNU build, required-patch/source guards, linked Mach-O verification and RAM-only package job to pass before accepting any new image.
- The user confirms Chapter 01's power-button-independent volume-key recovery/reset route works; the recovery gate is cleared. No flashing. The next accepted test must remain RAM-only fastboot boot iosboot.img.


## Gemini audit cross-check, Run #302 acceptance and next diagnostics — 16 November 2026

- **Run #302 is green end-to-end:** build and separate RAM-only package jobs both succeeded. Artifact `14245608682` contains `iosboot.img` (16,601,088 bytes), SHA-256 `2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98`. This accepts patch 0119 and its source-order guard.
- Run #301 was cancelled after being superseded. Do not treat it as a source/build failure.
- Exact Run #300 buildtree cross-check corrects Gemini's call-graph description: after stage 20, `arm_init()` calls `PE_create_console()`, `PE_init_printf(FALSE)`, `cpu_machine_idle_init(TRUE)`, and then `PE_init_platform(TRUE, &BootCpuData)`, before `machine_startup(args)`. The event-34–39 diagnostics bracket these actual boundaries.
- `kernel_bootstrap()` does **not** contain `STARTUP_SUB_VM_KERNEL` or the second `PE_init_platform()` call. Its real instrumented sequence uses `vm_mem_bootstrap()`, `STARTUP_SUB_OSLOG`, `sched_init()`, `STARTUP_SUB_MACH_IPC`, and kernel-thread creation (events 41–46). Do not copy Gemini's incorrect subsystem ordering into future analyses.
- Patch `0120-lavender-panic-and-bootstrap-trace.patch` (commit `c6e812c7cdc712f8d88384a72e32e11bf7871a6e`) adds: early panic text on the post-logo console; first kernel synchronous-exception ESR/FAR/ELR/SPSR reporting; event 34–39 checkpoints across the post-stage-20 `arm_init()` path; and event 41–46 checkpoints through the real `kernel_bootstrap()` path.
- The patch applies cleanly after 0119 against the exact Run #300 buildtree; all 16 unified-diff hunk counts and the event-order checks passed locally. This is source validation only; compilation and package acceptance remain pending.
- Workflow guard commit `dc0603b6e6a64e117b04b447f82de37ecc36ed3f` requires 0120 and checks its event ordering, panic wiring, and exception reporter. Run #303 (`43909137046`) and preferred guarded Run #305 (`43909349821`) were queued at last check; #304 is the acceptance run.
- The SWEET GIC Group-1/virtual-timer configuration is **not** presumed correct for Lavender. The Lavender platform profile explicitly rejects inheriting SWEET's GIC security-routing and timer assumptions. The `#if defined(SWEET)` gaps in patches 0017/0004/0065/0078 require a separate hardware-evidence audit, not a blind conditional broadening.
- **Physical-video correction:** the apparent final FASTBOOT frame is a recording glitch. The handset restarts normally; the cause remains unknown. The four green bottom bars correspond to simple stages 17–20, not Darwin trace events 17–20.
- **Recovery gate:** the user confirms the power-button-independent volume-key recovery/reset route works. This clears the handset-test recovery gate. Continue to use RAM-only `fastboot boot iosboot.img`; never flash or write non-`userdata` partitions.
- Run #302's manifest predates the recovery confirmation and contains obsolete recovery-pending wording. The workflow manifest text has since been corrected; do not use that older manifest line as current device status.


## Patch 0120 hunk-count repair and Run #305 — 16 November 2026

- Independent validation of the committed patch found one under-counted context line in the first `kernel_bootstrap()` hunk. The hunk header was corrected from `12/18` to `13/19` in commit `b457583a2e8b6d42406bf44d692c612225d0c88d`.
- The corrected committed patch has 16 hunks with all old/new counts matching. It was applied after patch 0119 to a fresh copy of the exact Run #300 buildtree; both `git apply --check` and application succeeded.
- Run #303 uses the pre-correction patch and is not an acceptance candidate. Run #304 also references the pre-correction patch. **Run #305** (`43910208977`) is the preferred guarded acceptance run containing both the corrected 0120 patch and the workflow source guard.
- Recovery route remains confirmed; the previous physical-test gate is cleared. Only use a CI-accepted image and RAM-only `fastboot boot iosboot.img`.

## Lavender early-console split — 17 November 2026

- User requested that the inherited sweet_early.c be ported into a distinct lavender_early.c.
- Live branch at start of work: ci/lavender-machine-profile, HEAD 9295a8324577bc1c26f4a03e1f4598ca1107236d. Run #306 (43910553183) failed during XNU compilation; it did not pass the linked verifier or package job. Do not use its raw kernel candidate.
- Inspected the exact Run #306 xnu-buildtree artifact 14247844964; its patched source contains the current Lavender implementation in the ARM64_BOARD_CONFIG_LAVENDER branch of osfmk/arm64/sweet_early.c.
- Added tools/split-lavender-early.py. After the full pinned patch stack, the workflow now extracts that Lavender implementation to osfmk/arm64/lavender_early.c, renames public functions to lavender_early_*, creates a dedicated header, removes the Lavender branch from sweet_early.c, adds the new source to files.arm64, and retains sweet_early.h as a macro compatibility facade for existing shared call sites.
- Updated .github/workflows/xnu.yml: invoke the splitter after patches, touch the generated source, include the tool in cache hashing and push filters, and assert the source split and target routing before existing startup/panic trace guards.
- Workflow runs #307–#310 were triggered by incremental commits while the change was being wired in. #310 (44008439363, source 22590d814dc0770b2740d413b52600e0c32d622e) is the latest intended guarded validation run; inspect its actual build, source guard, linked verifier and package results before accepting it. Runs #307–#309 are intermediate snapshots, not acceptance candidates.
- The generated source is based on the exact #306 post-patch buildtree, preserving the existing Lavender framebuffer (0x9d400000, 1080×2340, stride 1080 pixels), bootloader AppleLogo asset integration, stage markers, Guardian cell, panic output and trace events 1–46. The SWEET source remains for SWEET only.
- No new iosboot.img is accepted yet. Run #302 artifact 14245608682 remains the latest accepted diagnostic image (SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98). Continue RAM-only fastboot boot iosboot.img; never flash.


### Run #322 trace-event decoding

The compact renderer prints the numeric event ID as `[DARWIN] POST-PLATFORM EVENT N`. Decode it as follows:

| Event | Boundary |
|---:|---|
| 47 | Remaining `arm_init()` tail entered after `PE_init_platform()` |
| 48 | CPU configuration invalid branch, immediately before its panic |
| 49 | CPU configuration passed; `mem_size` is passed as an argument but the compact renderer currently prints only the event ID |
| 50 | `ml_map_cpu_pio()` returned |
| 51–52 | Before / after `cpu_timebase_init(TRUE)` |
| 53 | CPU timebase and optional KPERF timer block completed |
| 54–55 | `PE_init_cpu()` / `fiq_context_init(TRUE)` returned |
| 56 | Optional hibernation initialization block completed |
| 57 | `/chosen` device-tree node lookup completed |
| 58–59 | `dram-base` / `dram-size` property reads completed; the values and byte lengths are passed as arguments but are not displayed by the compact renderer |
| 60 | `gDramBase` and `gDramSize` assigned |
| 61 | Stack-protector guard initialization completed |
| 62–63 | `machine_startup()` entered / pre-fastipi checkpoint |
| 67 | Optional fastipi configuration block completed |
| 66 | `machine_conf()` entered |
| 68–69 | `machine_info.memory_size` assignment before / after |
| 70–71 | CPU-topology lookup before / after; `num_cpus` is passed as an argument but is not displayed by the compact renderer |
| 64 | `machine_conf()` returned |
| 65 | Immediately before calling `kernel_bootstrap()` |
| 41 | First trace in `kernel_bootstrap()`; definitive bootstrap entry |

These event numbers are a separate namespace from early Guardian stage markers 17–20 and post-KPRINTF scheduler/BSD events. The startup trace writes only to the framebuffer console; it must not update the Guardian progress cell.


### Latest footage clarification — 17 November 2026

- The latest clip `1000317833.mp4` has a duration of approximately 55.7 seconds. Its last clearly readable XNU console checkpoint is event 38, `ARM_INIT PE_PLATFORM DONE`. Event 39 is not legible, so `machine_startup()` entry remains unproven.
- The clip's final frame shows a low-battery/charging glyph, not a readable Android restart, FASTBOOT screen, or XNU panic. Do not infer a reset cause or a Guardian timeout from this recording.
- Patch 0121 passes diagnostic argument values to the trace helper for selected events, but its compact renderer currently displays only `POST-PLATFORM EVENT N`. The values are not visible on screen. Use the event ID to locate the last completed source boundary; do not claim the associated memory or CPU values were observed.
- The recording contains no device-side image digest and is not checksum-bound to Run #320 or Run #322.

## Run #323 failure and post-platform trace patch repair — 17 November 2026

- **Run #323** (`44041091676`, source `c2d60d1ac7fe596e85c374d1221e3f7f397eb5f9`) compiled XNU and produced a raw kernel candidate, but failed the required-patch guard. Its build log identifies the exact skipped patch: `0121-lavender-arm-init-machine-startup-trace.patch`, with `patch failed: osfmk/arm64/sweet_early.c:425`. The RAM-only package job was skipped. Do not boot the raw candidate.
- **Root cause:** patch 0121 tried to modify the post-platform event renderer in the historical combined `sweet_early.c` translation unit. The current workflow splits the Lavender implementation into `lavender_early.c` after applying the patch stack, so that renderer hunk no longer has stable source context.
- **Correction:** remove the renderer hunk from patch 0121 and make `tools/split-lavender-early.py` install/verify the generic event 47–71 renderer in the generated Lavender translation unit. The machine-startup and `machine_conf()` instrumentation remains in patch 0121. Follow-up commit `1a279d22554357bca7d1509e7f1d75d580b4daf4` corrected the helper's escaped source matcher.
- **Validation runs:** #324 (`44056533652`), #325 (`44056539239`) and #326 (`44056634170`) were triggered by the repair sequence. #326 is the first run containing the final matcher correction. Accept no new image until the required-patch guard, all source guards, linked Mach-O verification and separate RAM-only packaging pass.
- **Physical evidence correction:** the latest user-supplied frame sequence does not establish event 39. The last clearly readable XNU checkpoint remains event 38, `ARM_INIT PE_PLATFORM DONE`; `MACHINE_STARTUP BEGIN` is not yet confirmed. Do not claim that `machine_startup()`, `machine_conf()` or `kernel_bootstrap()` has executed on the handset based on this footage.
- **Next physical diagnostic:** after a fully accepted image is packaged, observe events 39, 47–50, 62–71 and then event 41. Events 66/68–71 distinguish entry/exit through `machine_conf()`; event 65 is immediately before `kernel_bootstrap()`; event 41 is its first instruction-level trace boundary. The current renderer displays event numbers, not the argument values.
- **Separate userspace blocker remains:** Lavender SDHC1/eMMC root-device support and the read-only HFS+ mount path are not implemented. Reaching event 41 would prove XNU bootstrap entry, not Darwin userspace or a mounted iOS root.

### Patch 0121 follow-up — machine_conf hunk restoration and count validation

- During the renderer-hunk removal, the trailing `machine_routines_common.c` diff was also dropped. This would have removed events 68–71 from `machine_conf()`. Restored that complete diff; events 68–71 are required to distinguish the memory-size assignments and CPU-topology dereference.
- A unified-diff audit then found stale hunk counts in three retained sections. Corrected all eight hunk headers and independently recounted every old/new line total; all eight now match their declared counts.
- Latest patch commits: `d9bd8b41e98ea6678501d0465518be2b55878e96` (restore machine_conf section) and `5ea46bd43798a2a719b95563074e0068de53d696` (restore accurate hunk counts). Runs #327 and #328 were queued for these successive patch snapshots; #328 contains the count-corrected patch and is the preferred acceptance candidate.
- Runs #324–#326 are intermediate snapshots. Do not accept their artifacts as the new diagnostic image. Only #328 or a later run that passes patch application, source guards, linked verification and RAM-only packaging can replace the Run #320 baseline.

### Additional handset-frame review — 17 November 2026

- The newly supplied frame sequence shows the red shim handoff, then the genuine AppleLogo with the XNU verbose console, followed by a Redmi / “Powered by Android” boot splash in frame `lav_110.png`. The handset therefore restarted after the XNU screen.
- The supplied console frames are too overexposed to establish a later event number beyond the previously documented last clearly readable event 38. They do not show a legible XNU panic, Guardian reason string, or reset diagnostic.
- Do not attribute the restart to `machine_conf()`, the A53 Guardian, a watchdog, low battery, or a particular kernel fault without a device-side marker or serial/panic evidence. Keep the reset cause open.

## Run #330 accepted image and post-platform trace repair — 17 November 2026

- **Run #330** ([44063916673](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44063916673), source commit `b24b4a2e789a6c2a610b6b25a2640a80c3d81478`) completed successfully. The XNU build job passed compilation/linking, the required Lavender entry-guard patch, all source guards, linked ARM64 Mach-O verification and artifact upload. The separate `Package RAM-only Lavender fastboot image` job also passed.
- **Patch 0121 repair:** commit `17d6b4b8f79537fb0ee88c963ab25873c3b81709` corrected malformed unified-diff hunk counts and removed an orphaned `sweet_early.c` diff header. The renderer update in `tools/split-lavender-early.py` (commit `b24b4a2e789a6c2a610b6b25a2640a80c3d81478`) now prints the diagnostic `a=` and `b=` values for events 47–71 when nonzero. Its idempotency guard checks for the new renderer body rather than mistakenly treating the shared event-range condition as proof that the renderer was already upgraded.
- **Accepted artifact:** artifact `14320736107`, `lavender-fastboot-boot-test`, downloaded and inspected. It contains `iosboot.img` at **16,601,088 bytes**. Independent SHA-256 of the extracted image: **`514997704164e6745d8ebcd6f0e73966b2dd615ced80a2702ebd94836f0fe9d5`**. The artifact's `SHA256SUMS` records the same digest for `shim/out/shim-boot.img`; the extracted `iosboot.img` hashes identically.
- The matching `BUILD-MANIFEST.txt` identifies source commit `b24b4a2e789a6c2a610b6b25a2640a80c3d81478`, target Redmi Note 7 / lavender / SDM660, boot CPU MPIDR `0x100`, and **fastboot boot only**. Do not flash this image to any partition.
- This CI acceptance establishes a structurally verified RAM-only boot image and the presence of the post-platform trace instrumentation. It does **not** establish that the handset has passed the AppleLogo, entered Darwin bootstrap, mounted an iOS root, or reached launchd/SpringBoard. The latest physical footage predates this image; do not carry its last-observed event forward as a result for Run #330.
- **Next physical diagnostic:** test this exact image using RAM-only `fastboot boot iosboot.img` and record the last visible event ID plus any printed `a=`/`b=` values. Prioritize the post-logo path: events 39, 47–50, 62–71, then 41. Event 41 is the first trace inside `kernel_bootstrap()`; it is still not proof of Darwin userspace. If the console remains only at the AppleLogo, capture a checksum-bound recording and the final framebuffer/Guardian marker before changing startup code.
- Recovery remains confirmed through the user's volume-key route. The separate watchdog-first fallback remains unverified. Preserve RAM-only testing; never flash, erase, repartition or write internal Android eMMC/userdata.


## Run #330 handset result and PE_init_cpu / GIC trace follow-up — 17 November 2026

- Reviewed user recording `1000317866.mp4` (43.18 seconds). The clearest console frame shows post-platform events 47, 49, 50, 51, 52 and 53. Event 48 is absent, consistent with the non-error CPU-configuration path. The last clearly readable event is **53**, emitted after the CPU timebase and optional KPERF timer block, immediately before `PE_init_cpu()`.
- The recording then shows the display going dark and a Redmi / Powered by Android boot sequence. It establishes a restart, not its cause. No legible panic reason, synchronous exception record, Guardian reset reason, or image checksum is present. Do not attribute the restart to a watchdog or a specific GIC register access yet.
- Inspected the exact Run #330 `xnu-buildtree` artifact `14320452658`. In that compiled tree, `PE_init_cpu()` calls `pe_init_fiq()`; the Lavender board profile defines `HAS_GIC_V3 1`; and the active `pe_fiq.c` path performs GICv3 device-tree lookup, register-property parsing, SoC base lookup, and GICD/GICR MMIO mapping before returning to event 54.
- Cross-checked `tools/mkadt-lavender.py`: the generated Lavender ADT deliberately creates `/arm-io/gic`, sets `arm,gic-v3`, and provides the four GICD/GICR `reg` cells expected by `pe_fiq.c`. The suspected device-tree path mismatch is therefore **not supported** for the generated ADT.
- Added diagnostic-only patch `0122-lavender-pe-fiq-trace.patch` to split the event-53-to-54 gap. Events 72–76 trace `PE_init_cpu()` entry, optional stop-clocks handling, and `pe_init_fiq()` return. Events 77–87 trace entry into `pe_init_fiq()`, GIC DT lookup/property parsing, SoC base, and GICD/GICR mappings. The generated Lavender console now renders events through 87 with argument values.
- This is instrumentation, not a hardware behavior change. Do not infer a GIC fix from the current video. The next image must pass the complete required-patch guard, source-order guard, compile/link, ARM64 Mach-O verification, and separate RAM-only package job before handset testing.
- Next diagnostic interpretation: event 72 absent means the stop is before `PE_init_cpu()`; 72 without 75 localizes to optional stop-clocks handling; 75 without 76 is inside `pe_init_fiq()`. Events 78–87 then identify the DT and MMIO mapping boundary. Event 54 confirms `PE_init_cpu()` returned; event 55 confirms `fiq_context_init(TRUE)` returned.
- Preserve RAM-only `fastboot boot iosboot.img`. Never flash, erase, repartition, or write internal Android eMMC/userdata.


## Run #331 accepted diagnostic image and PE_init_cpu / GIC trace — 17 November 2026

- **Run #331** ([44075155032](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44075155032), source commit `748d7d1c75fa0af5e3c178d510167b3314c21c12`) completed successfully. Both the XNU build job and the separate `Package RAM-only Lavender fastboot image` job passed. The build job passed required-patch/source guards, XNU 8796 compilation, linked ARM64 Mach-O verification and kernel artifact upload. The package job passed kernel-artifact retrieval, Lavender Mach-O check, OTA/stock boot extraction, generated ADT validation, payload construction, fastboot image construction, boot-image structure/payload identity checks and artifact upload.
- **Accepted artifact:** `lavender-fastboot-boot-test`, artifact ID `14325018991`. Its ZIP contains `iosboot.img` (16,617,472 bytes), `shim/out/shim-boot.img` (16,617,472 bytes), `artifacts/lavender-payload.bin` (16,045,968 bytes), and `artifacts/lavender.adt` (10,072 bytes). The extracted `iosboot.img` is recognized as an Android boot image (kernel offset 0x8000, ramdisk offset 0x1000000, 4096-byte page size) and is byte-identical to `shim/out/shim-boot.img`.
- **Image SHA-256:** `f2b2bb70d236213f02fb8b3e25780a6b337ea8de26890c24ae11bb85a1fd806b`. Payload SHA-256: `8d62855897048bdb9ec82237701aa4a33be4c630deddfc882ba01a59c204c91e`. ADT SHA-256: `8c126d6367c02ed586a21ba466313062ae9fcb42b8f86e948f5de47f27057332`. The artifact manifest identifies the target as Redmi Note 7 / lavender / SDM660 and explicitly says **fastboot boot only; do not flash to a partition**.
- Patch 0122 and its event renderer through 87 are included in this build. This CI run proves the diagnostic source compiled and was packaged; it does not prove the handset reached any of events 72–87. No physical test or checksum-bound recording for this image is available yet.
- **Next physical test:** transfer this exact `iosboot.img` to the handset and use RAM-only `fastboot boot iosboot.img`. Record the last visible event and any arguments. Event 72 marks `PE_init_cpu()` entry; 75 is immediately before `pe_init_fiq()`; 76 is after it returns; 78–87 split GIC DT lookup, property parsing, SoC-base lookup, and GICD/GICR mappings; 54 and 55 remain the outer return boundaries. If the phone restarts, capture the final visible event and any panic/Guardian reason without assuming the cause.
- Preserve the confirmed volume-key recovery route. Never flash, erase, repartition, or write internal Android eMMC/userdata.

## Run #331 handset result — post-GICR-map stop and next trace — 17 November 2026

- Reviewed the newly supplied recording `1000317878.mp4` (37.53 seconds, 1024×576 at 30 fps). The verbose console visibly prints events 47, 49–53, 72, 75, 77–87. Events 73/74 are not present in the visible trace; no conclusion beyond the observed output is required for the stop-clocks branch.
- **Last visible checkpoint: event 87**, emitted immediately after `ml_io_map()` returns for the GIC redistributor region. The recording does not show event 76 (`pe_init_fiq()` returned), event 54, or event 55. It then shows the display going dark and the Redmi boot splash. This establishes a restart after the last visible diagnostic checkpoint, not its cause. The video does not contain a readable panic/exception/Guardian reset reason or a checksum tied to the image.
- The exact Run #331 `xnu-buildtree` artifact `14326350345` was inspected. After event 87, `pe_init_fiq()` assigns `gicr_size`, calls `find_gicr_pe_base()` (reads `GICR_TYPER` while matching the current MPIDR affinity), reads `GICR_WAKER`, may clear `ProcessorSleep` and wait for `ChildrenAsleep` to clear, configures `GICR_IGROUPR0` / `GICR_ISENABLER0`, accesses `ICC_SRE_EL1`, configures the priority/binary-point and EOI registers, then enables the GIC distributor Group 0 and `ICC_IGRPEN0_EL1`. Event 76 is after the entire function returns.
- The current evidence therefore narrows the unresolved interval to the post-map portion of `pe_init_fiq()`. It does **not** distinguish a fault during the first `GICR_TYPER` access, affinity mismatch, redistributor wake timeout, GICR interrupt configuration, ICC system-register access, or distributor/group enable. Do not change Group 0/Group 1 routing or PPI 27 based only on this recording.
- Added diagnostic-only patch `0123-lavender-pe-fiq-post-map-trace.patch`, locally dry-run/apply validated against the exact Run #331 buildtree. Events 88–93 cover MPIDR/GICR size, each redistributor base and `GICR_TYPER` read, affinity match, `GICR_TYPER.Last`, and the no-match panic boundary. Events 94–98 cover GICR size assignment, redistributor selection, `GICR_WAKER` read and wake completion. Events 99–103 cover redistributor interrupt setup, ICC system-register and priority setup, distributor enable, and Group 0 forwarding. The instrumentation adds no extra MMIO or system-register readbacks and changes no GIC configuration values.
- The renderer is extended through event 103 and the workflow guard now requires patch 0123 and its source-order checks. This is a new diagnostic build, not a fix to GIC behavior. Accept a replacement image only after patch/source guards, XNU compile/link, ARM64 Mach-O verification, and the separate RAM-only package job all pass.
- Next test interpretation: last event 89 without 90 localizes the first `GICR_TYPER` read; 90 without 91/92 means the affinity comparison or next loop; 97 without 98 localizes the redistributor wake branch; 99/100/101/102/103 split the interrupt and CPU-interface setup; event 76 confirms return from `pe_init_fiq()`. Keep the reset cause open unless a panic or Guardian reason is captured.
- Preserve RAM-only `fastboot boot iosboot.img` and the confirmed volume-key recovery route. Never flash, erase, repartition, or write internal Android eMMC/userdata.


## Run #331 post-map footage, Run #332 cancellation and Group-1 timer hardening — 17 November 2026

- The new recording `1000317878.mp4` shows events 47, 49–53, 72, 75 and 77–87. The last clear checkpoint is **87**, immediately after `ml_io_map()` returns for GICR. Events 76 (`pe_init_fiq()` return), 54 and 55 are absent. The handset then restarts to the Redmi boot screen. The clip is not checksum-bound to the image and contains no legible panic, synchronous exception or Guardian reset reason.
- Exact Run #331 buildtree artifact `14326350345` shows the next code reads `GICR_TYPER` in `find_gicr_pe_base()`, matches the current MPIDR affinity, reads `GICR_WAKER`, potentially clears ProcessorSleep and waits for ChildrenAsleep to clear, then configures the redistributor and CPU interface before returning. Patch 0123 events 88–103 were added to split those steps.
- Run #332 (`44082306828`) ended with job `build` cancelled and the package job skipped. No compiler or patch error was surfaced; classify it as cancelled, not as a source failure. Do not use its raw candidate.
- A proactive audit found a later, definite timer-delivery mismatch in the prior generic Lavender path: the generated ADT declares virtual timer INTID 19, while the generic fallback enabled PPI 27 as Group 0; Lavender had no matching Group-1 IRQ timer dispatcher. Xiaomi's SDM660 device tree confirms timer PPI index 3, i.e. INTID 19, and a GICv3 controller. Arm's GIC architecture specification defines the required NS Group-1 acknowledge path.
- **Patch 0124** (`xnu/patches/0124-lavender-group1-vtimer.patch`) fixes only the Lavender path: it takes the PPI from the ADT and validates it; configures only that PPI as Group-1 NS; uses `ICC_BPR1_EL1`, `ICC_IGRPEN1_EL1` and a Lavender `ICC_IAR1_EL1` / `ICC_EOIR1_EL1` handler that calls `rtclock_intr(TRUE)`; verifies system-register and distributor/CPU-interface enables; bounds distributor RWP waiting; and retains the single-core `cpus=1` gate. It does not enable SGIs/IPIs or SMP, and leaves SWEET/generic board paths intact.
- **Important register correction:** for non-secure accesses with ARE_NS=1, Arm IHI 0069G defines GICD_CTLR bit 1 (`EnableGrp1A`) as an alias of secure-view `EnableGrp1NS`; bit 0 is RES0. Patch 0124 therefore sets and verifies only `GICD_CTLR_ENABLEGRP1A`. Firmware must already provide ARE_NS; XNU refuses the unsafe transition if it is absent.
- The patch was regenerated against the exact Run #331 tree after patch 0123. Local `patch -p1 --dry-run` and application passed in order; all eight patch-0124 hunk counts matched. The new `tools/verify-lavender-gic-route.py` static contract test passed against that same patched tree.
- Commit `61410e063a3dcfb3eac54c48832c677932cdb178` includes patch 0124, renderer range through event 105, the workflow guard and verifier. Run #333 ([44086398531](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44086398531)) is queued.
- This is a **later interrupt-path hardening fix**, not a demonstrated cause of the event-87 stop: event 87 occurs before `find_gicr_pe_base()` and before timer-group setup. Events 88–105 remain necessary to localize the current stop. Require all source guards, XNU compilation/link, ARM64 Mach-O verification and the separate RAM-only package job before accepting a replacement image.
- Next physical interpretation after the new image is accepted: events 89/90 split the first GICR_TYPER read; 91 confirms affinity match; 92 shows Last; 94/95 bracket redistributor selection; 96/97/98 bracket WAKER; 99 is PPI/priority setup; 100 SRE; 101 PMR; 102/103 distributor state/enable; 104 Group-1 CPU interface; 105 EOI mode; 76 confirms `pe_init_fiq()` return. Keep reset cause open absent direct evidence.
- Preserve RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write internal Android eMMC/userdata.

References: [Xiaomi SDM660 device tree](https://github.com/xiaomi-sdm660/android_kernel_xiaomi_sdm660/blob/11-EAS/arch/arm/boot/dts/qcom/sdm660.dtsi), [Arm GIC architecture specification, IHI 0069G](https://www.scs.stanford.edu/~zyedidia/docs/arm/gic_v3.pdf).

## Patch 0125 DS-aware GIC security semantics and ADT topology guard — 17 November 2026

- Patch 0125 adds comments to the exact post-0124 GIC source: when GICD_CTLR.DS=0, `GICR_WAKER` and `GICR_IGROUPR0` are Secure/RAZ-WI to Non-secure EL1. In this mode secure firmware must leave the boot redistributor awake and assign the ADT timer PPI to Group-1 NS. When DS=1, XNU's current conditional wake and targeted group assignment are available. The patch is documentation-only and changes no register writes.
- `tools/verify-lavender-gic-route.py` now enforces those firmware preconditions and checks that the WAKER write remains conditional on an observed `ChildrenAsleep` bit; it also preserves the DS=1 targeted PPI group-write path. A successful static check does not prove firmware has configured DS=0/1 correctly or that a timer IRQ physically arrives.
- The ADT generator now emits `cluster-type` ASCII `E` for A53 cluster0 and `P` for A73 cluster1, based on the audited SDM660 CPU MPIDRs. Unknown Aff1 cluster IDs are rejected. The ADT's own validator and `tests/test_lavender_adt.py` verify both all-eight-core and A73-only layouts.
- `.github/workflows/xnu.yml` requires patch 0125 and runs the ADT regression immediately after machine-profile validation, before the long XNU compile. This catches topology regressions early.
- Commit `0fb0e5c300b393bc4b5323783c930f0a38e9b958` contains the code/test/workflow changes. The currently executing Run333 belongs to the prior source commit and does not include these changes; a fresh push-triggered run must pass all source, compile/link, Mach-O and RAM-only package gates.
- No SMP behavior is enabled: package bootargs still require `cpus=1`. Root storage, HFS+ and Darwin userspace remain unimplemented milestones.
