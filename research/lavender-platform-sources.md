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
