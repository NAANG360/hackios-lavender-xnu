# LIOS live continuation checkpoint — 22 November 2026

**Re-verify the live branch and Actions before every new change.** This is an observed snapshot, not a substitute for a fresh GitHub query.

- Repository: `NAANG360/hackios-lavender-xnu`
- Branch: `ci/lavender-machine-profile`
- Verified HEAD at checkpoint: `6c52daeb43a9663feb133adf076e0596b2324d10`
- Latest source repair: [`6c52dae`](https://github.com/NAANG360/hackios-lavender-xnu/commit/6c52daeb43a9663feb133adf076e0596b2324d10) — `fix(lavender): publish RPM nub through IOKit iterator`
- Latest accepted kernel/image remains **Run #400**, source `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`, artifact `14615937969`, `iosboot.img` 16,621,568 bytes, SHA-256 `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`. This is a QUP-only source/build milestone; it is not an accepted RPM-enabled build.
- Latest observed XNU workflow is **Run #470** (`45018828868`), source `6c52daeb43a9663feb133adf076e0596b2324d10`, currently in progress. QUP and RPM/GLINK host protocol regressions passed; patch application and staged-source verifier passed. XNU 8796 compilation is still running; no final link or artifact is yet confirmed.
- Repair commit `6c52dae` changes `xnu/patches/0130-lavender-rpm-glink-v0.patch` and `tools/verify-lavender-rpm-glink.py`: it creates an `OSCollectionIterator` over the one-entry array, passes the iterator to `createNubs()`, and releases the temporary array. The verifier enforces this shape.
- Apple's XNU 8796.141.3 headers confirm `IODTPlatformExpert::createNubs(IOService *, OSIterator *)`; `OSCollectionIterator::withCollection(const OSCollection *)` returns an iterator-compatible object in this C++14 build configuration.
- All four patch hunk counts on `6c52dae` were independently checked and match their bodies. This is patch-structure validation, not XNU compilation.
- **Verified on `6c52dae` in Run #470:** QUP/RPM host suites, patch application and staged-source verifier passed. XNU 8796 compilation remains in progress. **Not yet verified:** final link, pre-link object audit, Mach-O verification, RAM-only image packaging, manifest/checksum, or physical operation.
- No physical RPM OPEN, OPEN_ACK, RPM-SMD ACK, LOW_SVS voltage transition, QUP transaction, eMMC read, touchscreen input, or display operation is established.
- Preserve Run #400 unchanged as the accepted baseline. Do not test an image unless it is built from the exact current source, independently verified, and packaged with matching manifest and SHA-256.

## Active route

1. Recheck live branch HEAD and newest Actions runs.
2. Run the QUP and RPM host suites on the exact current checkout.
3. Apply patch 0130 to the exact post-0129 XNU staged tree and run the staged-source verifier.
4. Confirm the injected non-LTO and PRE_LTO checks inspect the exact final-link file lists and strong symbol immediately before linking.
5. Trigger one XNU build only after the preceding gates pass. Require final ARM64 link, independent Mach-O verification, RAM-only package and matching manifest/SHA-256.
6. Only after package acceptance consider physical RAM-only boot, `fastboot boot iosboot.img`. Capture actual GLINK/RPM/QUP evidence; do not flash or write partitions.

## Hard safety invariants

- Confirmed Android 15/Lucid OS remains strictly read-only.
- Never flash, erase, format, repartition, or write any Android partition.
- Physical testing is RAM-only via `fastboot boot iosboot.img`; preserve `cpus=1` and the independent volume-key recovery path.
- Do not add weak vote-symbol stubs, fake ACKs, bypass the acknowledged `vdd_dig` vote, or write PMIC registers directly.
- Do not integrate the Novatek client before a real acknowledged RPM vote and successful physical I2C transaction.
- Do not start display/panel bring-up during this RPM/QUP milestone.
- Never equate compilation, image packaging, framebuffer output, or an IOKit nub with a functioning OS or Darwin userspace.

Detailed record: `docs/handoff/05-native-lavender-iokit-drivers.md`.
Validation ledger: `docs/handoff/06-validation-and-physical-bringup.md`.

---

# Lavender XNU — live project handoff

**Resume command:** “Continue from latest operations.”

Read `AGENTS.md`, inspect the live branch HEAD and newest Actions runs, then load only the routed chapter `docs/handoff/03-current-debugging.md`.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu

## Current milestone — 18 November 2026

- **Active branch:** `ci/lavender-machine-profile`; resolve the live branch HEAD before further engineering. Documentation-only follow-ups are layered after the Run #358 tested source commit.
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


## DeviceTree allocator stall — 18 November 2026

- The clearest physical frame ends at IOKIT DEVICE TREE ALLOC BEGIN after ROOT NUB INIT ENTER, and the user reports the screen then remained static before the A53 Guardian displayed its last-marker line. This is evidence that the attempt reached event 127, not proof of the exact instruction where XNU stopped. The recording is not bound to a known artifact SHA-256.
- Run #358 ADT audit: 10,392-byte blob, parsed end matches blob end, 25 nodes, and a host-side reproduction of XNU's depth-first iterator reaches all nodes. One definite malformed property key was found on arm-io/i2c0/novatek-touchscreen: lavender,gpio-controller-compatible was truncated into the 32-byte ADT name field without an in-field NUL. The following length word lets a C-string reader encounter a NUL shortly after the field, so this is a real format defect but not proven as the sole hang cause.
- ADT generator now renames the key to lavender,gpio-compat, rejects property names over 31 ASCII bytes, and validates serialized keys. Regression tests cover the limit. Shim/ADT Run #53 passed on commit 040b55e2880f1b2f73de3454302f4196b374fd8b.
- Patch 0128 adds allocator events 133–146: entry, registry plane, symbols, globals, root conversion, stack, iterator, sampled per-child conversion, tree completion, root attachment, interrupt-map walk, and return. Child indices 18–24 are individually traced; earlier children are sampled every fourth node. The screenshot's suspicious Novatek node is child 20 in the reproduced traversal.
- Guardian telemetry now continues after stage 20. Old behavior left the shared cell at 0x80000114 and could therefore report stale early progress while XNU continued. Late markers preserve the high-bit handoff flag and encode event in bits 8–15; event 141 also puts the current child index in bits 0–7. Example: allocator event 141 at child 20 is 0x80018D14. Every event updates the Guardian even when generic console events are throttled.
- Generic platform events 47–106 now print every fourth event plus event 106, preserving framebuffer rows for allocator diagnostics. Throttled events still update the Guardian, and empty trace events do not consume a console row.
- The first splitter insertion attempt failed before compile in Run #361 due to event-label insertion order. The splitter was rewritten to insert 107–125, then 126–132, then 133–146 idempotently; later source verification checks the event ordering, labels, Guardian encoding, per-child payload, and console throttle.
- Patch 0128 passed local patch --dry-run against the clean Run #358 IODeviceTreeSupport.cpp. Current guarded XNU validation candidate at this handoff: Run #371, source commit 1e594ec5880a8cfb337de7a16980f5c31c6f38d9, queued at the time of update. Runs #362–#370 are earlier queued/in-progress validation runs from intermediate commits; only accept a candidate after complete source guards, XNU build, linked ARM64 Mach-O verification and RAM-only packaging pass.
- Run #358 (artifact 14445642030) remains the latest accepted diagnostic image (iosboot.img SHA-256 b6361d9d03147570fb0a5407413d1b25c623d77c20649bcba63c35a3426f508f). Do not physically test unaccepted intermediate candidates.
- Next: follow the latest guarded run to a verified iosboot.img. Then use only RAM-only fastboot boot, preserve cpus=1 and volume-key recovery, and capture the full A53 LAST MARK hex value. Never flash, erase, format, repartition, or write Android eMMC/userdata or any other partition.


## Run #376 failure and allocator renderer repair — 19 November 2026

- Run #376 (44449132010, commit 47461799297c69ab0bbd9779972c70d93b64e950) built XNU 8796 successfully but failed at tools/verify-lavender-iokit.py with FAIL: trace renderer case 138 is missing. The package job was skipped; there is no new accepted image from this run.
- Root cause: partial earlier splitter injections left the late event-146 sentinel present while event 138 (DT ROOT BUILD DONE entry=) was absent. The source verifier correctly caught the missing case. This was a generated renderer/injection defect, not a compiler failure.
- tools/split-lavender-early.py now enumerates each expected allocator case 133–146, inserts missing cases individually in numeric order before the next greater case, and no longer trusts event 146 as a completeness sentinel. A subsequent cleanup removed an unused local variable.
- Repair commits: 21b05e98275fb15671050aef1519399abc73f6de (ordered per-event repair) and b9d6fe5df2ba4f10d434256b1e799c3492bd765c (cleanup).
- Runs #377–#379 were started by intermediate repair commits. Run #379 (44452048435) was in progress at the time of this update. Treat these as validation attempts, not accepted artifacts. Follow the newest run on ci/lavender-machine-profile, inspect the exact IOKit verifier output, and continue fixing the generator if any event 133–146 is missing or out of order.
- Acceptance still requires all source guards, successful XNU 8796 build, linked ARM64 Mach-O checks, RAM-only package construction and image identity/SHA-256 verification. Do not flash or write any device partition; physical test must remain RAM-only with cpus=1 and volume-key recovery.



## Run #380 follow-up and allocator diagnostic hardening — 19 November 2026

- Inspected Run #380 (44452106191) job 135318301022 and its complete log. The actual failure is `NameError: name 're' is not defined` in `tools/split-lavender-early.py:174`, inside `ensure_post_platform_renderer()`. The splitter calls `re.match`/ `re.finditer` but imported only `Path`. This failure occurs before the build tree is produced; do not describe Run #380 as a fresh successful XNU compilation.
- Run #380's `xnu-kernel-candidate` artifact (ID 14495329884) was nevertheless uploaded because output collection used `if: always()`. The logs explicitly say no generated XNU tree exists and that build-tree packaging was skipped. The copied 16,674,456-byte kernel candidate can therefore be stale cache output and is NOT accepted. Workflow fix gates both kernel collection and raw-candidate upload on successful preceding steps.
- Generator fix: `187b1e7ec95108b47f476c11d64aee6bef3ff8be` adds the missing `import re`.
- Verifier hardening: `0f8aa0e1eb76c43f3279836ed69d5dfe3aca0a4f` adds explicit numeric-order validation for renderer cases 133–146 and checks that `lavender_early.c` is listed in `osfmk/conf/files.arm64`. (Intermediate commit `3dfc8febdea329cb9e036fcd9558aeaea047b904` contained an escaped-regex typo; corrected in 0f8aa0e.)
- Stale-output guard: `253f7ee4fee690376547525163e5206f1f41d033` changes candidate collection/upload from `always()` to success-only.
- Allocator granularity improvement: patch `0128-lavender-device-tree-allocator-trace.patch` now emits event 141 for EVERY child before `MakeReferenceTable`; the generated C renderer prints the event only every fourth child and for children >=18. Thus the Guardian receives every child substage without flooding the framebuffer. Changes: `fdfa89d8e29647877ed0c76bfc0287819e5381a7` (patch), `d2b60de97e96a6ad1cc8b7926af3f5345725e39f` (generator), `e9314d3577b91be7e57ef15b024ee98b54283648` and `040fd105e45f6c8bdf90838589ab856bd912ae60` (verifier and regex correction).
- Event 141 Guardian value is `0x80010000 | (141 << 8) | child_index`; child 20 is `0x80018D14`. The late trace store is followed by `dc cvac` and `dsb sy`. Guardian executes on the A53 outside the XNU MMU/cache path; the marker is the reserved boot-argument word at `boot_args + 0x1000`.
- Code head for this checkpoint: `040fd105e45f6c8bdf90838589ab856bd912ae60`. Runs #381–#388 are validation attempts for intermediate/final commits; at documentation time no run had completed the full guarded build/package sequence. No accepted `iosboot.img` SHA-256 exists for this checkpoint. Do not use any intermediate candidate for physical testing.
- Acceptance remains: generated `lavender_early.c` inspected; cases 133–146 present and numerically ordered; Guardian event/substage and cache visibility guards pass; patch 0128 changes the actual compiled `IODeviceTreeSupport.cpp` translation unit; XNU build and linked ARM64 checks pass; RAM-only `iosboot.img` packaged and hashed. Preserve `cpus=1`, volume-key recovery, and RAM-only `fastboot boot`; never flash/erase/format/repartition Android storage.

- Patch hunk metadata was recalculated after converting event 141 to an unconditional per-child Guardian publication; corrected patch commit: `040fd105e45f6c8bdf90838589ab856bd912ae60`. Run #389 is the first validation run containing the corrected patch offsets.
