# Lavender XNU — live project handoff

**Resume command:** “Continue from latest operations.”

Read `AGENTS.md`, inspect the live branch HEAD and newest Actions runs, then load only the routed chapter `docs/handoff/03-current-debugging.md`.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu

## Current milestone — 18 November 2026

- **Active branch:** `ci/lavender-machine-profile`; current branch HEAD after documentation refresh: `e113ba2f8f5a5815b98835ffdb90f902b97d6cd6`.
- **Latest accepted CI image:** Run [#358 / 44339488789](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44339488789), built from source commit `f1f633f9ac1887736f17937c38eb6c32de489160`. Both XNU build/verification and downstream RAM-only package jobs passed.
- **Image artifact:** `lavender-fastboot-boot-test`, artifact ID `14445642030`; `iosboot.img` size **16,617,472 bytes**, SHA-256 **`b6361d9d03147570fb0a5407413d1b25c623d77c20649bcba63c35a3426f508f`**. The downloaded artifact was extracted locally; `sha256sum -c SHA256SUMS` passed and the hash matches `BUILD-MANIFEST.txt`. Artifact ZIP digest: `723a50d977358f356febe994429fcaf74cfcf85d0c652e6faa3fdf8348ef3eb5`.
- **Patch 0127:** full CI acceptance achieved. Root-nub trace events 126–132 passed patch/source guards, generated-label checks, IOKit verifier, XNU 8796 compilation, linked ARM64 Mach-O verification and image packaging.
- **Bootargs:** `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. Preserve `cpus=1`; the no-fbcon argument gates the ordinary fbcon path but not the independent direct-framebuffer checkpoint renderer.
- **Physical status:** Run #358 has not yet been tested/confirmed on the handset. Earlier videos are not checksum-bound to this image and must not be attributed to it. CI success is not physical boot success; no BSD init, root mount, launchd, SpringBoard or complete Darwin userspace is established.
- **Next action:** user should verify the exact image SHA-256 and test only with `fastboot boot iosboot.img`, then provide a fresh recording tied to that hash. Interpret events 126–132 using Chapter 03 before changing source.
- **Target:** Xiaomi Redmi Note 7 / Note 7S `lavender`, Qualcomm SDM660 / Kryo 260, ARM64. Kernel XNU 8796 (iOS 16 generation); longer-term userspace reference iPhone X iOS 16.7.16 (20H392).
- **Physical test constraints:** RAM-only `fastboot boot iosboot.img` only; preserve volume-key recovery. Never flash, erase, format, repartition, or write Android eMMC, `userdata`, or any other partition.

## Previous accepted operations — Run #300 (superseded by Run #302)

- **Run #300** ([43891112657](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43891112657), source commit `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`) completed successfully. Both the XNU build/verification job and the separate RAM-only packaging job passed.
- Build job: XNU 8796 compilation, expected cache-aware build-path guard, Lavender entry-guard patch check, and kernel artifact verification all passed. Package job: matching kernel retrieval, Mach-O presence, OTA/DTB extraction, Lavender ADT generation, payload construction, fastboot image creation, image-structure/payload-identity verification, and artifact upload all passed.
- Artifact **`14236569374`** (`lavender-fastboot-boot-test`) contains `iosboot.img`, **16,601,088 bytes**. Extracted image SHA-256: **`772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef`**.
- The artifact manifest identifies source commit `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`, genuine iPhone X iOS 16.7.16 (20H392) AppleLogo raster (194×238), target Redmi Note 7/lavender/SDM660, and fastboot-boot-only mode. The package is a verified CI image, not evidence of a complete Darwin userspace boot.
- Runs #298 (`43890854367`) and #299 (`43890920180`) failed at the required-patch guard after XNU compilation; their linked verification and packaging were skipped. Run #300 is the succeeding full build + verify + package acceptance run for the corrected source state.
- The user's clip `1000317779.mp4` shows the XNU handoff, genuine AppleLogo, and four lower-edge green markers consistent with simple stages 17–20. It is not checksum-bound to this Run #300 image. Do not claim the clip proves this exact image reached KPRINTF, BSD init, root mount, launchd, SpringBoard, or complete iOS userspace.
- Run #300's manifest is stale on recovery status. The user confirms the power-button-independent volume-key recovery/reset route works; the physical test recovery gate is cleared. The Guardian watchdog-first fallback remains separately unverified.
- **Next:** preserve this artifact as the newest accepted diagnostic image. Continue Chapter 03's post-KPRINTF trace investigation, then use Chapter 05 only when interpreting later BSD/root/userspace markers. RAM-only `fastboot boot iosboot.img`; never flash.

## Latest accepted operations — Run #297 (16 November 2026)

- **Run #297** ([43853989024](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43853989024), commit `fd40a4953708f22e7df492bef699e657dade93ea`) completed successfully. Both the XNU build/verification job and the separate RAM-only package job passed.
- Artifact `14219764264` (`lavender-fastboot-boot-test`) contains `iosboot.img`, 16,601,088 bytes. SHA-256: `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`.
- The package manifest confirms the genuine iPhone X iOS 16.7.16 (20H392) AppleLogo raster and early diagnostic markers. Patch 0118 adds stage 18 after `PE_consistent_debug_inherit()` and stage 19 after `serial_init()`; stage 20 remains the KPRINTF startup-complete marker.
- New user recording `1000317779.mp4` shows the shim's A73/16K fallback path, then the AppleLogo screen with four accumulated green lower-edge bars. Cross-checking the patch chain maps these to `sweet_early_mark` stages 17–20; this strongly indicates KPRINTF startup completion (stage 20). The clip is user-supplied physical evidence but is not checksum-bound to artifact `14219764264` or its `iosboot.img` SHA-256. It does not prove BSD/root mount, launchd, SpringBoard or complete iOS userspace.
- **Historical next task:** use Chapter 03 for marker attribution. The user has since confirmed the independent power-button-free volume-key recovery/reset route works, clearing the handset-test gate. RAM-only boot only; never flash.
- **Current route:** Chapter 03 for the next diagnostic boot; Chapter 05 for post-logo interpretation and Darwin userspace. Chapter 01 governs device safety.

## Physical milestone

The user-supplied 20.295-second video `1000317742.mp4` shows the Redmi Note 7/Lavender displaying the Apple-shaped diagnostic mark with the #261-attributed image. This is a user-attributed physical observation, not device-checksum-bound. The mark is patch 0094's hand-drawn approximation, not Apple's original AppleLogo. It confirms arrival at the framebuffer logo-rendering stage; it does not prove stage 20, root mount, launchd, SpringBoard or a complete iOS boot.

The user explicitly rejects the custom approximation and requires the actual Apple logo. The exact iPhone X iOS 16.7.16 (20H392) `applelogo@3x~iphone.im4p` was extracted and hash-pinned (IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`; decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`). Patch 0114 integrates that original raster at the same post-`arm_vm_init()` stage. It has not yet passed the full CI acceptance ladder or a physical test.

## Earlier build history (superseded by Run #297 below)

- **#261**: XNU build, verification and RAM-only packaging passed; user video shows the diagnostic logo-rendering stage.
- **#266**: full build/verification/package passed with the old diagnostic logo; artifact `14197706518`, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`; not separately physically tested.
- **#267**: failed during AppleLogo preparation because escaped shell variables reached `mktemp`; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` fixed expansion.
- **#268**: XNU compiled; AppleLogo header/hash/source guard, commpage, SHA2 and framebuffer checks passed. It failed later in the startup-trace source verifier with `ValueError: substring not found`: the verifier still used removed `lavender_logo_ellipse` as the end of the trace-helper slice. Linked Mach-O verification and packaging were skipped.
- **Verifier correction**: commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` bounds the trace-helper slice at the next function `sweet_early_logo(void)`. A local check against the #268 patched buildtree confirms the new boundary and trace-scope assertions.
- **#269** (`43809333900`, source `5dc07a9`) is running the corrected workflow. At last inspection, the XNU build step was still in progress; require linked verification and RAM-only packaging before accepting its image.

## Latest operations — 16 November 2026

- **Run #281** ([43819847178](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43819847178), source `94c2366567aee5df21435a7c4bb91041f161305e`) compiled XNU but failed the required-patch guard. Its log reports 0115 corrupt at EOF line 161 and 0116 at EOF line 46; linked verification and packaging were not reached.
- **Exact cause:** both patches had a trailing whitespace-only context line after the final hunk, and the final hunk headers each declared one extra old and new line. Correct bodies are 0115 `6/12` (previously `7/13`) and 0116 `28/21` (previously `29/22`).
- **Repair commits:** `ab90b5840b891ee2894e2606f8b0e45b83ef8bd4` and `857e735a046c76c9889ddac429583b4a24e03706` remove the stray EOF context lines; `f84b0c43b94c322bb300bf57b2fde7329fce8ca5` and `596bf0c9690e468ab188d0008e7e9495108c6b11` correct the final hunk counts.
- **Patch validation:** an independent count of all 11 hunks confirms each declared old/new count matches its actual body; no malformed lines remain.
- **Run #285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), source `596bf0c9690e468ab188d0008e7e9495108c6b11`) is the first run using both fully corrected patches. It has reached **Build XNU 8796**. Still require clean patch application, all source guards, linked Mach-O verification and successful RAM-only packaging before accepting `iosboot.img`.
- Runs #282–#284 were triggered by intermediate repair commits; #285 is the acceptance run.
- **Visual status:** the genuine AppleLogo itself was physically observed earlier. The post-logo 0115/0116 trace overlay is not yet accepted in a packaged artifact or physically verified; pre-logo console replay remains a separate gap.


## Historical storage/root planning — superseded; do not execute

The following older notes discussed a possible HFS+ root on the existing `userdata` partition. They are retained only as project history. They are **not current implementation instructions or authorization**. Current device authorization is RAM-only kernel testing; no partition writes are permitted.

- Any future SDHC1/eMMC discovery, GPT/HFS+ work, root mounting, or root-image plan must first be re-audited against current hardware evidence and documented in a new proposal.
- Do not create, flash, erase, format, repartition, or write an iOS root image to `userdata` or any other eMMC partition.
- Any future storage-write work requires a separate explicit user decision and a reviewed data-loss/recovery plan. Preserve Android user data and all partitions in the meantime.
- The target-specific SDHC1/eMMC transport, power sequencing, read-only discovery and root mount are not accepted as implemented.

## Safety

Use only RAM-only `fastboot boot iosboot.img` for current kernel bring-up. Never run `fastboot flash`, erase, format or repartition commands. Never write Android eMMC, `userdata`, boot, vendor_boot, recovery, modem, persist, or any other partition. Preserve `cpus=1` and the independent volume-key recovery route. A visible logo or checkpoint is not proof of Darwin userspace.

## Latest CI correction — Run #286 (16 November 2026)

- **#285** ([43822752674](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43822752674), commit `596bf0c9690e468ab188d0008e7e9495108c6b11`) compiled XNU but failed the required-patch guard because `0115-lavender-post-logo-darwin-trace.patch` was skipped. The build log gives the actual apply errors: `osfmk/kern/startup.c:744` and `bsd/kern/bsd_init.c:1044`. The earlier “only malformed hunk counts” diagnosis was incomplete; those hunks also used stale source context relative to the actual XNU tree after the preceding Lavender patches.
- **Corrective change:** regenerated patch 0115 against the exact post-0106…0114 source tree extracted from #285's `xnu-buildtree` artifact. The new unified diff includes the actual context around scheduler startup, IOKit, BSD bootstrap, VFS initialization, root mount, and `bsd_utaskbootstrap`. It was locally validated with `patch -p1 --dry-run` against that exact build tree: all three files checked successfully.
- **#286** ([43825403975](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43825403975)) is the fresh validation run for commit `b1b434dfc9d3668bbea9a4704a85a324ff0f9887`. It is in progress at the time of this update. Require 0115 and 0116 not to be skipped, then require kernel verification and RAM-only package completion. Do not distribute its candidate unless the full run is green.
- **Important correction to prior reports:** #281–#285 were not valid green builds for the new trace changes. The reliable previously packaged test image remains #274. The 0115 patch fix is now tested against the actual XNU source context rather than by hunk-count lint alone.


## Post-KPRINTF trace CI hardening — 16 November 2026

- Run #297 remains the latest accepted image: artifact `14219764264`, `iosboot.img` SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`. The physical clip `1000317779.mp4` strongly correlates with simple stages 17–20, but is not checksum-bound to this image; do not claim BSD/root/userspace.
- Added workflow source guards for the distinct `sweet_early_trace_startup` event namespace 17–29: ordered scheduler, IOKit, early-boot, interrupt, BSD-init, VFS, root-mount and utask-bootstrap boundaries; plus the transparent `[DARWIN]` console renderer and post-logo header ordering.
- Change commit: `9e7aeafa26a2512ee148bdf8cc18691e8e363eaa`. The guard was executed locally against the exact patched XNU source extracted from Run #297's `xnu-buildtree` artifact and passed. A fresh Actions run is the next acceptance gate; no new image is accepted until XNU build, linked Mach-O verification and RAM-only package all pass.
- The volume-key recovery/reset route is confirmed working by the user; the recovery gate is cleared. Never flash; RAM-only fastboot boot only.


## Latest continuation checkpoint — Run #300 forensic verification (16 November 2026)

- Revalidated the downloadable Run #300 `iosboot.img`: 16,601,088 bytes; SHA-256 `772acd07ebfccd6c58c0370cf38c85bb66b27fe4f0f9ccf9183bbea2c5815cef`, matching artifact `14236569374` from run `43891112657`.
- Inspected the exact Run #300 `xnu-buildtree` artifact `14238555047`. The compiled XNU tree contains the `sweet_early_trace_startup()` renderer and event instrumentation 17–29. The source sequence is scheduler 17–18, IOKit 19–20, early boot/interrupt enable 21–22, BSD init 23–24, VFS/root mount 25–27, and BSD utask bootstrap 28–29.
- The next diagnostic boundary is event 17, `KBOOT SCHED BEGIN`, emitted at entry to `kernel_bootstrap_thread()`; event 18 follows `sched_startup()`. This is the first post-KPRINTF split. The buildtree audit proves instrumentation was compiled, not that the phone reached those events.
- Run #300 has been superseded by accepted Run #302. No flash. The user confirms Chapter 01's power-button-independent volume-key recovery/reset route works; the recovery gate is cleared. The current recording is not checksum-bound to Run #300 and does not prove BSD/root/userspace. Its apparent final FASTBOOT frame is a video glitch; the phone restarts normally, with the reset cause still unknown.
- Current routed chapter: `docs/handoff/03-current-debugging.md`; consult Chapter 05 for later BSD/root/userspace interpretation.

## Latest continuation — Run #300 video and new bootstrap-entry trace (16 November 2026)

- New phone clip 1000317806.mp4 shows shim handoff output before AppleLogo, four green markers and only the small verbose-console header after AppleLogo; no readable [DARWIN] log scroll is visible. The apparent final FASTBOOT frame is a recording glitch; the phone restarts normally, but the video does not identify the reset cause.
- Source correction: event 17 is after idle_thread_create(processor), not at kernel_bootstrap_thread entry. Absence of event 17 cannot distinguish failure to schedule/enter the bootstrap thread from a stall inside current_processor or idle_thread_create, and the console-render path remains another possible visibility issue.
- Patch 0119 adds event 33 before load_context(thread), event 30 at bootstrap-thread entry, and events 31/32 around idle_thread_create. Existing events 17/18 continue to bracket sched_startup.
- Patch commit 56c427f8a506cdc6ba250c536810df95dfb01ae4; workflow guard commit 34a85dd88d13cc4b30e6ccf45a92aae9f685cce8. Runs #301/#302 were queued; #302 is preferred because it includes the required source guard.
- Chapter 01's recovery/reset safety gate has been cleared by the user's confirmation. RAM-only fastboot boot only; never flash.


## Latest engineering update — Run #302 and patch 0120 — 16 November 2026

- Run #302 (`43903857144`) passed both build and RAM-only packaging. Artifact `14245608682` contains `iosboot.img` (16,601,088 bytes), SHA-256 `2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98`. Patch 0119 is accepted.
- Patch 0120 (`c6e812c7cdc712f8d88384a72e32e11bf7871a6e`) adds direct early panic reporting, first kernel synchronous-exception ESR/FAR/ELR/SPSR output, real post-stage-20 `arm_init()` boundaries (34–39), and actual `kernel_bootstrap()` boundaries (41–46).
- Workflow guard commit `dc0603b6e6a64e117b04b447f82de37ecc36ed3f` requires 0120 and verifies its source ordering and panic/exception hooks. Run #303 (`43909137046`) and Run #304 (`43909349821`) were triggered before the patch-format fix and are not acceptance candidates. Commit `b457583a2e8b6d42406bf44d692c612225d0c88d` corrected the 0120 hunk count; guarded Run #305 (`43910208977`) is the preferred acceptance run.
- Corrected call graph: `PE_init_platform(TRUE, &BootCpuData)` is in `arm_init()` after stage 20 and before `machine_startup(args)`; it is not in `kernel_bootstrap()`. The latter progresses through `vm_mem_bootstrap()`, `STARTUP_SUB_OSLOG`, `sched_init()`, `STARTUP_SUB_MACH_IPC`, and kernel-thread creation. There is no `STARTUP_SUB_VM_KERNEL` boundary in this function.
- Do not blindly extend SWEET GIC Group-1 routing to Lavender: the Lavender machine profile explicitly rejects inheriting SWEET's interrupt-security/timer assumptions. Audit this with Lavender-specific hardware evidence.
- The user confirms the independent volume-key recovery/reset route works. The handset-test recovery gate is cleared. The Guardian watchdog-first fallback is separate and remains independently unverified.
- The apparent FASTBOOT end-frame in the latest recording is a video glitch; the phone restarts normally. The reset cause is unknown.
- Kernel tests remain RAM-only `fastboot boot iosboot.img`; never flash.

## Lavender early-source split checkpoint — 17 November 2026

- The user requested a dedicated Lavender early console implementation rather than continuing to keep Lavender code inside SWEET's source file.
- tools/split-lavender-early.py now separates the Lavender branch from the patched XNU source after the full patch stack: generated osfmk/arm64/lavender_early.c and lavender_early.h, SWEET-only sweet_early.c, compatibility aliases in sweet_early.h, and a files.arm64 entry for lavender_early.c.
- .github/workflows/xnu.yml invokes the splitter before compilation, invalidates the generated source in the build path, hashes the generator in the cache key, and checks the split, board routing, Lavender source symbols and source-list registration.
- Current branch HEAD at this checkpoint: 22590d814dc0770b2740d413b52600e0c32d622e (ci/lavender-machine-profile). Runs #307–#310 were triggered by incremental workflow commits; #310 is the intended latest guarded validation candidate. They are in progress at the time of this note; do not accept an image until one complete run passes XNU build, source guards, linked Mach-O verification and RAM-only package.
- Run #306 failed during XNU compilation; its raw kernel candidate is not accepted. Run #302 artifact 14245608682 remains the latest accepted diagnostic image, SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98.
- The source split preserves Lavender's verified framebuffer geometry and all current logo, guard, checkpoint, panic and trace behavior. It does not import Sweet's framebuffer or GIC/timer assumptions.
- Continue only with RAM-only fastboot boot iosboot.img. Never flash or write boot-critical partitions.


## Latest Lavender early-source split correction — Run #318 / Run #320 (17 November 2026)

- Run #318 (44016311664, source 9041148e40462d7eea7a32b9fd176f7ec29d0f28) compiled all translation units but failed at the final link of kernel.development.lavender.unstripped with undefined _sweet_early_trace_startup references.
- Cause: the splitter renamed the implementation to lavender_early_trace_startup, and sweet_early.h macro-redirects callers that include it. However, startup.c, lock_mtx.c, lock_group.c and other patched translation units have raw extern declarations and do not include the facade header, so their object files still reference the original ABI symbol.
- Run #318 also exposed a guaranteed Step 13 verifier failure after linking: the workflow still searched lavender_early.c for sweet_early_* names and the old single-line putc signature.
- Commit 0abe1270618c7a5d3a303dd1c2603e107c94bf26 fixes both issues. The splitter appends a sweet_early_trace_startup compatibility definition forwarding all six arguments to lavender_early_trace_startup, and ensures the wrapper on already-split/idempotent paths. The workflow's Step 13 anchors now use lavender_early_* names and lavender_early_putc(char c).
- The Python splitter parsed successfully and the workflow YAML parsed successfully before commit. The duplicate stage-marker assertion was removed. The caller source files were not rewritten; the ABI wrapper is the compatibility boundary.
- Run #320 (44020121980) is the validation run for the fix. At last inspection, setup, Xcode/tooling selection, machine profile, cache restore, runner capacity, dependency install and cache invalidation passed; Build XNU 8796 was in progress. Step 12/13, linked verification, artifact verification and RAM-only package are not yet confirmed.
- Run #302 artifact 14245608682 remains the latest accepted diagnostic image (16,601,088 bytes; SHA-256 2fa85c8381efe026a092a40275b6298c13a3eff473d9a5be5990627a0b4c0f98). Do not boot raw candidates from failed or incomplete runs.
- Next action: follow Run #320 through the source guards, linked Mach-O verifier and package job. RAM-only fastboot boot only; never flash.
