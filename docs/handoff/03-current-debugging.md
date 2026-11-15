# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Latest docs HEAD before this update:** `c6964327ef60c0bb3370da2630fab12332fbad8e`
**Latest tested source:** `f2a4cfd4a65829352736783def33c6a5b61e4a68`

## Latest physical evidence — Run #233 image

The user tested the #233 `iosboot.img` on the physical Redmi Note 7. The new recording (73 seconds) displays these sequential C-level diagnostics:

1. `arm_init entered (MMU on)`
2. `PE_init_platform done`
3. `dt_parse_cpu_topology done`
4. `cpu_bootstrap done`

This is a major verified advance over Run #232: XNU has entered `arm_init()` and progressed through platform initialization, device-tree CPU topology parsing and `cpu_bootstrap()`. The former Stage 9 stall immediately before C entry is no longer the active failure.

**Current frontier:** after the `cpu_bootstrap done` marker. Do not infer that the scheduler, secondary CPU bring-up, interrupt controller, console, launchd, userspace or Apple logo has been reached. The recording's displayed milestones establish progress only through the fourth marker. Before naming the precise faulting instruction/function, inspect the full video and current marker/guardian source; do not invent an exception diagnosis.

## Previous failure and correction (retained context)

Run #232 stopped at assembly marker 9, immediately before `ret` to `arm_init()`. The A53 guardian showed `A53 LAST MARK=0x00000009`, then intentionally reset after its timeout.

Source analysis found a 16 KiB-granule L2 block mapping alignment issue:
- L2 block size: `0x02000000` (32 MiB).
- `virtBase = 0xFFFFFFE006000000` (32 MiB aligned).
- Old `physBase = 0xA0400000` (4 MiB offset).
- The XNU block descriptor masks the physical output to the L2 block boundary, potentially translating 4 MiB below the shim's copied segment. This was a strong source-backed explanation; no synchronous exception was captured.

Correction commit `f2a4cfd4a65829352736783def33c6a5b61e4a68`:
- Lavender ADT `PHYS_BASE=0xA2000000`, next 32 MiB boundary.
- Advertised usable endpoint `0xEA848000`, followed by 1 MiB panic reservation to `0xEA948000`.
- Payload validation requires virtual and physical bases to have equal offsets modulo 32 MiB.
- Regression tests assert the aligned base and reject old `0xA0400000`.

## Memory-map evidence from Android

User supplied a root-terminal capture from the running Lavender:
- Device properties: `lavender`, `sdm660`, `qcom`.
- `/proc/iomem`: System RAM range `0xA0400000–0xFEABFFFF`; reserved range `0xEA948000–0xFE7FFFFF`.
- Device tree `memory/reg` and reserved-memory nodes were also captured.
- Proposed `0xA2000000` base is 32 MiB aligned and lies in reported System RAM.
- The proposed endpoint `0xEA848000` is below the first upper reserved region at `0xEA948000`; the 1 MiB interval between them is the panic reservation.
- Lower firmware carveouts shown in the captured DT are below the proposed base.

This reconciles the proposed aperture with the supplied Android boot's visible memory/reservation map. It is evidence for this device/boot configuration, not a universal Lavender memory map.

## Run #233 CI validation

Run `43695509652`, source commit `f2a4cfd4...`: **success**.
- XNU 8796 build: passed.
- Lavender kernel artifact verifier: passed.
- Packaging job: passed.
- Exact stock boot image and DTB extraction: passed.
- Lavender ADT generation and XNU payload build: passed.
- Non-flashing fastboot boot image construction: passed.
- Boot image structure and payload identity checks: passed.
- Artifact `lavender-fastboot-boot-test`, ID `14154570502`, contains `iosboot.img`.
- Shim/ADT Run #47, ID `43695509648`: success.

CI success validates build/package gates, not physical boot success. Physical recording provides the C-init progress above.

## Focused next investigation

1. Read current XNU `arm_init()` implementation and locate the exact call immediately after `cpu_bootstrap()` for this source revision.
2. Read shim marker definitions, guardian timeout/reset behavior and diagnostic display code. Establish what marker follows #4 and whether a reset can obscure later output.
3. Add narrowly spaced progress markers around the next initialization calls, plus a useful synchronous exception/fault capture if one is not already present. Keep diagnostics on the known-working display path.
4. Inspect any last-marker / timeout state from the new recording before selecting a single root-cause hypothesis. Do not shotgun unrelated platform changes.
5. Build once, package exact `iosboot.img`, run static verification, then test via RAM-only `fastboot boot iosboot.img`.

## Acceptance ladder

- [x] XNU assembly reaches `arm_init()`.
- [x] `PE_init_platform` completes.
- [x] `dt_parse_cpu_topology` completes.
- [x] `cpu_bootstrap` completes.
- [ ] Continue through remaining `arm_init()` platform initialization.
- [ ] Scheduler / idle-thread startup.
- [ ] Stable secondary CPU and interrupt bring-up.
- [ ] Kernel reaches a stable console / boot milestone.
- [ ] Apple logo.
- [ ] Darwin userspace.

Never report an unchecked stage as complete.
