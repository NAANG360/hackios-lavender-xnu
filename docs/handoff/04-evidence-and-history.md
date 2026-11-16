# Latest evidence update — 16 November 2026

- Run #261 passed build/verify/package and the user video shows the handset at the diagnostic mark. Patch 0094's mark is a custom approximation, not Apple's original artwork; the video is not checksum-bound to the image.
- Run #266 (`43802934997`) is the latest accepted old-logo package: artifact `14197706518`, `iosboot.img` 16,551,936 bytes, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`. It was not separately physically tested.
- Extracted the actual AppleLogo IM4P from iPhone X iOS 16.7.16 (20H392): archive member `applelogo@3x~iphone.im4p`, IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`, decoded RGBA PNG 194×238 SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`.
- Patch 0114 and build-time extraction/header generation replace only the logo rendering body; the post-`arm_vm_init()` call placement is preserved.
- Run #267 failed because `prepare-lavender-applelogo.sh` passed a literal escaped `${TMPDIR:-/tmp}` to mktemp. Commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` removes erroneous backslashes; Run #268 validates this fix.
- Exact Build 4 FDT audit confirms SDHC2 removable host at `0x0c084000`, 4-bit, GPIO54 card-detect, IRQ125/221, PM660L L5/L2 rails and SDCC2 AHB/APPS clocks 93/94. SDHC1 at `0x0c0c4000` is internal eMMC and must remain untouched.
- Xiaomi SDM660 downstream Linux driver shows V5 power IRQ must sequence VMMC, VQMMC and pinctrl transitions for BUS_ON/OFF and IO_LOW/HIGH requests. This is a real part of the initial XNU host driver, not optional logging.
- The 16 GiB logical HFS+ root image cannot fit in the ~1.10 GiB remaining ADT RAMDisk tail; the payload builder now rejects oversized RAMDisks before reading.

This update supersedes the earlier unconfirmed public-DTS-only SDHC2 lead and Run #265/#266-in-progress notes.

---

## New video batch — 15 November 2026

Four physical recordings are now reviewed together. Keep them separate from Run #240: none shows the patch-0109 rolling trace labels (`TABLE`, `QSORT BEGIN/DONE`, `INIT BEGIN/DONE`, `HOOK idx=...`).

| File | Duration | Visible evidence | Provenance |
|---|---:|---|---|
| `1000317701.mp4` | 87.785 s | Earlier review: completed checkpoints 01–08 and final visible `[09] entering kernel_startup_bootstrap`; later screen dark/charging indication | Run #236-consistent by labels, not checksum-bound |
| `1000317713.mp4` | 27.531 s | SHIM/Lavender XNU red diagnostic panel; boot counters; white checkpoint list reaches `[09] entering kernel_startup_bootstrap`; remains on that display in early frames, later recording shows dark screen with a bright lower-edge line | Unknown build; no Run #240 trace |
| `1000317714.mp4` | 27.744 s | Same SHIM/Lavender XNU diagnostic panel and boot-counter layout at start; screen becomes dark shortly after; bright lower-edge line persists; later battery icon appears | Unknown build; likely same diagnostic generation as adjacent clips, but not established |
| `1000317717.mp4` | 54.303 s | Same red panel and checkpoint list; `[09] entering kernel_startup_bootstrap` remains the last visible completed checkpoint across the long middle section; near the end screen goes dark and battery icon appears | Unknown build; no Run #240 trace |

### Cross-video interpretation

- The new clips reinforce that the older checkpoint-based image reaches the *entry* to `kernel_startup_bootstrap()`; none visibly proves marker `0x10A` or a return from that function.
- `1000317717.mp4` is the clearest sustained view: checkpoint 09 remains unchanged for roughly the visible middle of the 54.3-second recording. This is consistent with a stall after entering startup bootstrap, but the clip does not expose the internal patch-0109 trace, so it cannot identify table validation, qsort, subsystem logging or a particular callback.
- `1000317713.mp4` and `1000317714.mp4` show the same boot-panel family but have insufficient provenance to assign a workflow/run. Treat as corroborating historical footage, not a new test of Run #240.
- The red-panel counters include `BOOT CPU16K unsupported; trying A73`, A73 entry/target/observed fields, PSCI CPU fields, and MMFR value ending `101122`. These are shim diagnostics, not proof of an XNU startup-hook failure. Preserve exact readings as visual evidence; do not infer a new CPU bug from them alone.
- The screen darkening and later battery icon are visible outcomes, but they do not establish a kernel panic, Guardian reset, or exact cause.
- No separate 64-second clip was present in the mounted batch. The 54.303-second `1000317717.mp4` must not be mislabeled as the 1:04 clip mentioned by the user. The previously supplied 87.785-second `1000317701.mp4` is retained as the older Run #236-consistent reference.

