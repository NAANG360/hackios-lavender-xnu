# Lavender target notes — XNU port

This fork is being used as a separate XNU 8796 build-workbench. The upstream port is for Redmi Note 10 Pro (sweet / SM7150); its SWEET machine configuration and UFS, PM6150L and Goodix-specific code are **reference material only** and are not a Lavender kernel.

## Target observed from live Android report (2026-11-12)

- Device: Redmi Note 7, `lavender`
- SoC platform: Qualcomm SDM660
- Linux-visible memory: 2,803,968 KiB (about 2.67 GiB)
- RAM windows from `/proc/iomem`: 0x40000000-0x856fffff, 0x88f00000-0x8abfffff, 0x95000000-0x9fffffff, 0xa0400000-0xfeabffff
- CMA: 272 MiB
- Boot storage controller: `c0c4000.sdhci`; `mmcblk0` is present (eMMC path)
- Display boot argument: `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`
- Touch input: `NVTCapacitiveTouchScreen`, 1080 x 2340 coordinate range, 10 multitouch slots
- Live tree model string: `Qualcomm Technologies, Inc. SDM 660 PM660 + PM660L MTP F7A overlay`
- Live tree compatibles: `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp`
- Interrupt report identifies GICv3 and MPM-GIC controllers; Linux reports eight CPUs
- CPU topology is heterogeneous; Lavender profile reserves two CPU clusters

The live vendor tree says MTP F7A. This is not, by itself, proof of a device mismatch; compare it with the Lavender-specific downstream board description before selecting the final Apple Device Tree.

## Initial machine profile (0082)

The first Lavender-specific patch adds a distinct `LAVENDER` machine configuration, plain arm64 code generation, an eight-CPU/two-cluster profile, and GICv3 compile-time identification. The workflow now defaults to `machine_config=LAVENDER` and runs a preflight guard against accidental SWEET driver reuse.

**This is a compile-time profile scaffold, not a bootable Lavender kernel.** The 16 KiB XNU page-size setting is provisional for the iOS 16 userspace target and must be checked against `ID_AA64MMFR0_EL1` before any hardware boot attempt. No Lavender GIC interrupt routing, timer INTID, redistributor base, RAM allocator map, SDHCI, framebuffer, PMIC, NVT touch, or recovery-key driver is implemented by this patch.

## Hard blockers before a Lavender boot candidate

1. Verify 16 KiB translation granule support and implement Lavender MMU/page-table setup.
2. Model non-contiguous DDR banks and Qualcomm reserved-memory carve-outs. Do not replace them with a flat 3 GiB or 4 GiB range.
3. Implement the eMMC/SDHCI path at `c0c4000.sdhci`; do not reuse the SWEET UFS driver.
4. Identify GIC distributor/redistributor bases, security-group ownership, timer PPI/IRQ routing, PSCI conduit and exception setup from the exact Lavender DT/board sources.
5. Implement/verify Lavender display, framebuffer, PMIC/volume-key input, NVT touch, clocks/resets and I2C/SPMI controllers from the live DT and pinned board sources.
6. Keep modem, ADSP/CDSP, secure/QSEE, WLAN and video firmware-dependent services explicitly unsupported until their interfaces are understood.

## Recovery-key requirement

Requested behaviour: **Volume Up + Volume Down together should reboot to Recovery**, without relying on the broken Power button.

Treat this as a dedicated recovery-path feature, not a normal XNU panic/restart. The key chord must be debounced and detected in a stage that can safely select the existing recovery image or set the platform's one-shot recovery boot request. Do not write the misc/BCB partition or alter boot partitions until its exact bootloader-message format and a non-destructive recovery route are verified. The feature is not implemented by this reference build.

## CI classification

A successful build with `MACHINE_CONFIG=SWEET` proves only that the inherited XNU 8796 toolchain/build pipeline works. A successful `MACHINE_CONFIG=LAVENDER` compile proves only that the provisional Lavender machine profile compiles; neither is a bootable image or permission to flash. A dedicated Lavender runtime patch series is still required.
