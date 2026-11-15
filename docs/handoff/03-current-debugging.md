# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Latest build/package:** Run #240, workflow ID `43709690089`, source SHA `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`, success
**Diagnostic patch:** `xnu/patches/0109-lavender-startup-hook-trace.patch`
**Diagnostic image:** artifact `lavender-fastboot-boot-test`, ID `14160128075`; exact filename `iosboot.img`
**Image size / SHA-256:** 16,531,456 bytes / `e21d513f42f288e2cbd1705111507c9ba9ab398c7366b397227efb6b9227922a`
**Kernel size / SHA-256:** 16,587,456 bytes / `cba785aa6ccf6916ca86fde8ff97b1b4f583e888b5b57b8ba159b1e03743e712`
**Last physical recording:** `1000317701.mp4`, Run #236-consistent video, 87.8 seconds

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


## Immediate next operation

1. Do not trigger another XNU build yet.
2. Before requesting a new handset test, establish and physically verify a reboot/recovery path that does not depend on the broken power button. Volume Up + Volume Down is only documented as a reset request; it is not a verified recovery selector.
3. Once a safe exit/recovery route exists, RAM-boot only the exact Run #240 `iosboot.img` and record the last complete bottom-row trace plus Guardian marker.
4. Inspect only the indicated source callback or boundary, make the smallest evidence-backed fix, and then build once.

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
