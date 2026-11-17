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

Patch `0112-lavender-commpage-armv8a-llsc.patch` changes the two acquire-CASA operations to `LDAXR/STXR` retry loops while preserving the busy-lock return and the existing `STLR` unlock. The busy-lock branch now also executes `CLREX` before returning -1. This is defensive exclusive-monitor cleanup; it is not required for remote-CPU correctness, but is a sensible cold-path hygiene measure. Keep LSE disabled globally; explicit assembly sites must be corrected individually.

The new `tools/verify-lavender-commpage-llsc.py` guard checks the post-patch source and can independently inspect linked ARM64 disassembly. Workflow hardening is prepared to:
- parse the 0112 unified diff with `git apply --numstat` before the expensive build;
- execute the source guard immediately after the pinned build script's patch stage and before compilation;
- disassemble `__commpage_text` separately from `__text`, then require both linked helper symbols to contain `LDAXR/STXR`, `CLREX` and no LSE atomic mnemonic.

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


## Run 257 — scope of the commpage atomic guard

The first CLREX-enabled build reached the source-validation gate but failed because the workflow searched the entire `osfmk/arm/commpage/commpage_asm.s` file for executable `CASA`. That file contains other commpage routines outside patch 0112's scope. The guard is now scoped to `_pfz_trylock_and_enqueue` and `_pfz_trylock_and_dequeue`; within each function it requires no executable CASA, the expected LDAXR/STXR loop, and CLREX after the busy-lock label. Do not impose a file-wide CASA ban on unrelated routines.


## Userdata-root and Topaz storage/driver reference (16 November 2026)

The user has clarified the deployment model: install the iOS HFS+ root into the existing Lavender `userdata` partition, accepting loss of Android user data, while keeping Android boot/recovery and all non-userdata partitions intact. Android recovery's wipe/format-data path is the planned return route. The user confirms the volume-key reset/recovery route works; document and preserve the exact sequence before destructive operations.

Topaz (`MakrSas/ios-topaz`, branch `topaz`) is a direct iOS 16.7.16 (20H392) reference. Its README documents a journaled HFS+ root on `userdata`, a RAM-booted shim/XNU, BSD root selection via `rd=ufsroot0`, and separate `ufsrw=1 rootrw=1` write enablement. It also documents the launchd → backboardd → SpringBoard progression. **Topaz hardware is SM6225 / Snapdragon 685 with UFS 3.0**, whereas Lavender is SDM660 with eMMC behind SDHC1. Reuse the storage/root architecture, boot-argument separation, block-device registration and staged validation; do not copy UFSHCI/SCSI transport, UFS addresses, descriptors, DMA assumptions or Topaz DT/ADT values.

For Lavender, SDHC1 (`0x0c0c4000`, 8-bit, non-removable) is the candidate root transport. The first driver milestone remains a read-only, bounded probe and PIO read path; only after GPT/HFS+ reads and the recovery path are validated may write support be scoped to `userdata` alone. Do not write boot, vendor_boot, recovery, modem, persist, or any other eMMC partition. SDHC2 (`0x0c084000`) is removable media and is not the root target. See Chapter 05 for the detailed implementation plan.


### Topaz driver work — what transfers and what does not

The Topaz README and numbered patch map identify useful, bounded reference areas:
- **Wi-Fi/modem architecture:** patches 0113–0130 cover modem PAS boot through TrustZone, GLINK/QRTR, pd-mapper/tftp/rmtfs, WLFW, WMI/HTT, scanning, association and WPA2 data path. This is a reference for Qualcomm modem/WLAN sequencing and the iOS-facing `wifid` integration. Lavender's actual WCN3990/SDM660 firmware, memory carve-outs, IRQs, clocks, regulators and TLMM must be audited independently. Topaz's README explicitly says cellular data is not working; do not treat this as a completed LTE data implementation.
- **Display/cache:** patches 0092–0103 and 0111–0112 document direct scanout, double buffering and real cache maintenance; patches 0104–0110 cover brightness and display on/off. Reuse the cache-coherency and IOMobileFramebuffer service design, not Topaz's MDP/DSI addresses, panel geometry or voltage votes.
- **IOSurface/CoreAnimation:** patches 0131–0139 provide a useful reference for IOSurface value semantics, bulk attachments, client memory and release protocol. The iOS 20H392 userland is the same target, but Lavender's framebuffer/GPU path still needs its own hardware implementation.
- **Input:** Topaz patch 0083 is FocalTech FT5452 on QUP0 SE2. Lavender uses Novatek NT36xxx at I²C address 0x62; do not transplant the FocalTech driver or clock constants.
- **Battery/PMIC:** patch 0091 is Topaz's gauge/charger path. PMIC and gauge wiring are board-specific; use Lavender's PM660/PM660L FDT and downstream source instead.
- **GPU/audio/USB/SIM:** Topaz's Adreno 610 and its GPU path do not prove Adreno 512 compatibility. The README says audio and Bluetooth are not working on the iOS Topaz port; its roadmap points to separate topaz-woa audio, USB and SIM/LTE references, which require separate hardware validation before any Lavender port.

