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
