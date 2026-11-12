# Lavender platform baseline — source-pinned hardware notes

**Target:** Xiaomi Redmi Note 7 / Redmi Note 7S family, device codename `lavender`  
**SoC:** Qualcomm SDM660  
**Evidence collected:** live Android report dated 2026-11-12 plus public downstream kernel DTS at commit `5ce3bc43631dca83f6081757c8982c2d5f6e017c`  
**Status:** source-grounded platform inventory; not a bootable XNU platform yet.

## 1. Identity and board selection

The live device reports:

- `ro.product.device=lavender`
- `ro.product.model=Redmi Note 7`
- `ro.board.platform=sdm660`
- DT model: `Qualcomm Technologies, Inc. SDM 660 PM660 + PM660L MTP F7A overlay`
- DT compatible: `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp`

The exact same model, compatible list, board ID `<0x20008 0>` and PMIC IDs are in `arch/arm/boot/dts/qcom/sdm660-mtp-lavender-overlay.dts` at the source revision above. The full tree `sdm660-mtp-lavender.dts` includes `xiaomi/lavender.dtsi`. Its include chain goes through `longcheer/common/longcheer-sdm660-base.dtsi`, `xiaomi-sdm660-common.dtsi` and `longcheer/lavender/lavender-base.dtsi`.

**Conclusion:** the live MTP F7A label is expected for this Lavender device-tree selection. It is not a reason to substitute a different SoC or board.

## 2. CPU topology and exception architecture

The pinned `sdm660.dtsi` describes eight ARMv8 CPUs arranged in two clusters of four:

- CPU logical nodes 0–3, cluster 0, `enable-method = "psci"`
- CPU logical nodes 4–7, cluster 1, `enable-method = "psci"`
- PSCI node: `arm,psci-1.0`, method `smc`
- GICv3 distributor: physical base `0x17a00000`, size `0x10000`
- GICv3 redistributor window: `0x17b00000`, size `0x100000`, stride `0x20000`
- Architected timer PPI declarations are the standard four timer entries in the Qualcomm DT. The live Linux interrupt table identifies `arch_timer` as GIC/MPM interrupt 19, consistent with the virtual timer PPI.

The downstream DTS has a lower-efficiency cluster and a higher-efficiency cluster; it does not provide Apple-compatible CPU implementation IDs. Keep the machine configuration generic ARMv8/Kryo until MIDR and feature handling are explicitly implemented. Do not copy the SWEET/Kryo 470/A76-A55 assumptions.

**XNU blocker:** the inherited SWEET CPU implementation registers only one CPU and explicitly rejects a topology count other than one. Lavender requires a proper PSCI secondary-CPU bring-up path before SMP is enabled. A safe first kernel milestone may intentionally remain boot-CPU-only, but must not advertise eight-core support.

## 3. Memory map

Live `/proc/iomem` reports four non-contiguous System RAM windows:

| Start | End | Approx. size |
|---|---|---:|
| `0x40000000` | `0x856fffff` | 1111 MiB |
| `0x88f00000` | `0x8abfffff` | 29 MiB |
| `0x95000000` | `0x9fffffff` | 176 MiB |
| `0xa0400000` | `0xfeabffff` | 1510.75 MiB |

The combined ranges are about 2.76 GiB. Linux reports MemTotal 2,803,968 KiB and CMA 278,528 KiB. The live reserved-memory node contains modem, ADSP, CDSP, PIL/MBA, QSEE, secure, WLAN MSA, splash, Venus, ramoops, DFPS and CMA reservations.

**Implementation rule:** do not model Lavender as one flat 3 GiB/4 GiB RAM bank. Reconstruct the physical banks and non-reusable carve-outs from the live DT values. The current report contains only names for most reserved-memory nodes; exact `reg`, `size`, `alignment` and `alloc-ranges` property bytes still need parsing from the last user-side report when it is made available. Until then, preserve the known Linux RAM windows as observations and do not invent carve-out sizes.

## 4. Storage — eMMC, not SWEET UFS

The live boot-device property is `c0c4000.sdhci`. Linux exposes:

- `mmc0` on `soc/c0c4000.sdhci` — internal, non-removable storage
- `mmc1` on `soc/c084000.sdhci` — secondary/removable SD host
- `mmcblk0` as the live internal block device

The Lavender board DTS configures `&sdhc_1` as non-removable and enables HS400/HS200/DDR 1.8 V modes; `&sdhc_2` is the removable SD slot. The generic MTP include contains UFS definitions, but the Lavender board-specific configuration and live boot path identify eMMC/SDHCI.

