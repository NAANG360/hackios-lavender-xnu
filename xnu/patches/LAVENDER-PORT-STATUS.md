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

## Published LucidOS Build 4 boot-image cross-check (2026-11-12)

The exact OTA ZIP from `NAANG360/Lucid-OS/ota/stable.json` was downloaded and verified (1,355,867,286 bytes; SHA-256 `d2b81388a57c685b82e3cb8f29da475f540dfcb9f5114003846d9337533d3219`). Its boot image carries a Linux `4.19.325-NYXION-SUSFS` kernel and three appended FDT blobs; the Lavender FDT identifies `qcom,sdm660-mtp` / `qcom,sdm660` / `qcom,mtp`.

Confirmed source values now reflected in the ADT builder:

- CPU MPIDR order follows the actual FDT and Linux topology: `0x0, 0x1, 0x2, 0x3, 0x100, 0x101, 0x102, 0x103`. The executing boot MPIDR still needs a shim readout before secondary startup is enabled.
- Framebuffer: `0x9d400000`, 1080×2340, stride 4320, `a8r8g8b8`, size `0x23ff000`.
- GICD/GICR: `0x17a00000` / `0x17b00000`; redistributor stride `0x20000`; timer INTID 19 is also reported by the live phone.
- eMMC SDHCI: compatible `qcom,sdhci-msm-v5,qcom,sdhci-msm-cqe`; register windows `0x0c0c4000/0x1000`, `0x0c0c5000/0x1000`, `0x0c0c8000/0x8000`; IRQ specifiers `<0 110 4>`, `<0 112 4>`; 8-bit non-removable; clocks `iface_clk,core_clk,ice_core_clk`.
- I²C MSM v2 controller at `0x0c175000`, 400 kHz bus, 19.2 MHz input clock. Novatek `novatek,NVT-ts` at address `0x62`, IRQ GPIO 67, reset GPIO 66; GPIO controller is SDM660 TLMM at `0x03000000`.
- Panel node is Tianma NT36672A, 1080×2340 at 60 Hz, DSI video/burst, 24 bpp, WLED backlight. Vendor includes QDCM calibration files for Tianma NT36672A, Shenchao NT36672A and BOE TD4320.
- Linux kernel config is ARM64 4 KiB pages, eight CPUs, GICv3, ARM timer, SDHCI MSM v5, I²C MSM v2, SDM660 pinctrl, NT36xxx touch, KGSL and KGSL IOMMU. This does not prove XNU's 16 KiB granule support; that remains a hardware-register check.

Vendor and ODM images were reconstructed from the OTA and passed read-only ext4 checks. No `.ko` files or module-load manifests were found in the vendor inventory. The vendor image contains the Adreno A512 ZAP firmware set and the panel calibration XML; ODM contains no separate touch/panel driver payload. Linux drivers and Android HALs are reference evidence, not reusable XNU binaries.

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
- The public mainline DTS does not supply the NVT touchscreen node. The Build 4 embedded FDT now identifies the vendor node as `novatek,NVT-ts` at I²C 0x62, with reset GPIO 66 and IRQ GPIO 67 through SDM660 TLMM. The XNU I²C/TLMM/NVT driver and firmware protocol are still unimplemented.
- The public SoC DT leaves the memory node for the bootloader to fill. The live four-bank memory map remains authoritative for the actual phone; do not collapse it into a fake contiguous 3 GiB range.

References:
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660-xiaomi-lavender.dts
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660.dtsi
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm630.dtsi
- XNU target source: https://github.com/apple-oss-distributions/xnu/tree/xnu-8796.141.3

## Current Lavender work

- **0082** adds the provisional XNU machine profile: plain arm64, eight CPUs, two clusters/AMP and GICv3 identification. **0083** sets the SDM660 64-byte I/D cache-line configuration. **0084** adds `LavenderPlatformExpert`, registers it for `xiaomi,lavender`, and publishes the CPU and platform device-tree nubs. These are source changes; successful compilation and runtime registration are not yet verified.
- `tools/mkadt-lavender.py` builds a separate Apple Device Tree with the safe contiguous RAM aperture, the SDM660 GIC window, eight MPIDRs and the 1080 x 2340 framebuffer. Its `--check` mode validates those structures. It omits the optional iPhone10,3 `/product` node if `research/ipsw-x/d22ap.adt` is absent.
- The shim now has a separate `TARGET=lavender` path: SDM660 GICR address, the two-cluster MPIDR table, bootloader framebuffer mapping across both 32 MiB blocks, PSCI reset (no unverified restart-reason/PSHOLD writes), no SWEET UFS probe, and a distinct `LAVENDRX` payload marker. The SWEET marker and build path remain supported.
- `.github/workflows/lavender-shim.yml` cross-compiles the shim and validates the ADT. It produces a compile artifact only; it does not package a boot image or authorize flashing.

**The current branch is not a bootable Lavender kernel yet.** The Build 4 Linux kernel uses 4 KiB pages; XNU's 16 KiB page-size setting remains provisional and must be checked against `ID_AA64MMFR0_EL1.TGran16` before hardware boot. `LavenderPlatformExpert` is now present in patch 0084, but its compilation and runtime registration are unverified. Remaining kernel work includes Lavender-specific GIC/timer integration, boot CPU registration, framebuffer console/IOFramebuffer provider, an eMMC/SDHCI driver using the now-known three register windows, I²C MSM v2/TLMM support, NVT touch and recovery-key input. No Lavender eMMC, NVT touch or recovery-key driver is implemented.

## Hard blockers before a Lavender boot candidate

1. Confirm the provisional GICD/GICR/PSCI references against the exact live vendor DT, then implement Lavender-specific timer IRQ routing, GIC security ownership and exception setup.
2. Compile and validate patch 0084, register the boot CPU, and connect the bootloader framebuffer to an IOMobileFramebuffer provider.
3. Implement eMMC/SDHCI at `c0c4000.sdhci`; do not reuse SWEET UFS.
4. Implement/verify PMIC/volume-key input, NVT touch, clocks/resets and I2C/SPMI from the actual vendor source.
5. Keep modem, ADSP/CDSP, secure/QSEE, WLAN and video firmware-dependent services explicitly unsupported until their interfaces are understood.

## Recovery-key requirement

Requested behaviour: **Volume Up + Volume Down together should reboot to Recovery**, without relying on the broken Power button.

Treat this as a dedicated recovery-path feature, not a normal XNU panic/restart. The key chord must be debounced and detected in a stage that can safely select the existing recovery image or set the platform's one-shot recovery boot request. Do not write misc/BCB or alter boot partitions until its exact bootloader-message format and a non-destructive recovery route are verified. This feature is not implemented.

## CI classification

A successful `MACHINE_CONFIG=SWEET` build proves only that the inherited XNU 8796 toolchain works. A successful `MACHINE_CONFIG=LAVENDER` compile proves only that the provisional machine profile compiles. A successful shim compile proves only that the standalone loader code compiles; it is not a boot image or a device boot. None of these artifacts should be flashed until the boot chain and runtime drivers are verified.