Do not copy patch numbers or source files as if they were drop-in Lavender patches. First map each reference to the corresponding Lavender FDT node, chipset/revision, firmware source, clocks, power, IRQ and memory constraints; then port one bounded subsystem at a time.

Topaz's `tools/mkiosroot.sh` is a useful image-layout reference: it builds one case-sensitive journaled HFS+ volume, copies the OS root plus SystemOS and AppOS cryptex trees under `/private/preboot/Cryptexes/{OS,App}`, and creates the dyld shared-cache lookup paths using hard links. This is a host-side image-construction pattern, not a Lavender storage driver. The companion `tools/sweet-root-tweaks.py` is **not** a drop-in: it removes CommCenter launch jobs because Topaz has no cellular data, disables VirtualAudio/recorder paths and marks Setup complete. Since Lavender's roadmap explicitly includes cellular and other Apple services, audit every tweak individually and do not remove CommCenter or disable services merely to imitate Topaz's current limitations.

## Lavender early-console source separation — 17 November 2026

- The inherited osfmk/arm64/sweet_early.c had accumulated the full Lavender implementation behind an ARM64_BOARD_CONFIG_LAVENDER branch. That made Sweet/SM7150 and Lavender hardware paths share one translation unit and blurred the target boundary.
- tools/split-lavender-early.py now runs immediately after the pinned XNU patch stack and before compilation. It extracts the Lavender branch into osfmk/arm64/lavender_early.c, renames exported symbols to the lavender_early_* namespace, creates lavender_early.h, leaves sweet_early.c SWEET-only, and adds the Lavender unit to osfmk/conf/files.arm64.
- sweet_early.h is retained as a compatibility facade for existing shared XNU patch call sites; under ARM64_BOARD_CONFIG_LAVENDER it maps legacy sweet_early_* entrypoints to lavender_early_* symbols. This avoids a risky mass rewrite of already-validated startup instrumentation while ensuring the linked Lavender implementation is no longer in the SWEET source file.
- Lavender-specific display values remain grounded in the Build 4 boot FDT: framebuffer PA 0x9d400000, 1080×2340, 4320-byte stride (1080 32-bit pixels), alias PA base 0x9c000000. The source keeps its 1080-pixel word stride and existing 20 MiB alias offset. No SWEET framebuffer base/geometry or SoC-specific GIC/timer assumptions are imported.
- Workflow cache keys include the generator; the build touches the generated translation unit; CI checks that the SWEET source has no Lavender branch, the new source exports the expected Lavender implementation, the compatibility facade redirects symbols, and files.arm64 compiles the new source.
- This is a source-layout/build integration change. It does not change the existing accepted Run #302 image or prove new handset behavior. Require a fresh successful XNU build, linked verification and RAM-only package before testing. Never flash.


## Lavender GICv3 timer/IRQ contract — 17 November 2026

- The generated Lavender ADT publishes `virtual-timer-intid = 19`. Xiaomi's SDM660 Linux device tree declares the architected ARMv8 timer as PPI index 3 (`<1 3 0xf08>`), i.e. INTID 16 + 3 = 19, and a GICv3 distributor/redistributor topology (GICD `0x17a00000`, GICR `0x17b00000`, redistributor stride `0x20000`). The generated boot ADT remains the runtime source for the Lavender timer PPI.
- The shim hands control to XNU in non-secure EL1. Arm IHI 0069G specifies that in the non-secure `GICD_CTLR` view, `ARE_NS` is bit 4; when set, bit 1 (`EnableGrp1A`) aliases secure-view `EnableGrp1NS`; bit 0 is RES0. Do not set or demand readback of bit 0 in that state.
- Patch 0124 creates a Lavender-only Group-1 NS path: read and validate the ADT timer PPI; set only that PPI's `GICR_IGROUPR0` group bit, priority and enable; configure `ICC_BPR1_EL1`; require firmware's `ARE_NS`; set/read back `GICD_CTLR_ENABLEGRP1A`; bound the distributor RWP wait; enable and verify `ICC_IGRPEN1_EL1`; and keep combined EOI mode.
- `sleh_irq()` acknowledges Group-1 using `ICC_IAR1_EL1`, services PPI 19 through `rtclock_intr(TRUE)`, then writes `ICC_EOIR1_EL1`. No Lavender SGI/IPI route is enabled. `cpus=1` remains mandatory until secondary-core startup and GIC routing are separately verified.
- The previous generic fallback used a Group-0/PPI-27 configuration, while the Lavender timer node advertises PPI 19; Lavender also lacked a matching IAR1/EOIR1 timer dispatcher. Patch 0124 addresses this definite later mismatch. It is **not** claimed as the cause of the current event-87 stop, which occurs before the new post-map trace reaches timer configuration.
- CI now runs `tools/verify-lavender-gic-route.py` to enforce the ADT-to-GIC-to-XNU timer contract. This guard and a green build prove source/build consistency, not physical IRQ delivery.

References: [Xiaomi SDM660 device tree](https://github.com/xiaomi-sdm660/android_kernel_xiaomi_sdm660/blob/11-EAS/arch/arm/boot/dts/qcom/sdm660.dtsi), [Arm GIC architecture specification, IHI 0069G](https://www.scs.stanford.edu/~zyedidia/docs/arm/gic_v3.pdf).