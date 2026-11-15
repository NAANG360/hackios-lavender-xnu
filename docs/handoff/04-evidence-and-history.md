# Chapter 4 — Evidence, provenance and historical traps

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
