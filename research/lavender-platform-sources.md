# Lavender platform source notes

## Scope

These are source-level references for the Redmi Note 7 (lavender / SDM660) port. They are not a replacement for the phone's live vendor DT, bootloader state, or the supplied hardware audit.

## Mainline device tree

Source: `arch/arm64/boot/dts/qcom/sdm660-xiaomi-lavender.dts`

- Model: Xiaomi Redmi Note 7.
- Compatible: `xiaomi,lavender`, `qcom,sdm660`.
- Bootloader-provided `simple-framebuffer`: base `0x9d400000`; width 1080; height 2340; stride 4320 bytes; `a8r8g8b8`.
- Reserved framebuffer region: `0x9d400000`, size `0x23ff000`.
- Volume Up: PM660L GPIO 7, active-low.
- Volume Down: PON resin input.

## SDM660 SoC tree

Source: `arch/arm64/boot/dts/qcom/sdm660.dtsi`, including `sdm630.dtsi`.

- GICv3 distributor: `0x17a00000`, size `0x10000`.
- GICv3 redistributor: `0x17b00000`, size `0x100000`, stride `0x20000` (eight frames).
- PSCI: version 1.0, conduit `smc`.
- CPU map: two clusters, four CPU interfaces each. The generic mainline CPU compatible strings are all `arm,cortex-a53`; treat that as DT metadata, not definitive core identification.
- The memory node is expected to be filled by the bootloader; it does not provide a complete Lavender bank/carve-out map.

## Cross-check against the live phone audit

The live phone report instead exposes a Qualcomm SDM660 PM660 + PM660L MTP F7A overlay, four physical memory windows, the eMMC controller at `c0c4000.sdhci`, a Tianma NT36672A display, and an NVT capacitive touchscreen. Preserve those differences: the public mainline DT is a useful reference for the framebuffer/GIC/PSCI/key declarations, but it does not prove the live vendor firmware uses every same address or power sequence.

## Actual LucidOS Build 4 ROM evidence (2026-11-12)

Source: the exact ZIP referenced by `NAANG360/Lucid-OS/ota/stable.json`:
`LucidOS-Android15-lavender-build-4-experimental.zip`
SHA-256: `d2b81388a57c685b82e3cb8f29da475f540dfcb9f5114003846d9337533d3219`.
The archive was downloaded in CI and its size, digest, and ZIP integrity were verified. Its `boot.img` contains a gzip-compressed Linux kernel and three appended FDT blobs; the third FDT identifies the SDM660 MTP/Lavender board (`qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp`).

### Kernel and CPU topology

- Kernel release string: `4.19.325-NYXION-SUSFS`; build date 2025-03-19; Clang 19.0.0 / LLD 19.0.0.
- The embedded `.config` is recoverable (`CONFIG_IKCONFIG=y`). It selects ARM64, `CONFIG_ARM64_PAGE_SHIFT=12` / `CONFIG_ARM64_4K_PAGES=y`, eight CPUs, GICv3, and the ARM architectural timer.
- Actual FDT CPU nodes are ordered `0x0, 0x1, 0x2, 0x3, 0x100, 0x101, 0x102, 0x103`. The live Linux topology report groups cpu0–cpu3 in package 0 and cpu4–cpu7 in package 1. The ADT builder previously put the `0x100` cluster first; it now follows the actual FDT/Linux order. Confirm the executing boot MPIDR in the shim before enabling secondary startup.
- The Linux 4 KiB page choice does **not** prove whether XNU's 16 KiB granule is usable. Keep the XNU setting provisional until `ID_AA64MMFR0_EL1.TGran16` is read on-device.

### Display, touch, storage and GPIO

- Bootloader framebuffer: `0x9d400000`, 1080×2340, stride 4320, `a8r8g8b8`, reserved size `0x23ff000`.
- Tianma panel node: `tianma nt36672a fhdplus video mode dsi panel`; 1080×2340, 60 Hz, DSI video/burst mode, 24 bpp, WLED backlight. The vendor partition contains QDCM calibration XML for Tianma NT36672A, Shenchao NT36672A, and BOE TD4320 variants.
- I²C controller: `qcom,i2c-msm-v2` at `0x0c175000`, size `0x600`, input clock 19.2 MHz, output 400 kHz, IRQ specifier `<0 95 4>`.
- Novatek touch node: `novatek,NVT-ts` at I²C address `0x62`; reset GPIO 66 and IRQ GPIO 67 through the SDM660 pinctrl/TLMM controller at `0x03000000`. The DT also contains a Synaptics TCM alternative at address `0x20`; the supplied phone audit identifies the active input device as `NVTCapacitiveTouchScreen`.
- eMMC: `qcom,sdhci-msm-v5,qcom,sdhci-msm-cqe` at `0x0c0c4000`; register windows `0x0c0c4000/0x1000`, `0x0c0c5000/0x1000`, and `0x0c0c8000/0x8000`; IRQ specifiers `<0 110 4>` and `<0 112 4>`; 8-bit, non-removable; clocks `iface_clk,core_clk,ice_core_clk`.
- GICD/GICR are `0x17a00000` / `0x17b00000`, with a `0x20000` redistributor stride. The live phone report identifies timer INTID 19.
- TLMM is `qcom,sdm660-pinctrl` at `0x03000000`, size `0xc00000`.

### Vendor/ODM partition findings

The OTA's vendor and ODM transfer streams were reconstructed as ext4 images and passed read-only `e2fsck` checks. No `.ko` files or module-load manifests were found in the vendor inventory; the relevant board drivers appear built into the boot kernel. Vendor contains the Adreno A512 firmware set (`a512_zap.elf/.mdt/.b00/.b01/.b02`), and the three panel calibration XML files listed above. The ODM partition is small and contains no separate touch/panel driver or firmware payload.

The kernel config confirms `CONFIG_I2C_MSM_V2=y`, `CONFIG_PINCTRL_SDM660=y`, `CONFIG_TOUCHSCREEN_NT36xxx=y`, `CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FORCE=y`, `CONFIG_MMC_SDHCI_MSM=y`, `CONFIG_QCOM_KGSL=y`, and `CONFIG_QCOM_KGSL_IOMMU=y`.

These are OS-artifact findings, not proof of successful physical boot of Build 4. Linux driver code and Android HALs are hardware references; they are not drop-in XNU drivers.

## Implementation consequences

1. Prefer bootloader framebuffer handoff for first pixels; validate base/stride/format before mapping.
2. Use GICD/GICR addresses as provisional source references only; validate against live DT before runtime.
3. Implement PSCI through SMC only after checking that the bootloader exposes the same conduit to non-secure EL1.
4. Do not reuse SWEET's timer INTID 19, Group-1 IRQ workaround, UFS driver, framebuffer address, or display geometry.
5. Build a separate NVT path from the vendor touchscreen driver and its I2C/pinctrl description.
6. Keep recovery-key chord handling in a boot stage that can safely select recovery; no BCB writes are authorized by this source note.

## Upstream files

- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660-xiaomi-lavender.dts
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm660.dtsi
- https://github.com/torvalds/linux/blob/master/arch/arm64/boot/dts/qcom/sdm630.dtsi
