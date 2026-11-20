# Chapter 05 — Native Lavender IOKit drivers

**Status date:** 20 November 2026  
**Branch:** `ci/lavender-machine-profile`  
**Baseline:** Run #394, source commit `312227c4da9f27c3bbecc72be06cdd149edda99f`  
**Scope:** native XNU 8796 driver architecture and staged implementation plan; no driver implementation is accepted by this chapter.

## Mission and porting rule

Implement Lavender peripheral support as **native XNU 8796 IOKit drivers**. Use the Android 15/Lucid OS hardware configuration and the public Lavender Linux 4.19 driver behavior as the hardware/behavior reference. Do not transplant Linux C code into Darwin. Re-express the behavior using XNU's IOKit service lifecycle, provider matching, workloops/gates, interrupt sources, memory mapping, DMA abstractions, power/resource management, and the relevant storage, HID, and display frameworks.

Do not use SWEET's SM7150/Kryo 470 device constants or present inherited SWEET touchscreen code as Lavender support.

The preceding unbranched-session handoff reports that a source audit began and supplies the paths below. This document records those paths as **candidate references supplied by that handoff**; it does not claim that every file, register field, sequence, or commit has been independently re-audited here. Before coding, check out and inspect the exact public Lavender kernel branch, the matching Android 15 device tree, Lucid OS configuration, and XNU 8796 APIs. Record exact source commits and line ranges in follow-up commits.

## Hardware targets reported by the handoff

| Component | Reported target configuration | Required outcome |
| --- | --- | --- |
| Qualcomm SDHCI v5 / eMMC 5.1 | Controller MMIO base `0x0c0c4000`; 8-bit eMMC interface; standard SDHCI host registers plus Qualcomm vendor register window | Controller init, command/data paths, DMA, IRQ handling, card identification and block reads |
| Qualcomm QUP I2C v2 | Controller MMIO base `0x0c175000`; touchscreen at I2C address `0x62` | Bus setup, FIFO/tagged transfers, repeated starts, NACK/error handling and bounded timeout recovery |
| Novatek NT36xxx touchscreen | TLMM GPIO 67 interrupt; TLMM GPIO 66 reset; NT36672A-family panel/touch assembly reported | Controller/firmware identification, reset sequencing, event decoding and ten-finger multitouch |
| Tianma NT36672A panel / Qualcomm display | 1080×2340, 60 Hz, four-lane MIPI DSI | MDSS/DSI host, PHY, clocks, reset/regulators and panel initialization sequence |

The reported controller base addresses and peripheral properties must be cross-checked against the actual bootloader DTB and Android 15 DT sources before MMIO access. Do not infer register-window lengths, interrupt IDs, clock names, regulator names, DMA masks, or reset polarity from the base address alone.

## Candidate Android/Linux 4.19 reference paths