### Critical next step

The Run #240 image remains untested in the evidence available here. Do not treat these clips as its output. The next useful diagnostic evidence is a recording of the bottom-row patch-0109 trace from the exact Run #240 `iosboot.img`, but only after the independent recovery/reset safety gate is satisfied. No code change or additional build is justified by these videos alone.

# Chapter 4 — Evidence, provenance and historical traps

## Run #240 — startup-trace diagnostic candidate

- Run #240 (ID `43709690089`, source `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`) completed successfully. Both the XNU build and RAM-only image package jobs passed, including the required patch guard, Mach-O verifier and package structure checks.
- Patch `0109-lavender-startup-hook-trace.patch` compiled and is present in the packaged kernel.
- Fastboot artifact ID `14160128075`; exact `iosboot.img`, 16,531,456 bytes, SHA-256 `e21d513f42f288e2cbd1705111507c9ba9ab398c7366b397227efb6b9227922a`.
- Kernel artifact ID `14159918757`; 16,587,456 bytes, SHA-256 `cba785aa6ccf6916ca86fde8ff97b1b4f583e888b5b57b8ba159b1e03743e712`.
- Buildtree artifact ID `14160182439`; build log artifact ID `14160376884`.
- Runs #238 and #239 failed the evolving source-guard assertions only; Run #240 is the corrected passing candidate.
- Run #240 has **not** been physically tested. The latest phone evidence remains Run #236-consistent marker `0x109`.

## Run #240 linked-table audit

From the exact Run #240 kernel:
- `__BOOTDATA,__init_entry_set`: file offset `0xb1cd58`, size `0x17e20`, 97,824 bytes = 4,076 × 24-byte entries.
- Actual table extracted and fed through the XNU 8796 `bsd/kern/qsort.c` algorithm and `startup_entry_cmp`: 10,790 comparisons, recursion depth 7.
- Simulated sorted phases: TUNABLES 242, TIMEOUTS 18, LOCKS 614; 874 entries before the `STARTUP_SUB_LOCKS` cutoff.
- All 4,076 function pointers land in mapped executable segments; all non-null args land in mapped kernel segments; no sort ordering violations.
- Trace helper and startup/qsort functions are present in the linked symbol table.

This rules out a malformed/empty table and makes host-side qsort complexity unlikely. It is not a handset execution result and does not identify the stalled callback. Wait for the last visible trace line before making a functional change.

## New physical evidence — Run #236

- Video `1000317701.mp4` (87.8 seconds) shows the Lavender XNU console progressing through marker `0x109`, “entering kernel_startup_bootstrap”.
- Checkpoints 1–8 are visibly completed, including returns from `rtclock_early_init()` and `kernel_debug_string_early()`.
- No marker `0x10A` (“kernel_startup_bootstrap returned”) appears. The physical frontier is now `0x109`, superseding Run #233's `0x104`.
- Video labels match Run #236 instrumentation. No device-side checksum was recorded, so the video is consistent with the Run #236 `iosboot.img`, but exact byte identity was not independently verified.
- A later black screen/charging indication is visible; cause is unknown and must not be labelled a kernel panic.

## Latest CI candidate at this handoff
- Run #236 ID `43701501661`, source `17a68fb21294a99e8c11c3ee38f7b79df501033d`, success.
- Both XNU build and RAM-only package jobs passed.
- `lavender-fastboot-boot-test` artifact ID `14156453574`; kernel artifact `14156363785`; candidate `14156289042`; build log `14156538354`; build tree `14157101726`.
- This is a successful compile/verify/package CI result. The Run #236 image is not yet recorded as physically tested in the evidence available here.

## Last physical proof
- Run #233 image reached C marker `0x104`: `arm_init`, `PE_init_platform`, topology parse, `cpu_bootstrap()`.
- No later C marker, VM init, Apple logo or Darwin userspace has been confirmed.
- The 60-second A53 guardian timeout and 2.5-second display-before-reset sequence explain the observed reset after a stalled marker.

