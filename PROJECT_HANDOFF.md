# Lavender XNU — master handoff

**Read this file only at session start.** Do not read every chapter. Use the chapter index below and open only the chapter needed for the task.

- Repo: https://github.com/NAANG360/hackios-lavender-xnu
- Active branch: `ci/lavender-machine-profile`
- Target: Xiaomi Redmi Note 7/7S (`lavender`), Qualcomm SDM660, ARM64
- Mission: XNU 8796 / iOS 16 generation on physical Lavender; longer-term iOS 16.5-era Darwin userspace.
- Last branch HEAD observed before this documentation split: `e8c2e2303d765835e95fb79f3f15e711f3f82020`. **Always resolve live HEAD before editing.**

## Current state — read first

**Physical bring-up blocker:** Run #232's footage shows the A73 reaching XNU assembly Stage 9, immediately before `ret` to `arm_init()`. The A53 guardian later timed out and intentionally reset the handset. Current source-level root-cause candidate is a 4 MiB TTBR1 mapping offset: `physBase=0xA0400000` is misaligned for 32 MiB L2 block descriptors while `virtBase` is aligned.

- Fix commit: `f2a4cfd4a65829352736783def33c6a5b61e4a68`.
- `tools/mkadt-lavender.py`: XNU aperture now begins at `0xA2000000`, the next 32 MiB boundary after audited RAM start `0xA0400000`; advertised end remains `0xEA848000` before the 1 MiB panic reservation.
- `tools/mkpayload.py`: rejects Lavender payloads whose `virt_base` and `phys_base` offsets differ modulo 32 MiB.
- `tests/test_lavender_adt.py`: checks aligned base and rejects the old `0xA0400000` value.
- Shim/ADT workflow #47 (`43695509648`) passed, including ADT invariants, regression test and shim compile.
- XNU workflow #233 (`43695509652`) is in progress. No new kernel/image is accepted yet.

**Memory-map caveat:** the current aperture boundary is based on the live Linux `/proc/iomem` audit recorded in the ADT builder, not a complete bootloader-provided RAM/reservation map. Treat the change as a build candidate until the exact runtime memory map and all reserved regions are reconciled. No physical test yet.

## Immediate next action

1. Poll XNU Run #233. Confirm build path, official Mach-O verifier, instruction verification and preserved artifacts.
2. If successful, manually package only from #233 and inspect exact `iosboot.img`, payload/ADT values, kernel identity, DTB and load ranges.
3. Before any handset test, resolve the RAM-map caveat against the best available bootloader/runtime evidence and establish recovery independent of the broken power button.
4. If the corrected image is eventually tested, expect marker `0x100` as proof of C entry; later C-stage markers and console output are required before claiming further progress.

**Do not flash. Do not infer physical success from CI.**
## Chapter index — load selectively

| Chapter | Open when |
|---|---|
| [01 — Device and safety](docs/handoff/01-device-and-safety.md) | Hardware, FDT/memory, recovery, physical test safety, source authority |
| [02 — Architecture and build](docs/handoff/02-architecture-and-build.md) | XNU markers, patches, workflow/cache, package construction, key paths |
| [03 — Current debugging](docs/handoff/03-current-debugging.md) | Resuming the current verifier failure; detailed #231 facts and exact next steps |
| [04 — Evidence and history](docs/handoff/04-evidence-and-history.md) | Prior verified artifacts, physical observations and old failure traps |

## Session protocol

- Start: read this file, then inspect live branch HEAD and latest relevant Actions run.
- For the active task: read Chapter 03 only; open another chapter only if needed.
- Before changing workflow/package: read Chapter 02. Before device testing: read Chapter 01. For historical comparisons: read Chapter 04.
- Never load all chapters "for context" by default.
- Treat GitHub Actions logs/artifacts and current source as authoritative over stale prose. Update only the relevant chapter and this short current-state section after a milestone.
- Keep observed facts separate from hypotheses. Distinguish compile, verified kernel, packaged image, physical boot, Apple logo and Darwin userspace.
