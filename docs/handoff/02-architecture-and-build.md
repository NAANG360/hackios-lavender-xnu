# Chapter 2 — Architecture, build and comparative references

## Acceptance gates — keep separate
1. Required source/patches applied and guarded.
2. XNU builds for `DEVELOPMENT ARM64 LAVENDER`.
3. ARM64 Mach-O identity/structure and compiled instruction checks pass.
4. RAM-only image packages and passes structural/payload checks as exact `iosboot.img`.
5. Physical XNU stages are observed.
6. Apple logo is observed.
7. Darwin userspace / SpringBoard is reached.

A green CI build or packaged image does not prove physical boot.

## Lavender implementation
- XNU 8796 target: `DEVELOPMENT ARM64 LAVENDER`.
- `0106-lavender-verbose-boot-final.patch`: early framebuffer/boot diagnostics.
- `0107-lavender-post-assembly-entry-trace.patch`: assembly stages 8/9, C marker namespace and cache-clean publication.
- `0108-lavender-post-cpubootstrap-trace.patch`: markers around clock, early startup hooks, timer, CPU initialization and VM-init entry.
- `0109-lavender-startup-hook-trace.patch`: console-only rolling trace for startup table validation, qsort, subsystem logging and each callback through `STARTUP_SUB_LOCKS`; does not update Guardian state or change callback behavior.
- C marker encoding: `0x100 | stage`; publish before framebuffer output; `dc cvac` then `dsb sy`.
- Apple mark is deferred until VM initialization and permanent framebuffer alias setup.

## Current CI / packaging facts
- `.github/workflows/xnu.yml`: patch application, source guards, XNU build, Mach-O/instruction verification, artifacts and cache.
- `.github/workflows/package-lavender.yml`: manual package workflow; requires a successful XNU run, binds the kernel artifact to source SHA, extracts stock SDM660 boot/DTB, generates Lavender ADT/payload and makes a non-flashing image.
- Run #240 (ID `43709690089`, source `c61d930ac2fc379a2bdacfb9daa803b013d3f9ac`) build and package jobs passed with patch 0109. Artifact IDs and physical status are in Chapter 03. Runs #238/#239 were temporary source-guard failures before Run #240 corrected the assertions.
- Packaging is not automatic in general. Verify the exact run/source SHA, artifact, `iosboot.img` filename, manifest, checksums, DTB/ADT, payload identity, load ranges and structure.
- Cache restore may reuse objects; source files changed by patches must be invalidated/touched. Verify logs show the expected actual build path. Never weaken checks or assume a cache hit is valid.
- Do not overlap Actions builds.

## ARMv8.0 commpage LL/SC validation

The pinned macOS 13.5 release resolves to XNU tag `xnu-8796.141.3`. In that exact source, `_pfz_trylock_and_enqueue` and `_pfz_trylock_and_dequeue` are in `__TEXT_EXEC,__commpage_text`, not the ordinary `__text` section. The helpers document that they use only caller-saved registers x9–x15, so w12 is within the documented scratch-register set for the STXR status result.

Patch `0112-lavender-commpage-armv8a-llsc.patch` changes the two acquire-CASA operations to `LDAXR/STXR` retry loops while preserving the busy-lock return and the existing `STLR` unlock. Keep LSE disabled globally; explicit assembly sites must be corrected individually.

The new `tools/verify-lavender-commpage-llsc.py` guard checks the post-patch source and can independently inspect linked ARM64 disassembly. Workflow hardening is prepared to:
- parse the 0112 unified diff with `git apply --numstat` before the expensive build;
- execute the source guard immediately after the pinned build script's patch stage and before compilation;
- disassemble `__commpage_text` separately from `__text`, then require both linked helper symbols to contain `LDAXR/STXR` and no LSE atomic mnemonic.

These are build/source/binary acceptance checks only; they do not prove handset execution.

## Comparative codebase audit: Topaz / Sweet

Repositories:
- Topaz: https://github.com/MakrSas/ios-topaz (default branch `topaz`)
- Sweet: https://github.com/dipa-studio/ios-sweet (default branch `main`)

GitHub metadata confirms Topaz is a fork whose parent is Sweet. It is therefore a target-specific derivative, not an independent second implementation.

### Verified Topaz patch content
- `0080-topaz-cpu-topology-skip-sweet-hw.patch`
  - In `SweetCPU.cpp`, recognizes MPIDR Aff1 cluster / Aff0 core values: `0x0–0x3` A53 and `0x100–0x103` A73.
  - Names the CPU `Kryo 260`.
  - Gates inherited Sweet keys/touch/USB probes, documented as SM7150-specific MMIO, behind boot arg `sweethw=1`.
  - Do not call this patch merely a topology skip; inspect all hunks.
- `0081-topaz-armv8.0-no-lse-no-pan.patch`
  - Selects `-mcpu=cortex-a53` for the shared Sweet target build flags.
  - Replaces selected LSE CAS operations with LL/SC loops, adds selected user-mode LSE/LDAPR emulation, and removes the PAN-available define.
  - This is the highest-value comparative lead for checking whether Lavender emits/executes unsupported ARMv8.1+ instructions. It is not automatically safe to apply: inspect Lavender's actual build flags, instruction scan, feature registers and every relevant code path first.
- `0082-topaz-boot-on-gold-cpu.patch`
  - Requires logical boot CPU 0 to have physical MPIDR `0x100`; comments explain the shim boots XNU on Gold because Silver lacks 16K granule support.
  - This resembles Lavender's observed boot arrangement, but verify current device registers/ADT/topology rather than assuming equivalence.

### Porting rules
- Reuse algorithms, source-level techniques and diagnostic patterns only after review.
- Never copy SM6225/SM7150 MMIO addresses, memory maps, interrupt IDs, clocks, panel details, storage layout or DT/ADT nodes into Lavender without independent SDM660 evidence.
- Compare source diff, compiler flags, disassembly, feature detection and boot CPU selection before proposing a patch.
- Topaz and Sweet are related; do not count them as independent corroboration.
- The Topaz patches are comparative evidence, not yet applied to Lavender.

## Startup bootstrap diagnostic — Run #240

The exact linked kernel contains the trace helper and startup functions. Its `__BOOTDATA,__init_entry_set` section has 4,076 entries (97,824 bytes; 24 bytes/entry). The table was extracted from the Run #240 Mach-O and simulated with the XNU 8796 `bsd/kern/qsort.c` algorithm using the same `startup_entry_cmp`: 10,790 comparisons, max recursion depth 7. The sorted first phases contain 242 TUNABLES, 18 TIMEOUTS and 614 LOCKS callbacks (874 total before the cutoff).

All callback function pointers resolved inside executable kernel segments; all non-null arguments were within mapped kernel segments; the simulated sort output had no subsystem/rank-order violations. These are static/host-side checks, not a physical execution result. The trace's last line must identify whether the handset stalls in table validation, qsort, subsystem logging or a specific callback before any behavior change is justified.

## Key paths
- `tools/verify-lavender-kernel.py`
- `tools/mkpayload.py`, `tools/mkadt-lavender.py`, `tools/unpack_bootv2.py`, `tools/extract_appended_fdt.py`
- `xnu/patches/0106-lavender-verbose-boot-final.patch`
- `xnu/patches/0107-lavender-post-assembly-entry-trace.patch`
- `xnu/patches/0108-lavender-post-cpubootstrap-trace.patch`
- `shim/build.sh`
