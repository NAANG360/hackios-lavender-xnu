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

## Memory aperture decision

XNU 8796's inspected `arm_vm_init()` consumes `boot_args.physBase` and `boot_args.memSize` as one contiguous physical aperture. Its physical-aperture setup then maps the remaining memory from `topOfKernelData` to the end of that aperture. It does not consume the four Linux RAM banks as a generic list.

For a first safe bring-up, `tools/mkadt-lavender.py` therefore selects only the fourth contiguous RAM bank and excludes the first reserved carve-out:

- Physical base: `0xa0400000`
- Safe bank end: `0xea948000`
- Total safe span: `0x4a548000`
- Reserve 1 MiB for the panic log at `0xea848000-0xea948000`
- XNU `dram-size`: `0x4a448000` (about 1.1575 GiB), ending at the panic-log reservation

The other three RAM windows remain unadvertised to XNU. This deliberately sacrifices memory capacity to avoid reserved holes and Android/firmware allocations in those banks. A multi-bank allocator patch can reclaim them later. The bootloader framebuffer is in the separate `0x95000000-0x9fffffff` window and is mapped separately from XNU-managed RAM.

## Public Lavender DT cross-check

The mainline Linux device tree is a useful independent reference, but it is **not** a replacement for the live vendor tree:

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

## Current Lavender work

- **0082** adds the provisional XNU machine profile: plain arm64, eight CPUs, two clusters/AMP and GICv3 identification.
- `tools/mkadt-lavender.py` builds a separate Apple Device Tree with the safe contiguous RAM aperture, the SDM660 GIC window, eight MPIDRs and the 1080 x 2340 framebuffer. Its `--check` mode validates those structures. It omits the optional iPhone10,3 `/product` node if `research/ipsw-x/d22ap.adt` is absent.
- The shim now has a separate `TARGET=lavender` path: SDM660 GICR address, the two-cluster MPIDR table, bootloader framebuffer mapping across both 32 MiB blocks, PSCI reset (no unverified restart-reason/PSHOLD writes), no SWEET UFS probe, and a distinct `LAVENDRX` payload marker. The SWEET marker and build path remain supported.
- `.github/workflows/lavender-shim.yml` cross-compiles the shim and validates the ADT. It produces a compile artifact only; it does not package a boot image or authorize flashing.

**Neither the XNU machine profile nor the shim compile is a bootable Lavender kernel yet.** The 16 KiB page-size setting is provisional and must be checked against `ID_AA64MMFR0_EL1` before hardware boot. XNU still needs a Lavender platform expert, timer integration, framebuffer console/IOFramebuffer provider and CPU registration. No eMMC, NVT touch or recovery-key driver is implemented.

## Hard blockers before a Lavender boot candidate

1. Confirm the provisional GICD/GICR/PSCI references against the exact live vendor DT, then implement timer IRQ routing, GIC security ownership and exception setup.
2. Port the XNU platform expert, boot CPU registration and framebuffer console; then build an IOMobileFramebuffer provider.
3. Implement eMMC/SDHCI at `c0c4000.sdhci`; do not reuse SWEET UFS.
4. Implement/verify PMIC/volume-key input, NVT touch, clocks/resets and I2C/SPMI from the actual vendor source.
5. Keep modem, ADSP/CDSP, secure/QSEE, WLAN and video firmware-dependent services explicitly unsupported until their interfaces are understood.

## Recovery-key requirement

Requested behaviour: **Volume Up + Volume Down together should reboot to Recovery**, without relying on the broken Power button.

Treat this as a dedicated recovery-path feature, not a normal XNU panic/restart. The key chord must be debounced and detected in a stage that can safely select the existing recovery image or set the platform's one-shot recovery boot request. Do not write misc/BCB or alter boot partitions until its exact bootloader-message format and a non-destructive recovery route are verified. This feature is not implemented.

## CI classification

A successful `MACHINE_CONFIG=SWEET` build proves only that the inherited XNU 8796 toolchain works. A successful `MACHINE_CONFIG=LAVENDER` compile proves only that the provisional machine profile compiles. A successful shim compile proves only that the standalone loader code compiles; it is not a boot image or a device boot. None of these artifacts should be flashed until the boot chain and runtime drivers are verified.
