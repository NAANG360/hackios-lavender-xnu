# Chapter 2 — Architecture, build and packaging

## Acceptance gates (never conflate)
1. Source and required patches committed/applied.
2. XNU builds for `DEVELOPMENT ARM64 LAVENDER`.
3. ARM64 Mach-O identity/structure verified.
4. Actual compiled stage-8 and stage-9 instructions verified.
5. RAM-only package built and structurally verified as exact `iosboot.img`.
6. Physical boot stages observed.
7. Apple logo observed.
8. Darwin userspace / SpringBoard reached.

A green compile, shim graphic, source grep, QEMU run or package does not prove a later gate.

## Current implementation
- XNU 8796 target: `DEVELOPMENT ARM64 LAVENDER`.
- `0106-lavender-verbose-boot-final.patch`: early framebuffer/boot diagnostics.
- `0107-lavender-post-assembly-entry-trace.patch`: assembly stages 8/9 before `arm_init()`, C markers and cache-clean publication ordering.
- Stage 8 follows boot-args KVA calculation; stage 9 immediately precedes `ret` to `arm_init()`.
- Marker namespace: assembly 3–9; C entry `0x100`; C progress `0x100 | stage`; terminal marker uses high-bit namespace. Publish before framebuffer output; `dc cvac` then `dsb sy`.
- Apple logo intentionally waits until `arm_vm_init` and permanent framebuffer alias setup.

## Workflows
- `.github/workflows/xnu.yml`: build, patch/source guards, Mach-O + instruction verification, diagnostics and cache.
- Cache key is versioned XNU 8796 v2. A non-exact/legacy cache may reuse intermediate objects but must invalidate completed kernel output. Confirm logs show actual `make install` / XNU build path; never infer speedup. Previously runner used 4 CPUs (`XNU_MAKE_JOBS=4`).
- `.github/workflows/package-lavender.yml`: manual dispatch; requires successful XNU run ID. It binds kernel artifact to source SHA, extracts stock boot/SDM660 DTB, builds Lavender ADT + LAVENDRX + non-flashing shim, then creates `iosboot.img`.
- Packaging is not automatic. Package only the exact kernel accepted by verifier; DTB/ADT/payload must match that run's manifest.
- Verify package filename, kernel/payload identity, DTB/ADT, load ranges, structure and SHA-256 independently.

## Key paths
- `tools/verify-lavender-kernel.py` — current verifier blocker.
- `tools/mkpayload.py`, `tools/mkadt-lavender.py`, `tools/unpack_bootv2.py`, `tools/extract_appended_fdt.py`.
- `xnu/patches/0106-lavender-verbose-boot-final.patch`
- `xnu/patches/0107-lavender-post-assembly-entry-trace.patch`
- `shim/build.sh`
