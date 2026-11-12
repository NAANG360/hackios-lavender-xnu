# Lavender target notes — XNU port

This fork is a separate XNU 8796 build-workbench. The upstream port targets Redmi Note 10 Pro (sweet / SM7150); its SWEET machine configuration and UFS, PM6150L and Goodix-specific code are **reference material only**, not a Lavender kernel.

## Target observed from the live Android report (2026-11-12)

- Device: Redmi Note 7, `lavender`
- SoC platform: Qualcomm SDM660
- Linux-visible memory: 2,803,968 KiB (about 2.67 GiB)
- RAM windows from `/proc/iomem`: 0x40000000-0x856fffff, 0x88f00000-0x8abfffff, 0x95000000-0x9fffffff, 0xa0400000-0xfeabffff
- CMA: 272 MiB
- Boot storage controller: `c0c4000.sdhci`; `mmcblk0` is present (eMMC path)
- Display: `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`, 1080 x 2340
- Touch input: `NVTCapacitiveTouchScreen`, 1080 x 2340 coordinate range, 10 multitouch slots
- Live tree model string: `Qualcomm Technologies, Inc. SDM 660 PM660 + PM660L MTP F7A overlay`
- Live tree compatibles: `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp`
- Interrupt report identifies GICv3/MPM-GIC controllers; eight CPUs are exposed

## Public Lavender DT cross-check

The mainline Linux device tree is a useful independent reference, but it is **not** a substitute for the live vendor tree:

- `sdm660-xiaomi-lavender.dts` identifies Xiaomi Redmi Note 7 and declares a bootloader-provided simple framebuffer at physical `0x9d400000`, 1080 x 2340, 32-bit stride 4320, format `a8r8g8b8`. It reserves a 0x23ff000 framebuffer region. This is the preferred first display path: consume the bootloader framebuffer rather than bring up MDSS panel power immediately. Verify the actual base at boot before mapping it.
- `sdm630.dtsi` (included by `sdm660.dtsi`) declares GICv3 GICD at `0x17a00000` (0x10000 bytes), GICR at `0x17b00000` (0x100000 bytes), redistributor stride 0x20000, and eight redistributor frames.
- The public SoC DT declares PSCI 1.0 with the `smc` conduit.
- Its CPU map contains two clusters, each with four CPU interfaces. The public DT labels all eight interfaces `arm,cortex-a53`; do not treat those generic compatible strings as definitive Kryo core identification.
- The public handset DT maps Volume Up to PM660L GPIO 7 and Volume Down to the PON resin input. This is a reference for the requested simultaneous-key recovery path, not proof of the bootloader's recovery-message protocol.
- The public mainline DTS does not supply the NVT touchscreen node/driver needed by this device. The live report confirms the NVT input device; the exact vendor I2C client, address, reset line, IRQ trigger and firmware protocol still need source-level verification.
- The public SoC DT leaves the memory node for the bootloader to fill. The live four-bank memory map remains authoritative for the actual phone; do not collapse it into a fake contiguous 3 GiB range.

References:
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660-xiaomi-lavender.dts
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660.dtsi
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm630.dtsi
- XNU target source: https://github.com/apple-oss-distributions/xnu/tree/xnu-8796.141.3

## Initial machine profile (0082)

The Lavender-specific patch adds a distinct `LAVENDER` machine configuration, generic plain-arm64 code generation, an eight-CPU/two-cluster AMP profile, and GICv3 compile-time identification. The workflow defaults to `machine_config=LAVENDER` and runs a preflight guard against accidental SWEET driver reuse.

**This is a compile-time profile scaffold, not a bootable Lavender kernel.** The 16 KiB XNU page-size setting is provisional for the iOS 16 userspace target and must be checked against `ID_AA64MMFR0_EL1` before any hardware boot attempt. No Lavender timer INTID/routing, GIC security-group setup, multi-bank allocator map, SDHCI, framebuffer driver, PMIC, NVT touch, or recovery-key driver is implemented by 0082.

## Hard blockers before a Lavender boot candidate

1. Verify 16 KiB translation granule support and implement Lavender MMU/page-table setup.
2. Model non-contiguous DDR banks and Qualcomm reserved-memory carve-outs. XNU's boot_args memory contract must be reconciled with the four observed banks; do not invent a flat memory range.
3. Implement the eMMC/SDHCI path at `c0c4000.sdhci`; do not reuse SWEET UFS.
4. Reconcile the public GICD/GICR/PSCI reference with the exact live vendor DT, then implement timer PPI/IRQ routing, GIC security ownership, PSCI and exception setup.
5. Reuse the bootloader simple framebuffer first if its physical address and format are confirmed; implement the Lavender MDSS/panel driver only if the handoff cannot be retained.
6. Implement/verify PMIC/volume-key input, NVT touch, clocks/resets and I2C/SPMI from the actual vendor source.
7. Keep modem, ADSP/CDSP, secure/QSEE, WLAN and video firmware-dependent services explicitly unsupported until their interfaces are understood.

## Recovery-key requirement

Requested behaviour: **Volume Up + Volume Down together should reboot to Recovery**, without relying on the broken Power button.

Treat this as a dedicated recovery-path feature, not a normal XNU panic/restart. The key chord must be debounced and detected in a stage that can safely select the existing recovery image or set the platform's one-shot recovery boot request. Do not write misc/BCB or alter boot partitions until its exact bootloader-message format and a non-destructive recovery route are verified. This feature is not implemented by the reference build.

## CI classification

A successful `MACHINE_CONFIG=SWEET` build proves only that the inherited XNU 8796 toolchain works. A successful `MACHINE_CONFIG=LAVENDER` compile proves only that the provisional Lavender machine profile compiles; neither is a bootable image or permission to flash. A dedicated Lavender runtime patch series is still required.
