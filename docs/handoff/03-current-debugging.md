# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Last failed code under test:** `e463e56ca9d9c5b1d78c541ecfd751fe890f7a24` (XNU Run #252; handwritten commpage CASA)
**Current validation code:** `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba` (XNU Run #254; LL/SC commpage correction)
**Current corrective source:** `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba` (patch 0112)
**Corrective patch:** `0112-lavender-commpage-armv8a-llsc.patch` (added in the handoff fix commit)
**Last successful build/package:** Run #242, workflow ID `43729225770`, source SHA `6876fd9030add877b5cb8b02ff82e14ed6b9bc3f`; this predates the current mutex-subtrace and generic-CPU changes.
**Run #240 diagnostic baseline:** workflow ID `43709690089`, source SHA `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`, success
**Diagnostic patches:** `0109-lavender-startup-hook-trace.patch`, `0110-lavender-mutex-startup-subtrace.patch`, `0111-lavender-armv8a-generic-cpu.patch`, `0112-lavender-commpage-armv8a-llsc.patch`
**Run #240 image:** artifact `lavender-fastboot-boot-test`, ID `14160128075`; exact filename `iosboot.img`
**Run #240 image size / SHA-256:** 16,531,456 bytes / `e21d513f42f288e2cbd1705111507c9ba9ab398c7366b397227efb6b9227922a`
**Run #240 kernel size / SHA-256:** 16,587,456 bytes / `cba785aa6ccf6916ca86fde8ff97b1b4f583e888b5b57b8ba159b1e03743e712`
**Current CI:** inspection #25 (`43733584201`) passed. XNU #252 (`43733584216`, source `e463e56...`) failed because two handwritten `CASA` instructions in `commpage_asm.s` require LSE. Patch 0112 replaces them with ARMv8.0 `LDAXR/STXR` retry loops. XNU #254 (`43736404806`, source `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) is in progress; setup/toolchain/profile/cache/dependency steps passed and `Build XNU 8796` remains active. Source guards, linked-kernel verification, artifact collection and RAM-image packaging are pending. #253 (`43736362523`) is cancelled. No new verified boot image yet.
**Latest reviewed physical batch:** `1000317701.mp4` (87.785 s), `1000317713.mp4` (27.531 s), `1000317714.mp4` (27.744 s), `1000317717.mp4` (54.303 s); Run #240 still untested

## Live correction — patch 0112 was skipped in XNU #254

**Latest verified result:** XNU #254 (`43736404806`, source `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) is **failed**, not running. Build job `132879850377` completed with failure; package job `132884368318` was skipped. Its log explicitly says:

```text
Skipping patch 0112-lavender-commpage-armv8a-llsc.patch
error: patch fragment without header at line 24:
@@ -322,10 +327,15 @@ _pfz_trylock_and_dequeue:
```

The failure is a hunk-count mismatch, not a new architecture or mutex problem. Counting the actual patch lines gives:
- enqueue hunk: 11 old lines / 16 new lines, but header claimed 10 / 15;
- dequeue hunk: 9 old lines / 14 new lines, but header claimed 10 / 15.

Because patch 0112 was skipped, the original `CASA` instructions remained. The assembler then failed at `commpage_asm.s:276` and `:326` with `instruction requires: lse`. The raw kernel candidate artifact is `14171510404`; buildtree is `14170687291`; build log is `14170144930`. These are not verifier-approved. No `iosboot.img` was packaged.

**Corrective commit:** `24f81dc92b1b9bbe13a9cf15a0a5757ae5ed06ac` updates only the two hunk headers:
- `@@ -272,11 +272,16 @@ _pfz_trylock_and_enqueue:`
- `@@ -322,9 +327,14 @@ _pfz_trylock_and_dequeue:`

The ARMv8.0 `LDAXR/STXR` retry loops, lock-busy return behavior and existing release-store unlock are unchanged. This patch commit matches the `.github/workflows/xnu.yml` push path filter. The next Actions run has not yet been independently observed; do not invent a run ID or status.

**Next gate:** verify the new run first reports patch 0112 applied (no skip warning), then both source guards pass, the linked kernel passes the ARMv8-A Mach-O/instruction verifier, and the RAM-only package job uploads `iosboot.img`. Only then is a new candidate acceptable. Never use #254's raw kernel candidate.

## Failed build diagnosis and active correction — 15 November 2026

XNU #252 failed at the assembler stage, not in the mutex startup callback. The build log identifies two explicit LSE instructions in `osfmk/arm/commpage/commpage_asm.s`:
- line 276: `casa w10, w11, [x3]` in `_pfz_trylock_and_enqueue`;
- line 326: `casa w10, w11, [x2]` in `_pfz_trylock_and_dequeue`.

Both fail with `instruction requires: lse`. These are handwritten assembly instructions, so `-mcpu=generic -march=armv8-a` does not lower them automatically. #252's kernel-verification and RAM-only package jobs were skipped; its raw kernel candidate is not bootable/approved and no #252 `iosboot.img` exists.

Patch `0112-lavender-commpage-armv8a-llsc.patch`, corrected in source commit `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`, replaces both acquire-CASA operations with `LDAXR/STXR` retry loops. It retains the existing failed-lock return path and release-store unlock. The workflow now requires patch 0112 and checks both helper bodies for ARMv8.0 LL/SC sequences and absence of CASA.

**Current attempt:** XNU #254 (`43736404806`) uses the corrected source. Its runner setup, Xcode selection, pinned tools, machine-profile validation, cache handling and dependency steps passed. The latest observed job state has `Build XNU 8796` in progress; the latest log tail was still ordinary compile/header work. Do not assume the commpage assembler step has passed until the build completes or logs show it. Subsequent source guards, linked-kernel ISA verification, artifact collection and packaging must all pass before accepting `iosboot.img`.

The generic CPU target remains necessary but is not sufficient: audit any additional *explicit* LSE assembly instructions individually. Do not enable LSE globally, weaken the verifier, or use #252's raw candidate. Keep the Run #240 mutex callback lead separate; the present CI failure occurs earlier, during commpage assembly.

## Latest physical result — marker 0x109

The handset recording shows:
1. `arm_init entered (MMU on)`
2. `PE_init_platform done`
3. `ml_parse_cpu_topology done`
4. `cpu_bootstrap done`
5. `rtclock_early_init` entered and returned
6. `kernel_debug_string_early` entered and returned
7. `kernel_startup_bootstrap` entered

The last visible marker is `0x00000109`. Marker `0x10A` (written only after `kernel_startup_bootstrap()` returns) is absent. The checkpoint text matches Run #236 instrumentation; no device-side checksum was captured, so the association is consistent, not byte-for-byte proven. The clip later goes dark and shows a battery/charging indication. Its cause is unknown; do not call it a confirmed kernel panic.

Confirmed on the handset: C entry, platform init, CPU topology parsing, `cpu_bootstrap()` return, early clock init return, early debug-string return, and entry to startup bootstrap. Not confirmed: startup-bootstrap return, timer/CPU init, VM init, scheduler/interrupt stability, Apple logo or Darwin userspace.

## New video batch — 15 November 2026

Four physical recordings are reviewed as checkpoint-video evidence and remain separate from Run #240; none shows the full patch-0109 rolling trace. Two separate close-up screenshots expose a patch-0109 `HOOK` record; the screenshot-to-image provenance is still not checksum-bound. See the callback-resolution section below.

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

The callback is resolved to `_lck_mtx_startup_init` with `___startup_lck_mtx_spec_processor_updown_lock`. Its source path has been traced through `lck_mtx_init()` and `lck_grp_reference()`. The active build-level failure is now confirmed as two handwritten LSE CASA instructions in the commpage; see the failed-build diagnosis above. The generic CPU target alone cannot fix explicit assembly. Do not change callback order or bypass this mutex. Preserve the screenshot-to-image provenance uncertainty and keep device testing blocked by the recovery safety gate.

## Run #240 CI result and artifact provenance

- Run #240 / ID `43709690089`, source `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`: build and package jobs succeeded.
- Build job ID `132798332942`; package job ID `132800860081`.
- The required Lavender entry guard passed; the official Mach-O verifier passed ARM64 identity and compiled stage-8/stage-9 instruction checks. Package checks passed payload identity, ADT, DTB and non-overlapping load ranges.
- Artifact IDs: fastboot image `14160128075`; XNU kernel `14159918757`; buildtree `14160182439`; build log `14160376884`.
- The ZIP manifest binds the image to Run #240 and source SHA `c61d930...`. Its `iosboot.img` and `shim/out/shim-boot.img` have identical SHA-256 `e21d513f...`.
- Run #238 and #239 failed only at the temporary source-guard step while the guard assertions were being corrected. Run #240 is the corrected passing build/package.

## Patch 0109 — rolling startup trace

The current image adds console-only events to distinguish the exact stopping point inside `kernel_startup_bootstrap()`:

| Trace line | Meaning |
|---|---|
| `TABLE n=...` | startup table count, byte size and boundaries, before validation |
| `QSORT BEGIN` | entering the startup table sort |
| `QSORT DONE` | sort returned |
| `INIT BEGIN` | entering `kernel_startup_initialize_upto(STARTUP_SUB_LOCKS)` |
| `SUBSYS LOG BEGIN/DONE` | subsystem-name early logging boundary |
| `HOOK idx=... sub=... rank=... fn=... arg=...` | immediately before the named startup callback |
| `INIT DONE` | all callbacks through `STARTUP_SUB_LOCKS` returned |

The line is drawn at the bottom of the XNU framebuffer. Patch 0109 deliberately does **not** change the Guardian's shared progress marker. It neither changes callback order nor skips callbacks.

Interpretation:
- Last line `TABLE`: investigate validation/panic boundary or trace renderer.
- `QSORT BEGIN` without `QSORT DONE`: sort or table memory path.
- `QSORT DONE` without `INIT BEGIN`: immediate transition into initializer.
- `SUBSYS LOG BEGIN` without `DONE`: subsystem log path.
- Last line `HOOK idx=N ...`: inspect that exact callback and its argument; it is the next operation that did not visibly advance.
- `INIT DONE`: this phase completed; next existing marker should advance to `0x10A`.

## Run #240 compiled-instruction audit

The exact accepted `kernel.development.lavender` was disassembled from artifact `14159918757`; this confirms patch 0109's trace placement in the linked ARM64 binary, not merely in source:

- `_kernel_startup_bootstrap`: `0xfffffe0007a32ff4`
- `_startup_entry_cmp`: `0xfffffe0007a32fbc`
- `_qsort`: `0xfffffe0007810690`
- `_kernel_startup_initialize_upto`: `0xfffffe0007a33164`
- `_sweet_early_trace_startup`: `0xfffffe0007475c1c`
- The trace event 5 call is at `0xfffffe0007a3323c`; the immediately following `blr x8` invokes `cur->func(cur->arg)` at `0xfffffe0007a33244`. Only after the callback returns does the kernel increment the entry pointer and store `startup_entry_cur` at `0xfffffe0007a33248–0xfffffe0007a3324c`.

Therefore a frozen `HOOK idx=N` identifies the callback being entered, but because patch 0109 has no explicit post-callback event, it cannot fully distinguish a callback that has not returned from a fault in the immediately following pointer-advance/store path. Do not change callback behavior based on the line alone.

The accepted table's 874 pre-lockdown callbacks comprise:
- TUNABLES (242): 158 `_kernel_startup_tunable_init`, 49 `_scalable_counter_static_boot_mangle`, 16 `_sfi_class_register`, 6 `_kernel_startup_tunable_dt_init`, plus 13 single/small-count initializers.
- TIMEOUTS (18): 15 `_machine_timeout_init`, and one each of `_hw_lock_test_to_init`, `_ml_io_init_timeouts`, and `_ml_init_lock_timeout`.
- LOCKS (614): 280 `_lck_grp_startup_init`, 184 `_lck_mtx_startup_init`, 58 `_lck_attr_startup_init`, 43 `_lck_rw_startup_init`, 28 `_lck_spin_startup_init`, 13 `_usimple_lock_startup_init`, and eight remaining callbacks.

Static table and linked-code checks show no malformed table, unexpected callback pointer, invalid non-null argument mapping, or simulated comparator-order defect. This is not evidence that the phone ran those callbacks successfully. The screenshot trace now identifies the callback entry in the Run #240 table/symbol analysis. Physical provenance of the screenshot must still be confirmed before calling it a byte-bound Run #240 handset test. Do not modify or gate the initializer until its source path is audited.

## Exact Run #240 startup-table audit

The accepted kernel's `__BOOTDATA,__init_entry_set` is file offset `0xb1cd58`, size `0x17e20` (97,824 bytes), exactly 4,076 entries × 24 bytes. Its exact entries were extracted from the Run #240 Mach-O and sorted using the XNU 8796 `bsd/kern/qsort.c` algorithm and the actual startup comparator.

- Host-side simulation: 10,790 comparisons, maximum recursion depth 7.
- Sorted phase counts: TUNABLES 242, TIMEOUTS 18, LOCKS 614 (874 callbacks total before `STARTUP_SUB_LOCKS` completes).
- All 4,076 function pointers resolve inside mapped executable segments.
- All non-null argument pointers fall inside mapped kernel segments.
- Zero subsystem/rank ordering violations after the simulated sort.
- Linked kernel includes `_sweet_early_trace_startup`, `_kernel_startup_bootstrap`, `_kernel_startup_initialize_upto` and `_qsort`.

This rules out an empty/malformed table and makes a pathological sort workload less likely. It does not prove that the phone's physical sort returns or identify a callback failure. The rolling Run #240 trace is the decisive next evidence.

### Device-tree startup tunable audit

All six `kernel_startup_tunable_dt_init` specifications in the exact Run #240 startup table were decoded and checked against the packaged `lavender.adt`:

- `/defaults:kern.swap_min_capacity`: `/defaults` exists, property absent.
- `/machine-timeouts:interrupt-masked-debug-mode`, `sched-preemption-disable-mode`, and `nonspec-tb`: the requested machine-timeouts nodes/properties are absent, including the optional `/chosen/machine-timeouts` override.
- `/arm-io/cpu-debug-interface:panic-trace-mode`: target node absent.
- `/chosen:research-enabled`: `/chosen` exists, property absent.

These are optional device-tree tunables: `kernel_startup_tunable_dt_init()` skips missing nodes/properties and then checks the corresponding boot argument. They are not a property-size mismatch in this ADT.

The CSR tunable callback also checks `/chosen/asmb`. That node exists; `lp-sip0` is 8 bytes and `lp-sip1` is 4 bytes, matching `_csr_get_dt_uint64()` and `_csr_get_dt_bool()` in XNU 8796. This specific CSR/ADT size path is not a strong stall candidate.


## Latest callback resolution — screenshot trace correlated with Run #240 kernel

Two user-supplied close-up screenshots expose the bottom-row patch-0109 HOOK record. The reported readable fields are:

```
subsystem: 3 (LOCKS)
index:     680
rank:      3
function:  _lck_mtx_startup_init
argument:  ___startup_lck_mtx_spec_processor_updown_lock
```

The accompanying binary-analysis report states that the original XNU 8796 qsort() ordering was replayed against the startup table extracted from Run #240's actual kernel, and the function/argument addresses were resolved against that same kernel's symbol table. This maps the displayed entry to initialization of the processor_updown_lock mutex. Treat this as a symbol-resolution result for the Run #240 kernel, not a generic inference from index 680.

**Interpretation boundary:** patch 0109 emits HOOK immediately before cur->func(cur->arg); it has no post-callback event. A last-visible record therefore establishes the callback about to be invoked, but does not by itself prove the callback is where execution hangs. The callback may fail to return, or execution may fault in the immediate post-call pointer-advance/store path. Also preserve screenshot-to-image provenance as unverified unless the tested image is independently bound to the Run #240 artifact.

**Next engineering operation:** inspect the exact XNU 8796 implementation of _lck_mtx_startup_init, the ___startup_lck_mtx_spec_processor_updown_lock object/specification, and the ARM64 lock/mutex initialization path in the Run #240 buildtree. Trace every dereference and platform-dependent assumption relevant to this lock. Compare against the exact linked addresses and build configuration. Do not patch, skip, reorder, or gate the initializer until the source path identifies a concrete defect. Prefer a narrow source-level reproduction/guard and add post-callback diagnostic evidence only if needed; do not start another device test until the independent recovery/reset safety gate is satisfied.

## ARM64 compiler-target mismatch — 15 November 2026

The Run #240 build log shows Lavender C compilation with `-march=armv8-a -arch arm64`. On the same Xcode 16.0 toolchain selected by the XNU workflow, those flags resolve to Clang `-target-cpu apple-m1`; `-march=armv8-a` alone does not override the Apple-M1 CPU default.

The exact Run #240 kernel's `_lck_grp_reference` contains an LSE `LDADD` at `0xfffffe0007324cd4`. The atomic counter update is therefore compiled for an instruction extension that is not part of the stated ARMv8-A baseline. A compiler probe on Xcode 16.0 confirms that adding `-mcpu=generic` changes the target CPU to `generic` and emits the relaxed atomic sequence as `LDXR/STXR`, not `LDADD`.

Patch `0111-lavender-armv8a-generic-cpu.patch` changes only `MACHINE_FLAGS_ARM64_LAVENDER` in `makedefs/MakeInc.def` to append `-mcpu=generic`. The XNU workflow now:
- requires patches 0110 and 0111;
- uses a v3 cache key and removes the old `dxb/build/xnu.obj` tree plus its exported-header sentinel on a non-exact cache hit, preventing Apple-M1 objects from surviving the ISA change;
- checks the actual compile command for `-march=armv8-a -mcpu=generic -arch arm64`;
- disassembles the linked kernel and requires `_lck_grp_reference` to use `LDXR/STXR` with no LSE atomic mnemonic.

**Causality limit:** `compact_id_table_init()` is an earlier LOCKS startup callback that also calls `lck_mtx_init()` and reaches `lck_grp_reference()`. If the observed phone image truly executed that earlier path successfully, LSE alone would not explain a later freeze at the processor-up/down mutex. Treat 0111 as a concrete ISA-portability correction and a controlled diagnostic step, not as proof that the callback stall is solved.

Inspection #17 confirmed the Apple-M1 compiler default and decoded the Run #240 `LDADD`. Inspection #20 passed the Xcode 16.0 atomic probe using the generic target, but stopped later on an invalid 0111 patch hunk. The 0111 patch has since been regenerated with full context and locally checked against the exact Run #240 build-tree. Inspection #25 (`43733584201`) completed successfully. XNU Run #252 (`43733584216`) did not pass: the build failed on the two handwritten CASA instructions described below, and the verifier/package steps were skipped. Do not describe #252 as queued or successful.

Patch `0110` adds events 9–15 around mutex assignment and group-reference handling, plus event 16 after the startup callback returns. It is intended to distinguish mutex-field initialization, the atomic counter update, optional group refcount retention, and callback return. Do not interpret a pre-callback `HOOK` as proof that the callback itself is the failing instruction.

## Run #252 failure — handwritten commpage LSE instructions

**Run:** XNU #252, workflow `43733584216`, source `e463e56ca9d9c5b1d78c541ecfd751fe890f7a24`; failed at 16:30:14 UTC. The job completed runner setup, Xcode 16.0 selection, profile validation, cache handling and dependencies, then stopped in ARM64 commpage assembly:

```text
osfmk/arm/commpage/commpage_asm.s:276:2: error: instruction requires: lse
 casa w10, w11, [x3]
osfmk/arm/commpage/commpage_asm.s:326:2: error: instruction requires: lse
 casa w10, w11, [x2]
```

The instructions are handwritten in `_pfz_trylock_and_enqueue` and `_pfz_trylock_and_dequeue`. `-mcpu=generic -march=armv8-a` controls compiler-generated code but cannot rewrite explicit CASA mnemonics. This is a separate remaining LSE source from the earlier C atomic in `_lck_grp_reference`. Do not re-enable LSE or change the target CPU.

Run #252 uploaded raw diagnostic artifacts: kernel candidate ID `14169598249`, buildtree ID `14170183341`, build-log ID `14170312913`. The kernel candidate was not verifier-approved, the package job was skipped, and no `iosboot.img` exists for #252.

**Narrow correction:** patch `0112-lavender-commpage-armv8a-llsc.patch` replaces each acquire CASA with `LDAXR` followed by `STXR` retry logic. A nonzero lock value follows the existing busy-lock failure return; the existing release-store unlock remains unchanged. The XNU workflow requires patch 0112, checks both helper bodies for LL/SC and absence of CASA, and verifies the patch/source guard before accepting the build.

**Patch correction history:** the initial 0112 patch attempt had incorrect hunk counts / source-guard matching. Commit `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba` corrected the patch hunks and commpage source guard. The workflow file at that exact commit requires 0112.

**Follow-up runs — live status checked 15 Nov 2026 22:19 IST:**
- XNU #253 (`43736362523`, source `3fd3bf03db75f80f64c15c3d7dc610116b437b89`) is **cancelled**; it used the initial 0112 patch before hunk/source-guard correction.
- XNU #254 (`43736404806`, exact source `58bda409ddc1c6e67d077c6acdf5f4ca9790dfba`) is **in progress**. Job `132879850377`: runner setup, checkout, Xcode selection, pinned tooling, profile validation, cache restore, runner-capacity detection, dependency installation and legacy-cache invalidation all passed. Step 10, **Build XNU 8796**, is still running. The cache-aware build guard, Lavender entry guard, build-log upload, buildtree packaging, kernel collection, linked-kernel verification, cache save and fastboot-image package job are pending. The latest log tail contains ordinary compile/header work but does not yet prove the commpage assembly objects completed; do not infer success from the earlier setup steps.

#254 is the first validation run against corrected patch 0112. Keep the exact failure distinction: #252 failed on handwritten CASA, #253 was cancelled, #254 is running the LL/SC correction.

Do not assume this is the last unsupported instruction. If the corrected build exposes further handwritten LSE sites, fix each individually while preserving ARMv8-A.

## Immediate next operation

1. Keep #254 as the only active XNU build; do not start duplicate builds while its runner is compiling.
2. Monitor the `Build XNU 8796` step to completion. If it fails, inspect the first assembler/compiler error and fix that exact source site without enabling LSE globally. The known #252 failure is the two handwritten commpage CASA instructions; patch 0112 is the current correction.
3. Require both source guards, successful linked Mach-O verification and the RAM-only package job. Record new kernel/image artifact IDs and SHA-256 values only after those gates pass.
4. Do not use #252's raw kernel candidate as a boot image. Do not boot any new image until a power-button-independent recovery/reset route has been physically verified; RAM boot only, never flash.


## Device safety

- Preserve Android and recovery. No flashing, erasing, repartitioning or writes to boot-critical partitions.
- Temporary RAM boot only; never use `fastboot flash`.
- The device power button is broken. No new handset test until a power-button-independent recovery/reboot path is established.
- Do not commit the recording or raw device dumps.

## Acceptance ladder

- [x] Enter `arm_init()`
- [x] Complete `PE_init_platform`
- [x] Parse CPU topology
- [x] Return from `cpu_bootstrap()`
- [x] Return from `rtclock_early_init()`
- [x] Return from `kernel_debug_string_early()`
- [x] Enter `kernel_startup_bootstrap()`
- [ ] Return from `kernel_startup_bootstrap()`
- [ ] Complete timer and CPU initialization
- [ ] Enter and return from `arm_vm_init()`
- [ ] Stable scheduler / interrupts / secondary CPU startup
- [ ] Apple logo
- [ ] Darwin userspace