## Recent CI progression
- #233: successful build/package; later physical recording established marker `0x104`.
- #234: initial post-`cpu_bootstrap` diagnostic patch attempt.
- #235: failed while repairing the patch hunk counts.
- #236: corrected diagnostic patch; build and package jobs both succeeded. Use this as the current test candidate, not #233, unless artifact provenance or newer CI evidence changes the choice.

## Older Stage-9 memory-translation failure
The earlier image stalled at assembly marker 9 before `arm_init()`. Under 16 KiB pages, the relevant L2 block size is 32 MiB. The old `physBase=0xA0400000` was offset by 4 MiB from the 32 MiB-aligned virtual base, mis-translating TTBR1 KVA accesses. The payload base was changed to `0xA2000000`, and subsequent physical evidence passed into C initialization. Do not re-open this completed investigation unless new evidence contradicts it.

## Topaz/Sweet reference provenance
- Topaz `MakrSas/ios-topaz` is a GitHub fork of `dipa-studio/ios-sweet`.
- Patch 0080 changes CPU MPIDR matching/identity and gates inherited SM7150-specific input drivers.
- Patch 0081 changes compiler targeting and selected ARMv8.0 compatibility paths.
- Patch 0082 expects boot MPIDR `0x100`.
- These are comparative references only; they do not prove Lavender needs the same modifications.

## Historical traps to avoid
- #220: patch 0107 skipped due to stale context; required-patch guard must remain.
- #224: legacy cache reused stale completed kernel; cache/version invalidation and build-path checks must remain.
- #227/#228: verifier segment selection and NUL parsing bugs; keep actual Mach-O section discovery and compiled-instruction checks.
- #231: verifier over-applied VM containment to non-executable file-backed `__CTF` metadata; do not weaken executable-section verification.
- Earlier IOKit config-table failure came from Lavender string insertion/patch placement; retain source guard.
- Run #235 failed; #236 is the corrected successful run. Don't confuse the failed candidate with the latest successful one.

## Truth statement
CI Run #236 compiled, verified and packaged a RAM-only image. The latest confirmed physical image remains Run #233 at marker `0x104`. There is no verified Apple logo or Darwin userspace yet.


## Current source-level stopping point

The exact Run #236 buildtree's XNU 8796 `osfmk/kern/startup.c` shows that `kernel_startup_bootstrap()` validates the startup-entry table, sorts it with `qsort`, then runs initializers through `STARTUP_SUB_LOCKS`. The callback loop advances `startup_entry_cur` only after each callback returns. The next diagnostic should print table validation/sort boundaries and per-callback begin/end events to the early Lavender console, without changing the Guardian's shared progress marker. This is instrumentation, not a confirmed failure diagnosis.


## New close-up hook screenshots and callback resolution — 15 November 2026

Two user-supplied close-up screenshots capture the bottom-row patch-0109 HOOK trace. The reported readable fields are idx=680, sub=3 (LOCKS), and rank=3. The accompanying analysis recovered Run #240's kernel artifact, extracted its startup table, replayed the original XNU 8796 qsort() ordering, and resolved the function and argument addresses against that kernel's symbol table. The resulting mapping is:

- Function: _lck_mtx_startup_init
- Argument/specification: ___startup_lck_mtx_spec_processor_updown_lock
- Meaning: startup initialization of the processor_updown_lock mutex.

This is substantially narrower than the earlier generic LOCKS-phase lead. Do not misstate the rank as 2; the reported trace rank is 3. Index 680 alone was not sufficient to name the callback; the mapping depends on the binary/table/symbol resolution described above.

### What the trace proves — and does not prove

Patch 0109 writes the hook record immediately before invoking cur->func(cur->arg). It does not emit a post-callback marker. Thus the last visible record identifies the callback about to run, but cannot by itself prove that the callback body is the exact hang site: the callback may not return, or execution may fail immediately after return in the pointer-advance/store path. The screenshots' exact tested-image provenance is not independently checksum-bound in this handoff; keep that distinction explicit even though the address resolution was performed against Run #240's exact kernel artifact.

### Follow-up

Inspect _lck_mtx_startup_init, the ___startup_lck_mtx_spec_processor_updown_lock startup specification/object, and the corresponding ARM64 lock initialization implementation in the Run #240 buildtree. Trace relevant dereferences, object layout, initialization state and platform assumptions. Make no speculative behavior change, callback skip, ordering change or rank change. Establish a concrete defect and focused validation first. Respect the independent recovery/reset safety gate before any further handset test.
