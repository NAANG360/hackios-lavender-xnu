# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Current HEAD:** `f2a4cfd4a65829352736783def33c6a5b61e4a68`

## Physical evidence

Run #232 image was tested on the Redmi Note 7. The recordings show:
- Shim starts on A53 (MPIDR `0x0`), wakes A73 (MPIDR `0x100`) and loads the XNU payload.
- A73 jumps to `start_first_cpu`, enables the 16 KiB-granule MMU and completes assembly markers through `0x9`.
- Marker 9 is written immediately before `ret` to `arm_init()`.
- After 60 seconds without a new marker, the A53 guardian displays `A53 LAST MARK=0x00000009`, waits 2.5 seconds and intentionally resets the device.

This proves execution reached the final assembly handoff checkpoint, not successful C entry or a complete kernel boot.

## Root-cause analysis

Apple's `create_l2_block_entries` masks the physical output address with `ARM_TTE_BLOCK_L2_MASK` before writing an L2 block descriptor. For the Lavender 16 KiB granule profile, the block size is 32 MiB. Therefore the virtual and physical bases must have matching offsets modulo `0x02000000`.

Observed payload values:
- `virtBase = 0xFFFFFFE006000000` (32 MiB aligned)
- `physBase = 0xA0400000` (offset `0x00400000`)

The KVA block descriptor drops the low 4 MiB from its output base. For the arm_init KVA, that predicts a physical fetch 4 MiB below the address where the shim copied the segment. This matches the Stage 9 stall. The mapping math is source-confirmed; the exact synchronous exception after `ret` was not captured.

## Correction committed

Commit `f2a4cfd4a65829352736783def33c6a5b61e4a68`:
- Set Lavender ADT `PHYS_BASE=0xA2000000`, next 32 MiB boundary above audited RAM start `0xA0400000`.
- Preserve advertised aperture end `0xEA848000` (with 1 MiB panic reservation).
- Add `mkpayload.py` validation requiring `virt_base % 32MiB == phys_base % 32MiB`.
- Extend `tests/test_lavender_adt.py` to assert the new base and reject the old one.

## Validation status

- Lavender shim/ADT Run #47, ID `43695509648`: **success** — ADT generation, regression tests and shim compile passed.
- XNU Run #233, ID `43695509652`: **in progress** — do not call the new kernel verified until official steps pass.
- Run #232 remains the last successfully packaged candidate; it is not a known-good physical boot.

## Remaining caveat and next steps

The current RAM aperture is based on a live Linux `/proc/iomem` report. Repository safety notes explicitly say Linux-visible ranges do not replace a complete bootloader RAM/reservation map. Reconcile the proposed `0xA2000000..0xEA948000` region with runtime/bootloader evidence before another handset test.

1. Finish Run #233 and inspect verifier/artifacts.
2. Package only from the successful #233 kernel run.
3. Verify payload ADT `dram-base`, `dram-size`, segment destination ranges, boot_args/ADT placement, DTB and package hashes.
4. Establish a reliable recovery route independent of the broken power button.
5. Only then consider another RAM-only `fastboot boot iosboot.img` test. Never flash.

Next expected positive marker: `0x00000100` for C entry into `arm_init()`. Do not claim C entry unless it is observed.