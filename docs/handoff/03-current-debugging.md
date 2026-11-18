## Latest accepted image — Run #357 (18 November 2026)

- **Run:** [#357 / 44271639569](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44271639569), branch `ci/lavender-machine-profile`, source commit `fc9da9a76444fafc2a2a2d40bd1fd71c2104a1ad`.
- **Result:** success. The XNU build job passed the machine-profile and ADT topology checks, XNU 8796 build, Lavender entry guard, non-secure Group-1 virtual-timer contract, IOKit platform-match/early-init trace verifier, kernel artifact verification and upload. The independent `Package RAM-only Lavender fastboot image` job passed kernel/Mach-O checks, pinned OTA and DTB extraction, Lavender ADT generation, XNU payload construction, boot-image construction, structural and payload identity verification, and upload.
- **Artifact:** `lavender-fastboot-boot-test`, ID `14414465679` (artifact ZIP SHA-256 `ea41b37e8af83fadb0dd06ea71bf3888152b32fff12e600e963a009ea5749fdb`). The extracted `iosboot.img` is **16,617,472 bytes**, SHA-256 `97d2fb54ea8367b6236f3e41d13a43ac1cdcce4c28bb26632e020cb89dc5ad7f`. Independent local hash equals both `BUILD-MANIFEST.txt` and `SHA256SUMS`; `file` identifies it as an Android boot image (4 KiB pages). Kernel SHA-256 `b1c716b9c42c6982ae0319ed2a21526992cbf82ee4ebaf02e4d339baa71e53e8`; payload SHA-256 `3ae25b83c15376a2a3999900eea4c34e13804bffd692c8bb78325f65a49f2638`; DTB SHA-256 `bcb3848e080bcf058f0b32ebf2ec440b6f638fed144c182766aa80a45bb9bb22`.
- **Exact bootargs:** `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. The framebuffer console is disabled in this image; do not expect the KBOOT text console output used in the earlier recording. `cpus=1` remains mandatory.
- **Physical evidence caveat:** the user clarified the newly supplied approximately four-minute sequence was filmed using an older image. It is not a Run #357 test and is not checksum-bound to Run #357. Do not use it to claim progress or regression in this candidate. A fresh RAM-boot recording is required to evaluate Run #357 physically.
- **Prior failed candidate:** Run [#355 / 44264884787](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44264884787) failed and is superseded. Run #357 is the newest accepted CI/package artifact at this handoff.
- **Next:** test this exact image using `fastboot boot iosboot.img` only; preserve the phone's volume-key recovery route. Capture whether the device remains powered, resets, or reaches a visible marker. Since `lavender_no_fbcon=1`, use independent serial/other existing diagnostic output if available; do not infer a hang from a blank display alone. Correlate any recording with the image SHA-256 before interpreting it. CI acceptance is not proof of physical boot, BSD init, root mount, launchd, SpringBoard or complete Darwin userspace.
- Never use `fastboot flash`, erase, repartition or write Android eMMC/userdata. No speculative GIC edits; preserve `cpus=1`.

---

## Run #352 failure — patch phase precedes Lavender source split; corrected — 18 November 2026

- Run [#352 / 44262063956](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44262063956), commit `6cfa604a02516d3016671d7a8526cee4f8c5283d`, again compiled XNU but failed the required entry-guard gate; packaging was skipped.
- Inspected the exact `xnu-build-log` artifact (`14408748561`) and buildtree (`14408873436`). The exact reason was `0126-lavender-iokit-bringup.patch` skipped with `error: osfmk/arm64/lavender_early.c: No such file or directory`. The workflow applies the complete XNU patch stack first, then runs `tools/split-lavender-early.py`; therefore `lavender_early.c` does not exist when patch 0126 is applied. This is a build-phase ordering issue, not a compile/runtime failure. The buildtree confirms the split script later generated `lavender_early.c`.
- Corrective design: keep patch 0126 limited to the pre-split XNU files (`IOStartIOKit.cpp`, `KernelConfigTables.cpp`, and `pe_init.c`); remove its direct diff against the not-yet-created Lavender early source. Move event-label insertion (107–125) into `ensure_post_platform_renderer()` in `tools/split-lavender-early.py`, which runs after the source split and is also idempotent for already-split source trees.
- Follow-up commits: `4e742376c65a60807651180d1052ca33d52f1fd7` (patch cleanup), `b7a8ba022577e23b5dc9a97fe38100288c374af4` (post-split label injection), and `28aed7d5165e4b121436053c827c830151da8dcd` (correct Python escape sequences so the injector matches literal tab/newline source text).
- Latest complete correction is commit `28aed7d5165e4b121436053c827c830151da8dcd`. Run [#355 / 44264884787](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44264884787) is queued from this final commit. Runs #353/#354 were triggered by intermediate commits and are not acceptance candidates.
- Acceptance: #355 must show patch 0126 applies without skip, source split reports PASS, generated `lavender_early.c` contains event labels 107–125, required guard passes, then linked verification and separate RAM-only package/upload pass. No image from #352 or intermediate runs is accepted.
- Keep `cpus=1`, no GIC changes, and RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery.

## Run #351 failure and patch-path correction — 18 November 2026

- Run [#351 / 44252286411](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44252286411) compiled XNU 8796 successfully, then failed at the required Lavender entry-guard/source verification step. Packaging was skipped; Run #351 produced no accepted `iosboot.img`.
- Downloaded and inspected the exact Run #351 build log artifact (`14404698719`) and buildtree (`14405226708`). The log shows the precise patch failure: `0126-lavender-iokit-bringup.patch` was skipped because its final diff targeted `osfmk/arm64/sweet_early.c` at line 288, but the early-source split had already moved the Lavender implementation to `osfmk/arm64/lavender_early.c`. The split verifier itself passed. Because patch 0126 was skipped, its PE/IOKit trace labels and the platform-expert match correction were absent from the built tree; this was a patch target/order defect, not a compiler failure and not yet a runtime IOKit fix.
- Corrected patch 0126 by retargeting the early-trace diff paths from `sweet_early.c` to `lavender_early.c`, preserving the existing hunk context and all IOKit instrumentation/config changes. Commit: `6cfa604a02516d3016671d7a8526cee4f8c5283d`.
- Corrective XNU workflow Run [#352 / 44262063956](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44262063956) was triggered by that commit and is queued as of 13:20 IST. Wait for the required-patch guard, build, linked-kernel verification, and packaging to finish before accepting an image.
- Keep `cpus=1`; no GIC edits. Any resulting image remains for RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery.

## CI correction: Run #343 succeeded but did not publish iosboot.img — 17 November 2026

- Run [#343 / 44123579468](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44123579468), source commit `b69f4a27a78ec5c60ce882c5ab0c329e1273acbb`, completed both XNU build and RAM-only image construction jobs successfully.
- The package job built `shim/out/shim-boot.img` (16,617,472 bytes) and copied it to `iosboot.img`. The package job's verification script is truncated: its embedded Python heredoc ends immediately after importing `re`; the runner logs a shell warning that the expected `PY` delimiter was not found. The step nevertheless exits zero because the incomplete Python snippet itself succeeds. Therefore Run #343's “success” is not evidence that the intended identity checks executed.
- The main `xnu.yml` package job also has no `actions/upload-artifact` step for `iosboot.img`. Its artifact list contains only `xnu-build-log`, `xnu-kernel`, `xnu-kernel-candidate`, and `xnu-buildtree`. No image artifact ID, manifest, or image SHA-256 exists for Run #343.
- Corrective action: repoint the dedicated verified image publisher to Run #343's kernel artifact and source commit. The publisher is configured to run structural and byte-identity checks for shim kernel, XNU payload and SDM660 DTB, calculate SHA-256, write a manifest, and upload `iosboot.img` as `lavender-fastboot-boot-test`.
- Publisher run [#3 / 44138288151](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44138288151) is in progress. Its kernel download and ARM64 Mach-O selection succeeded; dependency installation is underway. No artifact is available yet.
- Do not distribute any unverified runner-local image from Run #343. The previous downloadable image from Run #2 / artifact `14340728816` uses the older Run #337 kernel and predates the GIC affinity-routing changes; it is not the current candidate.
- Acceptance requires publisher run #3 to pass every verification step and upload the artifact. Then independently check the downloaded image's size and SHA-256 against `BUILD-MANIFEST.txt`. Physical test only with RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition, or write Android eMMC/userdata. Keep `cpus=1` and preserve the confirmed volume-key recovery/reset route.

## CI result: Run #341 — verifier false positive — 17 November 2026

- Run [44117499917](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44117499917) completed XNU 8796 compilation successfully, then failed at step 15, `Verify Lavender non-secure Group-1 virtual timer contract`.
- Exact failure: `FAIL: non-secure Group-1 setup must not set the RES0 Group1 bit when ARE_NS is enabled`.
- Root cause: the static verifier rejected the token sequence `GICD_CTLR_ENABLEGRP1A | GICD_CTLR_ENABLEGRP1` anywhere in the Lavender GIC branch. The updated DS=1 safety precondition correctly uses that combined mask to *test* whether either group is already enabled; it does not write the combined mask. The actual write remains `gicd_ctlr |= GICD_CTLR_ENABLEGRP1A;`.
- Corrected the verifier to distinguish the DS=1 precondition read-mask from the distributor write, and to assert that only `GICD_CTLR_ENABLEGRP1A` is ORed into the write value. This is a verifier-only correction; no GIC runtime code changed in response to this CI failure.
- Packaging was skipped because the source contract gate failed. No image from Run #341 is valid for device testing.

## Physical test: GICD_CTLR affinity-routing transition — 17 November 2026

- The four split user recordings are treated as one test sequence. They show successful XNU progress through `PE_CREATE_CONSOLE DONE`, `PE_INIT_PRINTF DONE`, `CPU_IDLE_INIT DONE`, and `PE_PLATFORM BEGIN/DONE`, followed by post-platform trace rows through **event 102** and a visible panic: `pe_init_fiq: GICv3 non-secure affinity routing is not enabled by firmware`.
- The footage is not checksum-bound to the image bytes, so this is user-attributed physical evidence, not a cryptographically tied test of a particular artifact. It nevertheless supersedes the previous event-87-only physical boundary for this recording.
- Event 102 is `LAVENDER_FIQ_TRACE(102, gicd_ctlr, 0)`. The console renderer prints event arguments only when either argument is nonzero. The event-102 row has no argument fields; the observed `GICD_CTLR` value is therefore **0x00000000**. This is consistent with the explicit panic on the next source line.
- The captured `GICD_CTLR=0x00000000` also shows DS bit 6 clear, consistent with the two-security-state Non-secure view (DS=0). In that view, bit 4 (`0x10`) is the read/write alias of `ARE_NS`; bit 0 is the legacy `EnableGrp1NS` alias while `ARE_NS=0`; and bit 1 becomes the `EnableGrp1NS` alias after `ARE_NS=1`. Patch 0085 now names the DS bit explicitly, and patch 0124 handles both views: DS=0 requires legacy Group-1 NS clear; DS=1 requires both Group 0 and Group 1 clear before changing ARE.
- Arm IHI 0069G §2.3.3 says the DS=0 `ARE_NS` transition 0→1 is architecturally permitted only while `EnableGrp1NS=0`; poll `GICD_CTLR.RWP` so the change is visible before proceeding. The observed initial control value is zero, so the legacy Group-1 NS enable precondition is satisfied. This is a guarded transition, not permission to write arbitrary control bits. See [Arm GIC architecture specification IHI 0069G §2.3.3](https://www.scs.stanford.edu/~zyedidia/docs/arm/gic_v3.pdf).
- Corrected patch 0124 to: (1) refuse the transition if legacy Group-1 NS is already enabled; (2) set only `ARE_NS`; (3) wait for RWP with the existing machine timeout and verify the bit read back; (4) only then set `EnableGrp1A`, wait for RWP and verify Group-1 distribution; and (5) retain the CPU-interface readback checks. Trace IDs are now 102 = initial control read, 103 = ARE_NS readback, 104 = distributor Group-1 readback, 105 = CPU-interface Group-1 forwarding, and 106 = EOI mode.
- `tools/verify-lavender-gic-route.py` now guards the precondition, ordering and readback. The generated console renderer and workflow source check cover event 106. This is a source-level correction; physical completion of `pe_init_fiq()`, timer interrupt delivery, storage, root mount and Darwin userspace remain unproven until a new accepted image is tested.
- Next acceptance gates: XNU source/patch guards, compilation/link, ARM64 Mach-O verification, and separate RAM-only packaging must all pass. Keep `cpus=1`. Physical testing stays RAM-only via `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve the confirmed volume-key recovery/reset route.

---

## Accepted RAM-only Lavender boot image — Run #2 (17 November 2026)

<badge>CI verified and artifact downloaded</badge>

- Packaging workflow: [Run #2 / 44113290781](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44113290781), publisher commit `9256d45d5d33281d082a9fa970fbb499349e0416`; all 13 package steps succeeded, including image construction, structural/payload identity verification, SHA-256 generation and artifact upload.
- Source XNU build: Run #337 / `44105853768`, source commit `f9737461fc21f3181600a39c5ff6d71716baa045`.
- Artifact: `lavender-fastboot-boot-test`, ID `14340728816`, uploaded 17 November 2026; artifact ZIP size 5,838,494 bytes.
- `iosboot.img` size: **16,617,472 bytes**.
- SHA-256: `32d286522811bee3507f41032e84202b5bc735c59added615bfb28a53cd84b5c`.
- Downloaded artifact ZIP and extracted image were inspected locally. The manifest size and SHA-256 match the independently computed local size/hash; `file` identifies it as an Android boot image with 4 KiB pages.
- Image composition: shim in boot-header kernel slot, XNU payload in ramdisk, pinned Lavender SDM660 DTB. Bootargs: `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`.
- **Acceptance scope:** packaging and byte identity are verified. This does not mean the image has been physically boot-tested or that Darwin userspace, launchd, or SpringBoard has been reached.
- Physical test only: `fastboot boot iosboot.img` (RAM-only). Never use `fastboot flash`, erase, repartition, or write internal Android eMMC/userdata. Preserve the confirmed volume-key recovery/reset route.

---

## Repackaging rerun status — 17 November 2026

- Corrected publisher commit: `9256d45d5d33281d082a9fa970fbb499349e0416`.
- Fresh repackaging run #2: [44113290781](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44113290781). The job has started and is downloading the Run #337 XNU kernel artifact. No output artifact exists yet; wait for the corrected shim/ramdisk/DTB identity checks and upload step.
- Do not treat Run #1 (44110862946) as an accepted image. Its build output was not uploaded and no checksum was published.

# Latest image publication attempt — verifier correction (17 November 2026)

- Run #337 (44105853768), source commit `f9737461fc21f3181600a39c5ff6d71716baa045`, completed successfully and published the `xnu-kernel` input artifact. The initial dedicated repackaging run #1 (44110862946) successfully downloaded that kernel, selected the ARM64 Mach-O, verified the pinned Lavender OTA, extracted the SDM660 DTB, generated the Lavender ADT, built the XNU payload, and produced `iosboot.img`.
- The image build itself completed. Verification failed at the first kernel comparison because the verifier compared the unpacked boot-header kernel to the XNU Mach-O. This image format intentionally places the ARM64 shim at the boot-header kernel slot and the XNU payload in the ramdisk. The log reports a 155,648-byte shim kernel and a 16,046,784-byte ramdisk; the XNU Mach-O is 14,368,098 bytes before payload wrapping.
- The correct identity checks are: unpacked boot kernel equals `shim/out/Image`; unpacked ramdisk equals `artifacts/lavender-payload.bin`; unpacked DTB equals the pinned stock Lavender SDM660 DTB. The latter two checks were not reached in the failed run because the first comparison exited nonzero.
- Corrected `.github/workflows/publish-run337-image.yml` in commit `9256d45d5d33281d082a9fa970fbb499349e0416` to compare the unpacked kernel against `shim/out/Image`. The push should trigger a fresh packaging attempt. Await that run's successful verification and uploaded `lavender-fastboot-boot-test` artifact before accepting or distributing an image.
- No artifact was uploaded by failed run #1; it has no artifact ID, accepted image size, or SHA-256. Do not use or distribute any local runner output from that failed run.
- Source kernel remains Run #337 / commit `f9737461fc21f3181600a39c5ff6d71716baa045`. Boot arguments remain `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`.
- Physical testing remains RAM-only: `fastboot boot iosboot.img`. Never flash, erase, repartition, or write internal Android eMMC/userdata. Preserve the confirmed volume-key recovery/reset route.

---

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


## eMMC ADT preflight and Run #334–#336 hardening — 17 November 2026

- Commit `474abdc2daa0070251ac137d8571bd0f45eec4f9` strengthens `tools/mkadt-lavender.py` and `tests/test_lavender_adt.py` to check exact SDHC1/eMMC host/CQHCI/ICE register windows, host/power IRQ specifiers and names, clock names, 8-bit bus width, non-removable flag and enabled status. This is a guard on existing hardware metadata; no kernel storage-driver behavior changed.
- Run #336 ([44090184786](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44090184786)) is queued for that exact source commit; the parallel Lavender shim CI job is also running. Run #335 ([44089917704](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44089917704)) carries the verifier whitespace fix; Run #334 ([44089189096](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44089189096)) carries the first cluster/GIC hardening changes. Do not treat any as accepted until XNU build, guards, ARM64 Mach-O verification and RAM-only package are green.
- The storage driver itself remains a later substantial task: first perform a no-write SDHC1 register/state probe, establish the Qualcomm V5 power/rail/pinctrl/IRQ sequencing, then implement bounded PIO reads only. CQE, ICE, DMA and writes stay disabled pending independent evidence. No eMMC read-only root or HFS+ mount is claimed.


## Engineering status snapshot — Run #336 compilation and Run #337 verification — 17 November 2026

### Current state

- Lavender XNU bring-up remains **in progress**. The current source head is `f9737461fc21f3181600a39c5ff6d71716baa045` on `ci/lavender-machine-profile`.
- Latest workflow: **Run #337** ([44105853768](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44105853768)). It is currently **in progress**, with the build job at **Save XNU build cache after successful compilation**. The machine-profile and ADT checks have passed. It has not yet completed the final static verification or RAM-only packaging.
- **Run #336** ([44090184786](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44090184786)) completed with failure after XNU 8796 compilation. Its failing step was `Verify Lavender non-secure Group-1 virtual timer contract`; the package job was skipped. The failure was identified in the verifier's comment matching, not in compilation. The verifier correction is in the Run #337 source commit. Do not classify Run #336 as a compiler failure and do not treat it as a packaged build.
- Latest physical handset boundary remains **event 87** from the Run #331 diagnostic image. There is no checksum-bound physical evidence showing progress beyond event 87, and no accepted new boot image from Runs #336 or #337 at this snapshot.

### Completed infrastructure and hardware-description work

| Area | Current evidence / limitation |
|---|---|
| Early boot tracing | Events 72–87 are present in the accepted Run #331 diagnostic image; events 88–103 are staged in the post-map trace patch series, but require a successful later build and handset test |
| GIC initialization | Instrumentation narrows the known physical stop to after GICR mapping; the actual event-87 failure cause is still unknown |
| Virtual timer routing | Lavender-specific non-secure Group-1 route is implemented in patch 0124, using the ADT-selected PPI and Group-1 acknowledge/EOI path; static verification is not proof that a physical timer interrupt is delivered |
| GIC security modes | DS=0/DS=1 access assumptions and firmware preconditions are documented and guarded; handset/firmware behavior is not yet physically demonstrated |
| CPU topology | A53 efficiency / A73 performance metadata is emitted and ADT regression-tested for full and A73-only profiles; secondary CPU startup remains disabled |
| eMMC description | SDHC1/eMMC register windows, IRQs, clocks, bus width and related ADT metadata have static checks; this is not an eMMC driver |
| XNU build | XNU 8796 compilation passed in Run #336; Run #336 later failed its timer-contract static verifier |
| RAM-only packaging | Still awaiting a later source run that passes all verification and produces a newly accepted `iosboot.img` |

### Remaining bring-up gates

1. Get Run #337 through all remaining static checks, ARM64 Mach-O verification, and the separate RAM-only package job. Accept an image only after the artifact is downloaded and its manifest/hash are verified.
2. Extend the physical GIC trace beyond event 87 using the post-map checkpoints, then correlate the handset's last event with the exact image hash.
3. Establish actual physical timer interrupt delivery; a static Group-1 configuration is insufficient.
4. Keep `cpus=1` until secondary CPU startup and interrupt delivery are proven.
5. Implement the SDHC1/eMMC path incrementally: read-only register/state probe, bounded PIO reads, GPT validation, then a BSD block device. CQE, ICE, DMA and writes remain out of scope until separately justified.
6. Validate a read-only root filesystem and its dependencies before attempting launchd and SpringBoard.

### Boot milestone honesty

The Apple logo is not evidence of Darwin userspace. At this snapshot there is no verified read-only root mount, no demonstrated launchd startup, and no SpringBoard. Do not claim Darwin userspace or a completed iOS boot until those stages are observed directly.

### Safety

Use **RAM-only** `fastboot boot iosboot.img` for handset testing. Never flash, erase, repartition, or write internal Android eMMC/userdata. Preserve the confirmed volume-key recovery/reset route. The separate watchdog-first fallback remains physically unverified.


## Live correction — Run #337 build passed; RAM-only packaging still running — 17 November 2026

This section supersedes the earlier Run #337 status line above, which was captured while the build job was still saving its cache.

### Verified CI state

- Run **#337** ([44105853768](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44105853768)) is still in progress on source commit `f9737461fc21f3181600a39c5ff6d71716baa045`.
- The **XNU build job has now completed successfully**. Its machine-profile validation, Lavender ADT memory/CPU-topology regression, exact-match cache restoration, XNU 8796 compilation, cache-aware build-path check, entry-guard patch validation, and the `Verify Lavender non-secure Group-1 virtual timer contract` check all passed. The kernel artifact was verified and uploaded, and the XNU build cache was saved.
- The separate **Package RAM-only Lavender fastboot image** job is currently running. Checkout passed and it is at **Download XNU kernel artifact**. This is not yet a completed package job. There is no newly accepted `iosboot.img` from Run #337 at this update; wait for packaging, boot-image structure/payload checks, and artifact verification.
- Run **#336** ([44090184786](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44090184786)) did compile XNU 8796 successfully, but then failed the final static check `Verify Lavender non-secure Group-1 virtual timer contract`. The job log identifies the issue as verifier comment matching, not a compiler failure. Its package job was skipped. The Run #337 verifier correction now passes.

### Consolidated engineering status

| Area | Verified state | What remains unproven / unfinished |
|---|---|---|
| Early boot tracing | Events 72–87 are in the accepted Run #331 diagnostic image; the last physical checkpoint is event 87 | Events 88+ still need to be in a successfully packaged image and observed on the handset |
| GIC initialization | Post-map trace instrumentation exists; DS=0/DS=1 security assumptions and firmware preconditions are documented and guarded | Exact event-87 failure cause and successful physical GIC initialization remain unresolved |
| Non-secure virtual timer | Lavender Group-1 NS route is implemented and the Run #337 static contract verifier passes | Actual physical timer interrupt delivery into XNU is not demonstrated |
| CPU topology | A53 efficiency / A73 performance metadata is generated; full eight-core and A73-only ADT profiles are regression-tested | Secondary CPU startup remains disabled; keep `cpus=1` until SMP and interrupt delivery are proven |
| eMMC description | SDHC1/eMMC register windows, host/power IRQs, clocks, 8-bit bus width, non-removable and enabled metadata have exact ADT checks | No Lavender SDHC1/eMMC kernel driver, block device, or verified root mount |
| XNU compilation | Run #336 and Run #337 both compiled XNU 8796 successfully; Run #337 also passed its final timer-contract static check | Run #337's separate RAM-only packaging and artifact acceptance are still pending |
| Darwin boot | No verified read-only root mount, launchd startup, or SpringBoard | Darwin userspace is not reached/demonstrated |

### Ordered next engineering passes

1. Finish Run #337's separate RAM-only package job. Verify the downloaded artifact, image structure, payload identity, manifest and SHA-256 before calling it an accepted image.
2. Use that exact image for a RAM-only handset test and capture the last visible event. Correlate the recording with the image hash; do not infer progress beyond event 87 from static checks alone.
3. Extend the post-map GIC trace and establish the exact source boundary at which execution stops. Separately prove physical non-secure Group-1 timer delivery; static route configuration is not enough.
4. Audit the startup path from `PE_init_cpu()` through the first userspace transition, but keep conclusions tied to observable milestones.
5. Keep `cpus=1` until secondary startup and interrupt delivery are proven.
6. Build eMMC support incrementally and read-only: first a no-write SDHC1 register/state probe, then bounded PIO reads, GPT validation, and only then a BSD block device. CQE, ICE, DMA and writes remain disabled until independently justified.
7. Validate a read-only root filesystem and its dependencies before attempting launchd; demonstrate launchd before attempting SpringBoard.

### Boot milestone and safety statement

The Apple logo is not the finish line and is not proof of Darwin userspace. At this update there is no demonstrated read-only root mount, launchd, or SpringBoard. The project is making meaningful progress in build infrastructure, hardware metadata, static guards and diagnostics, but Lavender XNU bring-up remains **in progress**.

All physical tests must use **RAM-only** `fastboot boot iosboot.img`. Never flash, erase, repartition, or write internal Android eMMC/userdata. Preserve the confirmed volume-key recovery/reset route. The separate watchdog-first fallback remains physically unverified.


## Run #3 publisher audit — verified RAM-only image — 17 November 2026

- Publisher workflow Run #3 ([44138288151](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44138288151)) completed with conclusion `success` on publisher commit `ad439183878d974aa374f4fc0adb6eca39be76b1`. Its single `package` job completed successfully; all 15 listed steps passed, including pinned OTA verification, ARM64 Mach-O kernel selection, payload construction, RAM-only image construction, image verification, and artifact upload.
- The image-verification step **did execute**. The job unpacked `iosboot.img` and byte-compared the extracted kernel against `shim/out/Image`, ramdisk against `artifacts/lavender-payload.bin`, and DTB against the extracted Lavender SDM660 DTB. It then computed SHA-256 and wrote `BUILD-MANIFEST.txt`.
- Artifact `lavender-fastboot-boot-test` (ID `14353840756`) uploaded successfully. GitHub reports ZIP size **5,838,924 bytes**. This is the compressed artifact archive, not the image's uncompressed size. Downloaded ZIP contains exactly `iosboot.img` (16,617,472 bytes), `SHA256SUMS` and `BUILD-MANIFEST.txt`; the ZIP listing reports 16,617,897 bytes total uncompressed.
- Independent local verification of the downloaded `iosboot.img`: `stat` reports **16,617,472 bytes** and `sha256sum` reports **4452bfe1057beac8213ad2daf7d6681d0866ddff2d6fb86561dc6ee506049c79**. Both values exactly match the artifact's `BUILD-MANIFEST.txt` and `SHA256SUMS`. The `file` utility identifies it as an Android boot image (kernel, ramdisk, 4096-byte pages); the CI's unpack-and-byte-compare checks passed.
- Manifest provenance: source workflow run `44123579468`, source commit `b69f4a27a78ec5c60ce882c5ab0c329e1273acbb`, target Redmi Note 7 / lavender / SDM660, bootargs include `cpus=1`, and mode is RAM-only fastboot boot. No speculative GIC changes were made by this publisher; this packages the pinned source-run kernel and OTA-derived DTB/payload.
- **Disposition: artifact accepted as byte-verified package output, not as proof of successful physical boot.** The 5.57 MiB artifact size shown by GitHub is explained by ZIP compression; the actual image is 15.84 MiB. It is newer than Run #337's artifact in this publisher's intended source lineage, but it remains a repackaging of Run #343's kernel/source commit above, not a new XNU compile.
- Physical test remains **RAM-only**: `fastboot boot iosboot.img`. Never flash, erase, repartition, or write Android eMMC/userdata. Preserve the volume-key recovery route. Keep `cpus=1`.


## Run 343 physical test footage — FEAT_LSE undefined-instruction panic — 17 November 2026

### Observed handset result

- New handset recording `1000317921.mp4` shows the Run #3 image progressing past the shim handoff and the visible `KBOOT ... BOOTSTRAP DONE` marker, then taking a synchronous exception. The on-screen panic text reads `Undefined kernel instruction`, with PC `0xfffffe00072dca54` and instruction word `0xf8280128`. The Apple logo overlay is not a successful Darwin/userspace milestone.
- The recording itself does not embed the boot image SHA-256, so it is not cryptographically bound to the artifact. However, the displayed PC and instruction word match the exact Run 343 kernel packaged into the verified Run #3 image.

### Binary-level match

- In Run 343's exact `xnu-kernel` artifact, PC `0xfffffe00072dca54` lands inside `___firehose_buffer_tracepoint_flush` in `__TEXT_EXEC`.
- The word at that address is exactly `0xf8280128`; independent AArch64 disassembly decodes it as `LDADD x8, x8, [x9]`.
- `LDADD` is an Arm FEAT_LSE large-system-extension atomic instruction. Arm documents FEAT_LSE as optional from Armv8.0 and mandatory from Armv8.1, and identifies it through `ID_AA64ISAR0_EL1.Atomic`. The Cortex-A73 technical reference manual's `ID_AA64ISAR0_EL1` layout has no Atomic field; the Cortex-A53 manual's documented baseline value also has Atomic=0. Snapdragon 660's Kryo 260 uses Cortex-A73 performance cores and Cortex-A53 efficiency cores. Thus the observed instruction is incompatible with the documented SDM660 core feature set and explains the Undefined Instruction exception.
- This is a concrete post-bootstrap ISA mismatch, not evidence of a GIC fault. Do not make GIC routing, PPI, redistributor or timer changes in response to this panic.

### Build provenance / likely source of incompatible code

- Run 343's XNU build log shows the main XNU compilation using `-march=armv8-a -mcpu=generic`; these flags do not request FEAT_LSE.
- The same kernel link command explicitly links `-lfirehose_kernel`. The source tree contains the call to `__firehose_buffer_tracepoint_flush` in `xnu/libkern/os/log.c` and its private declaration, but not its implementation. The final binary's faulting routine is therefore strongly indicated to come from the linked prebuilt Firehose kernel library/KDK, rather than from the main XNU C compilation. This attribution should be confirmed by inspecting the archive member/object or linker map before selecting the replacement.
- Do not assume that adding another `-march=armv8-a` flag to XNU fixes this. The source compile already carries that target; the linked Firehose implementation must be rebuilt/replaced with a baseline-compatible implementation, or gated by a correct runtime feature dispatch. Do not hand-patch the single `LDADD` instruction without auditing its atomic semantics and all other LSE instructions in the final linked kernel.

### Next narrowly scoped engineering task

1. Inspect the exact `libfirehose_kernel` archive/KDK provenance and identify the archive member that defines `___firehose_buffer_tracepoint_flush`; preserve its source/build inputs and compiler target in the engineering journal.
2. Inventory every FEAT_LSE opcode in the final linked Mach-O, not just this first observed fault. Check all executable segments and all archive-supplied code against the target's `ID_AA64ISAR0_EL1.Atomic` value.
3. Produce a build whose entire reachable early-boot path is Armv8-A-safe. Prefer a compatible Firehose library/object or a semantically correct non-LSE implementation. Add a CI disassembly/feature guard that fails if LSE encodings remain in code reachable before feature detection.
4. Keep `cpus=1`. No speculative GIC modifications. Require fresh compile/link, ISA audit, package verification, manifest/hash verification, and only then repeat a RAM-only handset test.

### Safety / test state

- Run #3 image remains **package-integrity verified**, but this footage demonstrates that the tested image panics and must not be described as a booting XNU build.
- Any retest must use RAM-only `fastboot boot iosboot.img`. Never flash, erase, repartition, or write internal Android eMMC/userdata. Preserve the volume-key recovery route.

References: [Arm FEAT_LSE overview and ID register](https://support.arm.com/documentation/ddi0487/mc/-Part-A-Arm-Architecture-Introduction-and-Overview/-Chapter-A2-A-profile-Architecture-Extensions/-A2-2-Armv8-A-architecture-extensions/-A2-2-2-The-Armv8-1-architecture-extension), [Arm LDADD instruction (FEAT_LSE)](https://documentation-service.arm.com/static/6023d5512cb3723f20208db2), [Cortex-A73 Technical Reference Manual](https://documentation-service.arm.com/static/5e7b6c837158f500bd5c03fc), [Cortex-A53 Technical Reference Manual](https://docs.arduino.cc/resources/datasheets/cortexa53.pdf), [Qualcomm Snapdragon 660 specifications](https://www.qualcomm.com/smartphones/products/6-series/snapdragon-660-mobile-platform), [Kryo 260 core composition](https://www.anandtech.com/show/11338/qualcomm-announces-snapdragon-660-630-mobile-platforms).


## FEAT_LSE Firehose remediation and Run #344/#345 — 17 November 2026

### Root cause refinement

- The Run 343 build log shows the XNU source compile using `-march=armv8-a -mcpu=generic`, while the final kernel links `-lfirehose_kernel`. The physical panic PC resolves to `___firehose_buffer_tracepoint_flush` from that separately built libdispatch Firehose library; the faulting word is `0xf8280128` (`LDADD x8, x8, [x9]`), a FEAT_LSE instruction unsupported by the documented SDM660 Cortex-A73/A53 core feature set.
- The pinned `darwin-xnu-build` helper builds `libfirehose_kernel.a` separately in `build_libdispatch()`, before building XNU, and only rebuilds it when `fakeroot/usr/local/lib/kernel/libfirehose_kernel.a` is absent. The Lavender workflow had expanded the library's valid architectures to include `arm64` but had not set a baseline Armv8-A target for that slice.
- CI cached both `dxb/build` and `dxb/fakeroot`. The Run 343 build log contains no `Building libdispatch` / Firehose-library rebuild output, consistent with the cached archive being reused. The old invalidation step deleted `dxb/build/xnu.obj` but did not remove the Firehose archive or its object directory. This cache path is therefore a concrete way for an older/default-target Firehose archive to survive a clean XNU rebuild.

### CI remediation committed

- Commit `d6b8d6dbacd0cf2fb7f070fda09d6ce83a51a02f` updates `.github/workflows/xnu.yml` so the separate `libfirehose_kernel` arm64 slice is built with `OTHER_CFLAGS='$(inherited) -march=armv8-a -mcpu=generic'`.
- The kernel cache key was bumped from v4 to `xnu8796-v5-armv8a-firehose-generic`. On a non-exact cache restore, the workflow now deletes `dxb/fakeroot/usr/local/lib/kernel/libfirehose_kernel.a`, `dxb/build/libfirehose_kernel.obj` and `dxb/build/libfirehose_kernel.sym` in addition to invalidating the XNU object tree. This forces the helper to rebuild the archive with the baseline target flags.
- Follow-up commit `3c53b34a02d57dbd9505352e62a2c935989e00ec` makes the focused disassembly guard reject LSE atomic mnemonics in `___firehose_buffer_tracepoint_flush`. It does not assume one specific LL/SC mnemonic pair; it checks the absence of FEAT_LSE operations in the exact faulting routine. The pre-existing `_lck_grp_reference` LDXR/STXR guard remains.
- Run **#344** ([44145280971](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44145280971)) was started from the first remediation commit and may not include the follow-up guard adjustment. Run **#345** ([44145429582](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44145429582)) is the current source revision with the focused Firehose guard. At this documentation update, both runs are still in progress; neither has yet been accepted as a new image.

### Acceptance criteria and unchanged constraints

1. Confirm Run #345 actually rebuilt `libfirehose_kernel.a` with the Armv8-A target after cache invalidation; do not accept a green XNU compile alone.
2. Require the exact Firehose routine disassembly guard to pass, plus existing kernel/ADT/GIC checks and XNU link verification.
3. Require the package job and independently verified image structure, manifest size and SHA-256 before issuing a new `iosboot.img`.
4. Only then repeat the physical test using RAM-only `fastboot boot iosboot.img`.

No GIC configuration was changed in this remediation. Keep `cpus=1`; never flash, erase, repartition or write Android eMMC/userdata; preserve the volume-key recovery route. The Run #3 image remains byte-integrity verified but is now known to panic with the FEAT_LSE undefined instruction and is not an acceptable boot-test candidate.


## Run #345 failure and corrected Firehose build target — 17 November 2026

### Exact Run #345 failure

- Run #345 (workflow run ID `44145429582`, commit `3c53b34a02d57dbd9505352e62a2c935989e00ec`) failed in **Build XNU 8796**, specifically while the pinned helper rebuilt `libfirehose_kernel`. The workflow did successfully invalidate the cached archive and begin rebuilding it; the prior hypothesis that a forced rebuild was enough was incomplete.
- The Xcode build inherited three architectures: `x86_64`, `arm64e`, and `arm64`. The injected `OTHER_CFLAGS` included `-march=armv8-a -mcpu=generic` for all slices. Clang rejected `-mcpu=` for the x86_64 target (`unsupported option '-mcpu=' for target 'x86_64-apple-macos15.0'`). It also failed the arm64e slice with `arm64e LR authentication requires ptrauth`. Exit code 65; no kernel verification or packaging ran.
- These errors are build-configuration errors, not evidence that the Firehose C source itself is broken. Lavender links a plain arm64 kernel, so building x86_64 and arm64e Firehose slices is unnecessary.

### Follow-up correction

- Commit `20a3bf61f33d215b2a306d99f26cd6fa76df81ae` changes the pinned helper patch so the Firehose target builds only `ARCHS="arm64" VALID_ARCHS="arm64"` and passes `-march=armv8-a` only to that ARM64 slice.
- The cache invalidation and FEAT_LSE disassembly guard remain in place. This is the next build candidate; it has not yet been reported green. Do not treat Run #345's uploaded raw kernel candidate as usable: the build step failed and the verification step was skipped.

### Next acceptance check

- Confirm a new run starts from commit `20a3bf61f33d215b2a306d99f26cd6fa76df81ae`.
- Confirm the Firehose compile command contains only the arm64 architecture and `-march=armv8-a`; then require archive creation, XNU link, Firehose no-LSE disassembly guard, full kernel verification and package verification before another RAM-only handset test.


## Run #346 failure and Firehose guard correction — 17 November 2026

- Run [#346 / 44146858623](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44146858623), source commit `20a3bf61f33d215b2a306d99f26cd6fa76df81ae`, failed in the `Fetch pinned XNU build tooling` step before any XNU compilation began. The RAM-only packaging job was skipped.
- The Arm64-only Firehose target rewrite itself was applied and printed by the job. The actual failure was the next validation command: `grep -c 'VALID_ARCHS="x86_64 arm64e arm64"' dxb/build.sh`. Because the old multi-architecture string was correctly absent, grep printed `0` but returned exit status 1, aborting the shell under GitHub Actions' error handling. This was an inverted/stale CI guard, not a compiler, kernel, or device failure.
- Corrected in commit `275514ed74ffc572e9eb854afc2f6779238259b4`: the workflow now explicitly fails only if the legacy architecture setting is found, emits a PASS message when the Arm64-only configuration is retained, and then continues to the XNU build.
- Run #346 produced no verified kernel or `iosboot.img`; its packaging job was skipped. Do not download or test any candidate from that run.
- Next acceptance gates remain: complete XNU build; Firehose symbol disassembly contains no FEAT_LSE/LSE instructions; kernel verification passes; RAM-only `iosboot.img` is packaged and uploaded; device testing only via `fastboot boot iosboot.img`. Do not flash or write internal storage. Keep `cpus=1` and do not change GIC routing without new panic evidence.


## Run #347: Firehose LSE instruction survives armv8-a build flag — 18 November 2026

- Run [#347 / 44147171820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44147171820), source commit `275514ed74ffc572e9eb854afc2f6779238259b4`, passed the tooling setup, Lavender profile/ADT validation, dependency setup, XNU build, and the general Mach-O/kernel identity checks. It then failed in the focused final-kernel ISA guard, so the RAM-only image packaging job was skipped.
- The linked kernel's `___firehose_buffer_tracepoint_flush` still contains `LDADD`. The independent `_lck_grp_reference` check passes with LDXR/STXR, so this is isolated to the Firehose/libdispatch path rather than a global kernel ISA failure.
- The Firehose Xcode compile command in the log did include `-march=armv8-a` on `firehose_buffer.c`, yet the final linked routine still decoded as LSE. The current evidence does not establish why that one routine retained LSE; do not claim the root cause is solved. The upstream build helper emits an intermediate `liblibfirehose_kernel.a` and moves it to `libfirehose_kernel.a`, so cache invalidation is being hardened to remove both names as well as the object/symbol trees.
- Corrective experiment committed as `640e6d76bed7d1dcaeb9c68c8e4879edc74dfa64`: append explicit Clang target-feature disable `-Xclang -target-feature -Xclang -lse` and `-mno-outline-atomics` to the arm64-only Firehose compile, remove both archive names before rebuilding, and bump the exact cache key to v6. This is a hypothesis to test, not yet a verified fix.
- The raw kernel candidate artifact from #347 is not approved for device testing or distribution because the ISA guard failed. No `iosboot.img` was packaged. Continue to require the final linked Firehose symbol to contain no LSE atomics before permitting packaging. Device testing remains RAM-only via `fastboot boot iosboot.img`; never flash or write internal storage, keep `cpus=1`, and do not alter GIC routing without new evidence.


## Run #347 kernel ISA verification failure and Run #348 explicit LSE disable — 18 November 2026

- Run [#347 / 44147171820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44147171820), commit `275514ed74ffc572e9eb854afc2f6779238259b4`, passed the Firehose architecture guard, machine-profile validation, ADT checks, and completed XNU compilation. It produced a 16,658,072-byte ARM64 Mach-O kernel candidate, but the final linked-kernel ISA verification failed.
- Exact verifier result: `FEAT_LSE instructions remain in ___firehose_buffer_tracepoint_flush: ['ldadd']`. The symbol still contains `ldadd x8, x8, [x9]` at VM address `0xfffffe00072dca54`. The LSE check is valid; this candidate is not accepted and the RAM-only packaging job was skipped. Do not boot it.
- Run #347's Firehose compile command did include `-march=armv8-a`, but this did not prevent the LSE instruction from reaching the linked kernel. The source change to architecture-only `arm64` was therefore insufficient. Keep the disassembly check as a hard gate; do not weaken it.
- The next branch revision is commit `640e6d76bed7d1dcaeb9c68c8e4879edc74dfa64`, with Run [#348](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44224919672) in progress. Its Firehose target adds explicit Clang feature disabling (`-mno-outline-atomics -Xclang -target-feature -Xclang -lse`) and uses a v6 cache key. This is a testable remediation, not yet a verified fix. Wait for the final linked-kernel disassembly gate before accepting the output.
- Safety/acceptance remains unchanged: only use a verified RAM-only image with `fastboot boot iosboot.img`; never flash or write internal storage. Keep `cpus=1`; do not change GIC routing without new evidence.


## Run #347 failure — Firehose still emits FEAT_LSE; Run #348 mitigation — 18 November 2026

- Run [#347 / 44147171820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44147171820), commit `275514ed74ffc572e9eb854afc2f6779238259b4`, passed the tooling setup and completed the XNU link. Kernel verification confirmed an ARM64 Mach-O of 16,658,072 bytes, but the focused ISA guard failed: `FEAT_LSE instructions remain in ___firehose_buffer_tracepoint_flush: ['ldadd']`. The separate `_lck_grp_reference` check passed. Packaging was skipped. This proves the Armv8-A flag alone did not eliminate the offending LSE instruction; no image from #347 is approved.
- The exact Firehose faulting routine still contains `LDADD`, so do not weaken/remove the verifier or package this candidate. The next corrective attempt is Run [#348 / 44224919672](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44224919672), commit `640e6d76bed7d1dcaeb9c68c8e4879edc74dfa64`. Its Firehose arm64 target explicitly adds `-mno-outline-atomics -Xclang -target-feature -Xclang -lse` in addition to `-march=armv8-a`, to disable LSE and outlined atomic helper generation.
- At the time of this update, Run #348 is still running in `Build XNU 8796`; the Firehose no-LSE verification has not yet been observed. This is a mitigation attempt, not a confirmed fix. If the guard still finds LDADD, next step is inspect the actual Firehose compile command and linked archive provenance, then rebuild the Firehose implementation/source with a supported non-LSE atomic path. Do not alter GIC code or bypass the guard.
- Keep `cpus=1`; only accept a new image after the full ISA guard and package/manifest/hash checks pass. Physical tests remain RAM-only via `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery.


## Run #348 verified XNU build; missing package artifact fixed in #349 — 18 November 2026

- Run [#348 / 44224919672](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44224919672), commit `640e6d76bed7d1dcaeb9c68c8e4879edc74dfa64`, completed both jobs successfully. The XNU build job completed the final-kernel ISA verifier, including explicit PASS output that `_lck_grp_reference` uses LDXR/STXR and `___firehose_buffer_tracepoint_flush` contains no FEAT_LSE atomics. This verifies that the prior LDADD panic source was removed from the linked kernel candidate.
- The package job also built `iosboot.img` and unpacked the resulting boot image. The runner log reports a 16,617,472-byte image. However, inspection of the complete job step list and workflow source found that the package job had no artifact-upload step. The final verification here-document was also truncated at `import re`, so the job's green conclusion did not mean payload identity, checksum, or manifest verification had run. No downloadable image artifact was published by #348; do not treat its ephemeral runner file as available.
- Fixed in commit `d5c8979a8d660d06fe75ef2dc769a6b875c520ac`: completed the verification script; compare the unpacked boot kernel, payload/ramdisk, and DTB byte-for-byte with their exact build inputs; write `BOOT-IMAGE-VERIFICATION.txt`, `BUILD-MANIFEST.txt`, and `SHA256SUMS`; verify the checksum; and upload `iosboot.img` plus those records as the `lavender-fastboot-boot-test` artifact (30-day retention).
- Run [#349 / 44228087165](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44228087165) was triggered by the artifact-publication fix and is the acceptance run. It must finish the full XNU build/ISA gate and package job; the package job must show the new verification and upload steps as successful, and the artifact list must contain `lavender-fastboot-boot-test`. Then download the artifact and independently compare the image size and SHA-256 with the manifest before sharing it.
- Until #349 produces and verifies the artifact, there is no approved downloadable replacement image. Do not reuse the old Run #3 image: it panicked on the FEAT_LSE LDADD instruction. Keep `cpus=1`; physical testing only via RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition, or write Android eMMC/userdata; preserve volume-key recovery.


## Run #349 — published and independently verified iosboot.img — 18 November 2026

### CI result

- Run [#349 / 44228087165](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44228087165) completed with conclusion `success` on commit `d5c8979a8d660d06fe75ef2dc769a6b875c520ac`.
- Both jobs completed successfully: `build` (ID `134546108566`) and `Package RAM-only Lavender fastboot image` (ID `134552741860`). The package job's steps for extracting the exact stock boot image/DTB, generating and validating the Lavender ADT, building the payload, constructing the non-flashing fastboot image, verifying image structure/payload identity, and uploading the verified image all completed successfully.
- The package log explicitly reports: `PASS: boot image kernel, payload, and DTB exactly match their verified inputs`; `iosboot.img: OK` from `sha256sum -c SHA256SUMS`; and successful `actions/upload-artifact@v4` upload with name `lavender-fastboot-boot-test`.
- The build job's `Verify Lavender kernel artifact` step also completed successfully. This is the build candidate after the FEAT_LSE Firehose mitigation and guard. The earlier Run #348 record documents the linked-kernel no-LSE guard passing; Run #349 rebuilt from that source revision and its kernel verification step passed again.

### Published artifact and independent verification

- GitHub artifact: `lavender-fastboot-boot-test`, artifact ID `14395081384`, GitHub compressed artifact size `5,839,371 bytes`, created 2026-11-18 03:24 UTC, expiry 2026-12-18 03:24 UTC. The compressed ZIP contains `iosboot.img`, `BUILD-MANIFEST.txt`, `SHA256SUMS`, and `artifacts/BOOT-IMAGE-VERIFICATION.txt`.
- Downloaded artifact independently and extracted `iosboot.img`. Independent `stat` result: **16,617,472 bytes** (15.84 MiB). Independent `sha256sum`: `76ee03dc1b97fc3786344d543921a03758b2abefcd70f4d52c8ed35898aee38f`. Both exactly match `BUILD-MANIFEST.txt` and `SHA256SUMS`.
- The downloaded file is recognized as an Android boot image with kernel at offset `0x8000`, ramdisk at `0x1000000`, and 4096-byte page size.
- Manifest provenance: target `lavender`, Qualcomm Snapdragon 660 / SDM660, DEVELOPMENT kernel, source commit `d5c8979a8d660d06fe75ef2dc769a6b875c520ac`, workflow run `44228087165`. Kernel SHA-256: `5f02538d1a6133afc60287835b739b0217b4fb41f9fc8035aa92d5d589101e4f`; payload SHA-256: `42ddf23add0ba1fc3a7863283fdb4c1669dd3285466e74d4b0af2085a8838952`; DTB SHA-256: `bcb3848e080bcf058f0b32ebf2ec440b6f638fed144c182766aa80a45bb9bb22`.
- Bootargs retain `cpus=1`; manifest mode is explicitly `RAM-only fastboot boot; no flashing`.

### Disposition

- **Artifact publication and byte-level integrity: PASS.** This is the first accepted downloadable image from the repaired #348/#349 pipeline. The earlier 5.8 MB figure refers to compressed ZIP size, not the uncompressed boot image.
- **Physical Darwin boot: NOT YET VERIFIED.** The FEAT_LSE static kernel guard and build/package checks are necessary but do not prove the phone boots through the kernel. Do not describe this as a fully booting iOS port.
- Download the artifact `lavender-fastboot-boot-test` from Run #349, or use the verified extracted `iosboot.img` provided with this handoff. The physical test, if performed, must be RAM-only: `fastboot boot iosboot.img`. Never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery and keep `cpus=1`.

## Physical test after Run #349 — kernel initialization boundary — 18 November 2026

- User-supplied recording `1000317986.mp4` is approximately 106.2 seconds. It shows the Lavender shim handoff screen, followed by the verbose XNU framebuffer console and AppleLogo. The footage is user-attributed to the latest Run #349 test but is **not cryptographically bound** to the Run #349 image SHA-256; do not claim exact-image validation from the video alone.
- The shim screen shows `CPU16K unsupported; trying A73`, followed by `A73 16K supported; launching XNU`. The display then changes from the red shim diagnostics to the XNU framebuffer console.
- The visible POST-PLATFORM trace progresses through event 106. This recording therefore shows later progress than the earlier event-102 affinity-routing panic documented above. No readable new GIC panic appears in this clip; do not infer a new GIC fault or change GIC configuration without fresh panic evidence.
- The console visibly reports the following completed KBOOT checkpoints: `KBOOT ENTER`, `KBOOT VM_BOOTSTRAP DONE`, `KBOOT OSLOG INIT DONE`, `KBOOT SCHED_INIT DONE`, `KBOOT MACH_IPC INIT DONE`, `KBOOT KERNEL_THREAD_CREATE DONE`, `KBOOT LOAD_CONTEXT ENTER`, `KBOOT THREAD ENTER`, `IDLE THREAD CREATE BEGIN`, `IDLE THREAD CREATE DONE`, `KBOOT SCHED BEGIN`, and `KBOOT SCHED DONE`.
- The last readable checkpoint is `KBOOT IOKIT BEGIN`. The display remains visually unchanged from approximately 20 seconds through approximately 100 seconds. It goes dark near the end of the recording (~105 seconds); the video alone does not establish whether this was a reset, power-off, or recording cutoff. No readable panic/exception/reset reason is captured.
- **Current physical boundary:** scheduler startup has visibly completed and `PE_init_iokit()` has been entered (event 19); the matching return marker `KBOOT IOKIT DONE` (event 20) is absent. The next investigation is inside the IOKit initialization path and its return boundary. Do not speculate about a particular IOKit component or fault from the static screen alone.
- **Run #349 source correlation:** inspected `xnu/osfmk/kern/startup.c` and `xnu/osfmk/arm64/lavender_early.c` from the Run #349 buildtree artifact. The high-resolution frame at ~15 seconds reads **`KBOOT IOKIT BEGIN`**, not “KBOOT INIT BEGIN”: it is trace event 19. In `startup.c`, event 18 is emitted immediately after `sched_startup()`; event 19 is emitted immediately before `PE_init_iokit()`; event 20 (`KBOOT IOKIT DONE`) is emitted immediately after that call returns. The recording shows event 18 and event 19, but no event 20. Therefore the next bounded investigation target is **inside `PE_init_iokit()` or its return path**, not the earlier scheduler-to-IOKit interval. This does not identify a particular IOKit component or prove a specific fault. Add narrowly scoped before/after checkpoints around the IOKit initialization path, then subdivide only if the new evidence requires it.
- This is not evidence of BSD init, VFS/root mount, launchd, SpringBoard, or a complete iOS userspace boot. The displayed AppleLogo and kernel console are diagnostic milestones only.
- Run #349 artifact verification remains independently confirmed: artifact `14395081384`, `iosboot.img` size 16,617,472 bytes, SHA-256 `76ee03dc1b97fc3786344d543921a03758b2abefcd70f4d52c8ed35898aee38f`; manifest and `SHA256SUMS` match the independently downloaded image. Bootargs retain `cpus=1`; mode is RAM-only fastboot boot.
- Safety unchanged: only `fastboot boot iosboot.img`; never flash, erase, repartition or write Android eMMC/userdata. Preserve volume-key recovery. No GIC changes without new, explicit panic evidence.


## IOKit boundary investigation and patch 0126 — 18 November 2026

### Exact-source findings

- Retrieved and inspected the XNU buildtree artifact from Run #349 rather than relying on an upstream-source guess. `PE_init_iokit()` performs console setup, boot-version output, optional `/chosen/memory-map` reads, `pe_prepare_images()`, optional progress setup and optional iBoot timing reads before calling `InitIOKit()` and `ConfigureIOKit()`.
- `InitIOKit()` runs `IOLibInit()`, `OSlibkernInit()`, `IOMachPortInitialize()`, allocates the global IOKit symbols/sets, initializes interrupt accounting, allocates and initializes `IOPlatformExpertDevice`, attaches the root nub, and optionally records startup extensions. `ConfigureIOKit()` asserts the root nub and calls `configureDefaults()`.
- Found a separate deterministic platform-matching defect in the same exact buildtree: the Lavender personality in `iokit/KernelConfigTables.cpp` used `IONameMatch = qcom,sdm660-mtp`, but `tools/mkadt-lavender.py` builds the root `compatible` property from `D22AP`, `iPhone10,3`, `xiaomi,lavender`, `qcom,sdm660`, and `AppleARM`. The actual root therefore cannot match the MTP-only string. The fallback `IOPanicPlatform` explicitly panics when no real platform expert matches.
- This name mismatch is a confirmed IOKit configuration bug, but it is not proven to explain the recording's missing event 20: that recording does not expose a panic or exact failure point inside `PE_init_iokit()`. Keep those two findings distinct.

### Patch 0126 candidate

- `xnu/patches/0126-lavender-iokit-bringup.patch` changes the Lavender platform expert match to the generated root compatible `xiaomi,lavender`.
- It adds checkpoints 107–125: PE initialization milestones, entry/exit around `InitIOKit()`, completion of IOLib / OSlibkern / Mach-port initialization, root-nub allocation/init/attach, entry to `ConfigureIOKit()`, and return from configuration.
- `tools/verify-lavender-iokit.py` checks the actual post-patch XNU tree for the platform-expert match and checkpoint ordering. The workflow requires patch 0126 and runs this verifier. The ADT host test also checks that the patched match is represented by the generated root-compatible string.
- Local validation against the exact Run #349 buildtree: patch dry-run succeeded; patch application succeeded; `verify-lavender-iokit.py` passed. This is source-level validation only, not an XNU compile or physical boot result.

### Acceptance

- Require the new XNU build and linked-kernel verification to pass, then require the separate RAM-only image package and manifest/SHA-256 checks.
- The next physical recording should show which of events 107–125 is last. Only a new successful physical run through event 20 proves that the prior IOKit boundary was crossed.
- Keep `cpus=1`; no GIC changes; physical tests remain RAM-only via `fastboot boot iosboot.img`. Never flash, erase, repartition, or write Android eMMC/userdata. Preserve volume-key recovery.


## Run #355 kernel-verifier rejection and correction — 18 November 2026

- Run #355 (44264884787, commit 28aed7d5165e4b121436053c827c830151da8dcd) passed profile validation, Lavender ADT validation, XNU compilation, the cache-aware build check, the Lavender entry guard, the Group-1 virtual-timer verifier, and the IOKit platform/trace verifier.
- The linked ARM64 Mach-O candidate was produced at 16,674,456 bytes and uploaded as artifact xnu-kernel-candidate (artifact ID 14410398528). The build log shows file identifying it as “Mach-O 64-bit executable arm64”.
- The run failed in tools/verify-lavender-kernel.py with the exact message “missing Lavender identity strings: qcom,sdm660-mtp”. Consequently, the kernel-verification step failed, the cache-save and final packaging upload were skipped, and the separate RAM-only package job was skipped. Run #355 did not produce iosboot.img.
- Inspected the exact Run #355 kernel candidate: it contains LavenderPlatformExpert, xiaomi,lavender, and Redmi Note 7; it does not contain qcom,sdm660-mtp. The compiled kernel’s ARM64 text contains the exact stage-8 and stage-9 publication instruction sequences required by the verifier (stage 8 at file offset 0x48b8c0, stage 9 at 0x48b94c). This independent inspection isolates the observed rejection to the stale identity expectation; it does not substitute for a successful rerun of the complete verifier.
- The mainline Lavender DTS declares compatible = "xiaomi,lavender", "qcom,sdm660"; (see upstream sdm660-xiaomi-lavender.dts). The handset match in xnu/patches/0126-lavender-iokit-bringup.patch is correctly xiaomi,lavender. The verifier was still requiring the unrelated reference-board string qcom,sdm660-mtp.
- Corrected tools/verify-lavender-kernel.py in commit bb32ac57b972e3adb29866d6e86e05ba57124476: expected identity is now LavenderPlatformExpert, xiaomi,lavender, and Redmi Note 7. No machine-profile, timer, GIC, CPU-count, or startup behavior was changed in this correction.
- Acceptance remains pending until the new CI run passes the full linked-kernel verifier, completes the RAM-only image packaging job, and uploads a verified iosboot.img with matching manifest and SHA-256. A successful compile or raw kernel candidate alone is not a bootable image.


## Physical footage review — root-nub allocation boundary and source-verified follow-up (18 November 2026)

### Source material and attribution

- Reviewed `VID-20261118-WA0004.mp4` (98.345 seconds) and `VID-20261118-WA0002.mp4` (5.775 seconds), supplied on 18 November 2026. Gemini's independent comparison reports that they show the same handset boot session, based on matching pointers, screen artifact, and continuous audio.
- The final visible token in checkpoint [0084] is **NUB**, giving the exact marker `IOKIT ROOT NUB ALLOCATED`; the earlier `ROOT MAP` transcription was incorrect.
- The recording is not checksum-bound to Run #357. Treat it as an unbound physical observation until the exact tested `iosboot.img` is identified. Do not infer a source revision from the marker text alone.

### Source-correlated meaning

The actual XNU buildtree artifact from Run #357 (artifact `14414095564`, source commit `fc9da9a76444fafc2a2a2d40bd1fd71c2104a1ad`) establishes the following sequence:

- `IOStartIOKit.cpp`: event 120 (`IOKIT ROOT NUB ALLOCATED`) is emitted after `gRootNub = new IOPlatformExpertDevice` and the null check.
- The very next operation is `gRootNub->init(dtTop)`. If it returns false, the code calls `panic("Failed to initialize IOKit root nub")`. Event 121 (`IOKIT ROOT NUB INIT DONE`) appears only after successful return.
- Event 122 follows `gRootNub->attach(NULL)`; event 123 marks `InitIOKit()` completion; event 124 marks `ConfigureIOKit()` entry; event 125 is emitted by `PE_init_iokit()` after `ConfigureIOKit()` returns.
- `IOPlatformExpertDevice::init(dtRoot)` in `IOPlatformExpert.cpp` calls `IODeviceTreeAlloc(dtRoot)` when `dtRoot` is non-null, then calls `super::init(dt, gIODTPlane)` if allocation returned a tree. If no tree is available, it calls fallback `super::init()`. These are the operations between events 120 and 121.
- Workloop creation and `registerService()` are not in this init function. They occur later in `startIOServiceMatching()`, invoked through `PE_lockdown_iokit()` / `StartIOKitMatching()` after machine lockdown. The `initWithArgs()` / `registerService()` theory in the Gemini response does not match this exact source tree.

### Console and failure visibility

- Run #357 bootargs include `lavender_no_fbcon=1`. In the exact buildtree this argument gates only `fbcon_probe()` in `pe_serial.c`. The [DARWIN] early checkpoint renderer writes directly to the framebuffer and is independent of that argument. A visible fixed checkpoint overlay is therefore not, by itself, evidence against the Run #357 bootarg; it still does not establish image provenance.
- Panic output is already routed through `panic_trap_to_debugger()` to `sweet_early_panic()`, which resolves to Lavender's `lavender_early_panic()`. ARM64 synchronous exceptions already emit event 40 with ESR/FAR/ELR/SPSR. Do not add redundant panic markers without showing that these existing paths fail.
- The uploaded video transcription includes labels such as `PE_INIT_IOKIT DONE`, `PE ROOT TIMER DONE`, `IOKIT SYMBOLS DONE`, and `IOKIT GROUPS DONE`. Those exact strings do not match the current Run #357 renderer's event 107–125 map. This is another reason not to treat the recording as a matched Run #357 trace.

### New targeted instrumentation — patch 0127

Patch `xnu/patches/0127-lavender-root-nub-init-trace.patch` adds seven source checkpoints in `IOPlatformExpertDevice::init()`:

| Event | Label | Meaning |
|---:|---|---|
| 126 | `IOKIT ROOT NUB INIT ENTER dtRoot=` | Entered root-nub init; records DT root pointer |
| 127 | `IOKIT DEVICE TREE ALLOC BEGIN dtRoot=` | About to call `IODeviceTreeAlloc()` |
| 128 | `IOKIT DEVICE TREE ALLOC RETURNED dt=` | Allocator returned; records returned tree pointer |
| 129 | `IOKIT ROOT REGISTRY INIT BEGIN dt=` | About to call `super::init(dt, gIODTPlane)` |
| 130 | `IOKIT ROOT REGISTRY INIT RETURNED ok=` | That superclass init returned; records boolean |
| 131 | `IOKIT ROOT REGISTRY FALLBACK BEGIN` | Device-tree path was unavailable; fallback init begins |
| 132 | `IOKIT ROOT REGISTRY FALLBACK RETURNED ok=` | Fallback superclass init returned; records boolean |

Interpretation for the next physical trace:
- Last marker 126: the function was entered; investigate the branch condition immediately after it.
- Last marker 127: execution did not return from `IODeviceTreeAlloc()` (or output stopped there); inspect allocator internals and exception/panic evidence.
- Event 128 with a zero pointer: allocator returned null; the code should take fallback init (131/132).
- Last marker 129: execution did not return from the registry superclass init (or output stopped there).
- Event 130/132 with `ok=0`: the init function returns false and the caller should panic; the existing panic renderer should report it.
- Event 130/132 with `ok=1`: successful superclass init; the next caller-side checkpoint should be event 121.

The splitter was made cache-safe: cached generated `lavender_early.c` containing events 107–125 receives 126–132 exactly once, while a fresh split receives both groups. The IOKit verifier now checks the source brackets, renderer labels, and that workloop/service matching remain outside `IOPlatformExpertDevice::init()`.

### Validation and acceptance

- Patch 0127 dry-run and application succeeded against the extracted Run #357 `xnu-buildtree` artifact. Local structural checks passed for the seven new markers and confirmed that the panic/synchronous-exception hooks already exist.
- The updated XNU workflow requires patch 0127. **No new CI build, linked-kernel verification, or packaged image has passed yet.** Run #357 remains the latest accepted image.
- On the next accepted candidate, preserve `cpus=1` and test only with RAM-only `fastboot boot iosboot.img`. Never flash, erase, repartition, or write Android eMMC/userdata. Preserve volume-key recovery. Do not claim BSD init, root mount, launchd, SpringBoard, or complete Darwin userspace without direct evidence.


## Documentation and verifier follow-up — 18 November 2026

- Replaced the inherited SWEET/Redmi Note 10 Pro README with a Lavender-specific project guide. It now states the target, current Run #357 accepted-image identity, the unbound older footage caveat, the exact root-nub diagnostic boundary, the separate XNU/package workflows, repository map, known non-milestones, and RAM-only physical-test rules.
- Updated `PROJECT_HANDOFF.md` current status to Run #357 and patch 0127. Older run records remain as historical notes; the previous `userdata` root-image plan is explicitly superseded and is not authorization to write the device.
- Updated Chapter 01 to make current authorization unambiguous: RAM-only `fastboot boot iosboot.img`; no flash, erase, format, repartition, or write to eMMC/`userdata` or any other partition. Any future storage-write proposal requires separate explicit approval.
- Corrected the case-block delimiter regex in `tools/verify-lavender-iokit.py` (commit `ebfbf9e83e3074a7e705c74ac6f5fc9407e38cb6`). A local check against the real Run #357 source layout confirms the 0127 patch dry-runs and applies cleanly, the seven trace points bracket the intended branches, and the corrected regex isolates a trace case correctly. The full workflow verifier, XNU compile, linked-kernel verification and package have not yet passed for patch 0127.
- Documentation changes do not themselves constitute a build or physical test. Run #357 remains the latest accepted packaged image until a later complete run passes all gates.
