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
- C marker encoding: `0x100 | stage`; publish before framebuffer output; `dc cvac` then `dsb sy`.
- Apple mark is deferred until VM initialization and permanent framebuffer alias setup.

## Current CI / packaging facts
- `.github/workflows/xnu.yml`: patch application, source guards, XNU build, Mach-O/instruction verification, artifacts and cache.
- `.github/workflows/package-lavender.yml`: manual package workflow; requires a successful XNU run, binds the kernel artifact to source SHA, extracts stock SDM660 boot/DTB, generates Lavender ADT/payload and makes a non-flashing image.
- Run #236 (ID `43701501661`, source `17a68fb...`) build and package jobs passed. Artifact IDs and physical status are in Chapter 03.
- Packaging is not automatic in general. Verify the exact run/source SHA, artifact, `iosboot.img` filename, manifest, checksums, DTB/ADT, payload identity, load ranges and structure.
- Cache restore may reuse objects; source files changed by patches must be invalidated/touched. Verify logs show the expected actual build path. Never weaken checks or assume a cache hit is valid.
- Do not overlap Actions builds.

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

## Key paths
- `tools/verify-lavender-kernel.py`
- `tools/mkpayload.py`, `tools/mkadt-lavender.py`, `tools/unpack_bootv2.py`, `tools/extract_appended_fdt.py`
- `xnu/patches/0106-lavender-verbose-boot-final.patch`
- `xnu/patches/0107-lavender-post-assembly-entry-trace.patch`
- `xnu/patches/0108-lavender-post-cpubootstrap-trace.patch`
- `shim/build.sh`
