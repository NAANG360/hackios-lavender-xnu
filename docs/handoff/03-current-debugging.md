# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Test candidate:** Run #236, ID `43701501661`, source SHA `17a68fb21294a99e8c11c3ee38f7b79df501033d`
**Test image:** artifact `lavender-fastboot-boot-test`, ID `14156453574`; exact filename `iosboot.img`
**Image SHA-256:** `726a0708d2c47b21e26ddb2b7331b303013fc6b0bf3f0517f9a0cfd1d02758f8`
**Physical evidence:** video `1000317701.mp4`, 87.8 seconds

## Latest physical result — marker 0x109

The new recording shows the custom Lavender XNU diagnostic console on the physical handset. The visible checkpoint list advances through:

1. `arm_init entered (MMU on)`
2. `PE_init_platform done`
3. `ml_parse_cpu_topology done` (topology parsing complete)
4. `cpu_bootstrap done`
5. `entering rtclock_early_init`
6. `rtclock_early_init returned`
7. `entering kernel_debug_string_early`
8. `kernel_debug_string_early returned`
9. `entering kernel_startup_bootstrap`

The last visible marker is `0x00000109`. Marker `0x10A`, which is written only after `kernel_startup_bootstrap()` returns, is absent. The trace labels uniquely match the Run #236 patch chain; however, no device-side checksum was captured, so provenance is “consistent with the delivered Run #236 image,” not byte-for-byte proven.

The clip goes dark near its end and shows a battery/charging indication. The reason is not established. It is compatible with the Guardian timeout/reset behavior, but does not prove a kernel panic or identify the failure site.

## What this proves — and what it does not

Confirmed on the handset: entry to `arm_init()`; platform initialization; CPU-topology parsing; return from `cpu_bootstrap()`; return from `rtclock_early_init()`; return from `kernel_debug_string_early()`; entry to `kernel_startup_bootstrap()`.

Not confirmed: return from startup bootstrap, timer-call initialization, `cpu_init()`, `processor_bootstrap()`, `arm_vm_init()`, scheduler/interrupt stability, Apple logo or Darwin userspace.

The previous physical frontier was Run #233 marker `0x104`. Run #236's test has moved that frontier to `0x109`.

## Run #236 marker interpretation

Patch `xnu/patches/0108-lavender-post-cpubootstrap-trace.patch` adds:

| Value | Meaning |
|---|---|
| `0x105` | before `rtclock_early_init()` |
| `0x106` | after `rtclock_early_init()` |
| `0x107` | before `kernel_debug_string_early()` |
| `0x108` | after `kernel_debug_string_early()` |
| `0x109` | before `kernel_startup_bootstrap()` |
| `0x10A` | after `kernel_startup_bootstrap()` |
| `0x10B` | after `timer_call_init()` |
| `0x10C` | after `cpu_init()` |
| `0x10D` | after `processor_bootstrap()` |
| `0x10E` | entering `arm_vm_init()` |
| `0x10F` | after `arm_vm_init()` |

## Narrow next target: XNU startup bootstrap

The exact Run #236 buildtree artifact (`xnu-buildtree`, ID `14157101726`) was inspected. In `xnu/osfmk/kern/startup.c`, `kernel_startup_bootstrap()`:

1. Computes the number of `startup_entries`.
2. Checks for an empty table and verifies that its byte length is an exact multiple of `sizeof(struct startup_entry)`.
3. Sorts the table using `qsort(startup_entries, n, sizeof(struct startup_entry), startup_entry_cmp)`.
4. Calls `kernel_startup_initialize_upto(STARTUP_SUB_LOCKS)`.

The initializer asserts `startup_phase < upto`, then walks entries while `cur->subsystem <= upto`. For each entry it calls `cur->func(cur->arg)`; `startup_entry_cur` advances only after the callback returns. Thus the current `0x109` marker cannot tell whether the stall occurs during table validation, sorting, or one particular initializer callback.

### Diagnostic patch design

Add console-only trace events (no behavior changes) around:
- startup table count/byte-size validation;
- immediately before and after `qsort`;
- immediately before and after `kernel_startup_initialize_upto(STARTUP_SUB_LOCKS)`;
- each eligible callback, logging begin/end with index, subsystem, rank and function address.

The current early console has 108 text rows. The Run #236 source-tree scan found a modest number of ARM64-relevant `STARTUP(TUNABLES/TIMEOUTS/LOCKS,...)` registrations, so per-callback begin/end output should remain bounded. Verify the final linked hook-table order rather than assuming source grep order.

**Do not update the Guardian shared marker cell for these sub-events.** The Guardian treats any changed cell value as progress, so sub-markers there could postpone its 60-second timeout or trigger the special stage-20 high-bit behavior. Add a separate console-only helper in the Lavender early-console path. Do not reuse stages 16, 17 or 20; those are already reserved by later logo/guard flow. Preserve `0x109` as the Guardian marker during this investigation.

Do not alter callback order, skip hooks, weaken assertions, or add speculative hardware workarounds. Inspect the generated patch and source-level invariants before triggering one new CI build.

## Device safety

- Preserve Android and recovery. No flashing, erasing, repartitioning or writes to boot-critical partitions.
- Only temporary RAM boot is allowed.
- The device power button is broken. Do not ask for another handset test until a recovery/reboot path independent of that button is physically established.
- Volume Up + Volume Down is not a verified recovery selector; the documented chord requests reset only.
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