The prior-session handoff identifies these paths in the public [Lavender-Gang kernel_xiaomi_lavender tree, branch 14.0](https://github.com/Lavender-Gang/kernel_xiaomi_lavender/tree/14.0):

| Component | Candidate reference path |
| --- | --- |
| Qualcomm SDHCI | `drivers/mmc/host/sdhci-msm.c` |
| Qualcomm QUP I2C v2 | `drivers/i2c/busses/i2c-msm-v2.c` |
| Novatek touchscreen | `drivers/input/touchscreen/nvt_touch_f7a/nt36xxx.c` |
| Tianma panel DT description | `arch/arm64/boot/dts/vendor/qcom/xiaomi/longcheer/common/display/dsi-panel-tianma-nt36672a-1080p-video.dtsi` |
| Qualcomm MDSS DSI panel path | `drivers/video/fbdev/msm/mdss_dsi_panel.c` |

Treat these as navigation hints until the exact branch contents and source revisions have been checked. The panel DTSI describes configuration; it is not itself a complete DSI host/PHY driver. Also inspect the associated MDSS/DSI host, PHY, clock, regulator, pinctrl/TLMM, and reset implementations and the relevant DTS includes.

## Dependency order — four components, not isolated drivers

1. **QUP I2C v2 transport.** This is the prerequisite for the Novatek controller. Implement controller discovery/matching, MMIO mapping, clocks/resets, interrupt registration, FIFO/tagged transaction state machine, repeated-start support, NACK/arbitration/error handling, bounded timeouts, and bus recovery. Expose a small native IOKit bus/provider interface; do not make the touchscreen driver manipulate QUP registers directly.
2. **SDHCI v5 / eMMC host.** Start with controller reset/bring-up, command and data transfer, interrupt completion, DMA mapping, card initialization and read-only block enumeration. Keep CQE and ICE disabled until their hardware paths are independently implemented and verified. No partition writes, formatting, filesystem writes, or automatic root-mount changes are authorized.
3. **Novatek NT36xxx touchscreen.** Depend on the QUP provider. Implement reset via TLMM GPIO 66, a real TLMM-to-XNU interrupt translation for GPIO 67, firmware/chip identification, report parsing, error/recovery paths, and ten-contact input publication through the appropriate native XNU HID/input path. Do not encode GPIO 67 as a generic GIC interrupt; a GPIO interrupt controller/domain must translate it correctly.
4. **Qualcomm MDSS/MIPI-DSI host + PHY and Tianma NT36672A panel.** This is a coupled display stack. Bring up power rails, clocks, resets, MDSS host, DSI PHY and lane configuration before sending the panel's command/video initialization sequence. The panel driver alone cannot drive the display without a working host/PHY/clock path. Validate the panel sequence from the actual DTSI and Linux driver, including delays, command mode/video mode, lane count, pixel format and timing.

## Native XNU implementation constraints

- Use XNU 8796 APIs and lifecycle contracts, not Linux `platform_driver`, `i2c_adapter`, `mmc_host`, `input_dev`, `drm_panel`, `devm_*`, Linux IRQ, or Linux DMA APIs.
- Each driver must have a narrow IOKit personality/match table grounded in the generated ADT properties. Check the exact property names and compatible strings in the Run #394 ADT before writing personalities.
- Map only verified MMIO windows and validate resource lengths/overflow before register access. Respect device-tree endianness and register access ordering.
- Use bounded polling and timeout paths; no unbounded waits in boot-critical paths. Log the failing register/state without flooding the early console.
- Register interrupts only after controller state, interrupt parent/translation and handler lifetime are valid. TLMM GPIO IRQs require a GPIO-domain implementation; do not route a raw GPIO number directly into GIC APIs.
- DMA must use XNU's supported DMA/memory-descriptor facilities, with explicit alignment, address-width, cache-coherency and lifetime handling. Do not assume Linux DMA/IOMMU behavior carries over.
- Resource sequencing (clocks, regulators, resets, pinctrl) must be derived from Lavender's DT and reference drivers. Every failure path must unwind acquired resources.
- Keep the display and touch paths independently diagnosable: the panel can be initialized before touch is attached, and a failed touch device must not prevent console/display bring-up.
- Keep `cpus=1` until secondary CPU startup and interrupt routing are separately validated.
- Physical tests remain RAM-only: `fastboot boot iosboot.img`. Never flash, erase, format, repartition, or write any Android partition.

## Per-driver acceptance gates

Each component must pass all of the following before being called implemented:

1. **Reference audit:** record source commit, exact DT node, compatible, resources, clocks, resets, IRQs, power sequence, and relevant Linux 4.19 code paths.
2. **XNU design:** identify the provider class, service lifecycle, match properties, resource ownership, workloop/interrupt strategy, DMA/memory mapping, and failure cleanup.
3. **Host tests:** add tests for property/resource parsing and pure protocol/state-machine logic, including malformed DT, timeout, NACK, reset, and error cases where applicable.
4. **Build guards:** add source verifiers and patch-application checks; compile XNU 8796 and inspect linked ARM64 output.
5. **Runtime instrumentation:** add concise per-stage markers to distinguish provider match, resource acquisition, controller reset, interrupt delivery, transaction completion, and device registration.
6. **Physical proof:** test the exact newly packaged image with recorded SHA-256 and RAM-only fastboot boot. Record the last confirmed milestone and never infer success from CI alone.

## Current status

- Run #394 passed XNU build, Lavender ADT validation, IOKit platform-match/early-init verification, and RAM-only package verification.
- The existing IOKit work is device-tree conversion, interrupt metadata validation/guarding, platform nub publication and service-matching instrumentation. It is not a set of working SDHCI, QUP, TLMM, Novatek, MDSS or panel drivers.
- **No implementation of the four native drivers is claimed by Run #394.** The next implementation task is QUP I2C v2, after verifying the exact reference source and XNU service/API integration. Keep eMMC, touch and display as separate dependent milestones.

## Next-session instruction

Read this chapter and Chapter 01 (device/safety), then inspect the current branch HEAD and latest Actions run. Verify the source paths and actual Lavender DT before changing code. Implement the QUP I2C provider first as a native XNU driver; add tests and source guards, build and package, then proceed to SDHCI/eMMC, Novatek touch, and MDSS/DSI plus Tianma panel. Do not implement all four as one unreviewable patch, do not copy Linux code, and do not claim hardware support before physical evidence.