**Hard blocker:** the SWEET kernel's `sweet_ufs.c` root driver and shim `ufs.c` must not be carried over. Lavender needs a Qualcomm SDHCI/eMMC path, or an initial ramdisk-root boot path that does not claim internal-storage support.

## 5. Display and input hardware

Live kernel arguments identify panel `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`. The live input inventory reports:

- Active touchscreen name: `NVTCapacitiveTouchScreen`
- Resolution coordinates: X 0–1079, Y 0–2339
- Up to 10 multitouch slots
- I2C bus 1 exposes address `0x62`, and the Lavender F7A DTS describes `novatek@62` with compatible `novatek,NVT-ts`, IRQ GPIO 67 and reset GPIO 66.
- The same DTS comments identify the F7A Tianma NVT36672A module.

The report also lists I2C bus 1 address `0x20`; a separate Synaptics TCM description exists in the source tree, but the currently active Linux input device is NVT. Do not assume the 0x20 device is the active touchscreen.

GPU/MDSS and SMMU devices are visible in the live interrupt/resource tables. This proves Linux driver bindings exist; it does not provide an XNU Adreno/MDSS driver.

## 6. Volume-key recovery requirement

Requested product behaviour: **holding Volume Up and Volume Down together causes a reboot into the existing Recovery environment**, without using the broken Power button.

Live Linux input paths are split:

- Volume Up appears under `soc:gpio_keys`; the pinned SDM660 DTS binds it to PM660L GPIO 7, Linux key code 115.
- Volume Down appears under `qpnp_pon`, the PMIC power-on controller; its input event also exposes KEY_POWER.

Therefore, this chord cannot be implemented by watching only one GPIO input. A correct port needs both PMIC PON and TLMM/PM660L input paths, including debounce, simultaneous-state tracking and a bounded hold threshold.

Recommended staged design:

1. In the running XNU environment, add a small input-chord state machine that sees both real key sources, debounces them, handles either press order and requires a deliberate hold (suggested starting point: 800 ms).
2. On a confirmed chord, request a controlled restart through a Lavender-specific reboot service with a one-shot recovery reason.
3. In the trusted boot shim / boot selector, consume that one-shot reason and select the existing Recovery image.
4. Optionally detect the chord at early shim entry too, so it can work before XNU has started, but only after the exact PMIC PON state interface has been identified.

Do not copy SWEET's reset addresses or restart reason. Its shim uses `PSHOLD=0x0c264000`; Lavender's SDM660 DTS instead defines `qcom,pshold` at `0x10ac000` plus TCSR boot-misc detection at `0x1fd3000`. These are not interchangeable, and the port must not write them until the reset/reason semantics are reviewed. Do not write the `misc`/BCB partition until its exact bootloader-message structure and rollback route are verified. No physical reset or recovery tests while the Power button is broken.

## 7. Major SWEET-to-Lavender incompatibilities

The reference repository's existing port targets `sweet` / SM7150. Its machine config, platform expert, CPU class, framebuffer addresses, UFS root path, Goodix touch driver and reset path are device-specific. In particular:

- `SweetPlatformExpert` matches `xiaomi,sweet`; Lavender requires a separate target match.
- SWEET CPU startup is single-core only; Lavender has 8 CPUs.
- SWEET internal root is UFS; Lavender is eMMC on SDHCI1.
- SWEET framebuffer and early-console addresses cannot be reused.
- SWEET reset/PSHOLD address is not Lavender's.
- SWEET touch/power drivers do not correspond to Lavender's NVT + PMIC PON + GPIO keys.

Build results under `MACHINE_CONFIG=SWEET` are toolchain/reference results only. They are not Lavender kernels, boot images or evidence of physical compatibility.

## 8. Pinned external source locations

- SoC and board DTS: `xiaomi-sdm660/android_kernel_xiaomi_sdm660`, commit `5ce3bc43631dca83f6081757c8982c2d5f6e017c`
- Board file: `arch/arm/boot/dts/qcom/sdm660-mtp-lavender.dts`
- Overlay: `arch/arm/boot/dts/qcom/sdm660-mtp-lavender-overlay.dts`
- SoC: `arch/arm/boot/dts/qcom/sdm660.dtsi`
- Lavender chain: `arch/arm/boot/dts/qcom/xiaomi/lavender.dtsi`
- Board-specific overlay: `arch/arm/boot/dts/qcom/xiaomi/longcheer/lavender/lavender-base.dtsi`
- F7A NVT touch: `arch/arm/boot/dts/qcom/xiaomi/longcheer/lavender/sdm660-novatek-i2c_f7a.dtsi`

This document is a porting baseline, not a claim that any XNU image boots on Lavender.
