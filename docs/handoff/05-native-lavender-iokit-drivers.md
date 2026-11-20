# Chapter 05 — Native Lavender IOKit drivers

**Status date:** 20 November 2026  
**Branch:** `ci/lavender-machine-profile`  
**Latest accepted source/build:** Run #400, source commit `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`  
**Latest accepted image:** `iosboot.img`, artifact `14615937969`, 16,621,568 bytes, SHA-256 `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`  
**Scope:** native XNU 8796 driver implementation and staged hardware validation. Run #400 accepts QUP source/build integration only; no physical peripheral operation is claimed.

## Mission and porting rule

Implement Lavender peripheral support as **native XNU 8796 IOKit drivers**. Use the Android 15/Lucid OS hardware configuration and the public Lavender Linux 4.19 driver behavior as the hardware/behavior reference. Do not transplant Linux C code into Darwin. Re-express the behavior using XNU's IOKit service lifecycle, provider matching, workloops/gates, interrupt sources, memory mapping, DMA abstractions, power/resource management, and the relevant storage, HID, and display frameworks.

Do not use SWEET's SM7150/Kryo 470 device constants or present inherited SWEET touchscreen code as Lavender support.

The public Lavender Linux 4.19 reference tree is pinned at commit `da9b19cb9ec452281cbe1270c5e733e20d69475a`. QUP I2C, Novatek NT36xxx, TLMM pinctrl/interrupt and MPM parent-domain paths have been inspected at that exact commit. The SDHCI/eMMC and MDSS/DSI/panel paths still require their own exact source audit before implementation. Keep source-level evidence, CI acceptance and physical hardware verification as separate claims.

## Pinned source audit and verified Lavender hardware contract

The public reference tree is pinned to `Lavender-Gang/kernel_xiaomi_lavender`, branch `14.0`, commit `da9b19cb9ec452281cbe1270c5e733e20d69475a`. The following QUP, Novatek, TLMM and MPM source paths were inspected at that exact commit. This is a Linux behavior/configuration reference; all implementation remains native XNU.

- QUP: `drivers/i2c/busses/i2c-msm-v2.c`.
- Novatek: `drivers/input/touchscreen/nvt_touch_f7a/nt36xxx.c`.
- TLMM: `drivers/pinctrl/qcom/pinctrl-sdm660.c`, `arch/arm64/boot/dts/vendor/qcom/sdm660-pinctrl.dtsi`.
- MPM parent domain: `drivers/irqchip/qcom-mpm.c`.
- Board touch node: `arch/arm64/boot/dts/vendor/qcom/xiaomi/longcheer/common/longcheer-sdm660-base.dtsi`.

The current Lavender ADT contract and source DTS establish:
- QUP1 base `0x0c175000`, window `0x600`, compatible `qcom,i2c-msm-v2`, GIC SPI 95, `iface_clk` / `core_clk`, 19.2 MHz input and 400 kHz bus target; `qcom,disable-dma` is set.
- Novatek I2C address `0x62`; touch IRQ is TLMM GPIO 67 and reset is GPIO 66. The DTS interrupt specifier is `<67 0x2008>` (level-low trigger flags); the GPIO specifier is not a GIC interrupt.
- QUP1 pinctrl uses GPIO2/GPIO3 with the `blsp_i2c1` mux function. SDM660 TLMM uses the SOUTH region offset `0x00100000`; the GPIO control registers are per-pin `0x1000` apart.
- The TLMM parent interrupt is routed through the Qualcomm wake-GIC/MPM domain (the TLMM parent specifier is SPI 208 in the referenced DTS). A correct XNU TLMM-to-MPM-to-GIC translation is required for GPIO67; do not substitute GIC SPI67 or treat GPIO67 as a direct GIC source.
- `nt36xxx.c` defines `POINT_DATA_LEN=65` and reads `POINT_DATA_LEN + 1 = 66` bytes. QUP's two-byte input tag makes the transport envelope 68 bytes. The Linux QUP driver chooses FIFO versus block mode based on the controller's `IO_MODES` geometry; block mode is required whenever the 68-byte envelope exceeds the actual FIFO capacity.

The SDHCI/eMMC and MDSS/DSI/panel paths listed below remain candidate references until their register, clock, reset, regulator and interrupt paths are audited at the same pinned source revision.

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

## Accepted QUP IOKit source/build milestone — Run #400

**Source commit:** `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`  
**Workflow:** [Run #400 / 44706182280](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280)  
**Image artifact:** `lavender-fastboot-boot-test`, ID `14615937969`  
**Image:** `iosboot.img`, 16,621,568 bytes  
**Image SHA-256:** `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`  
**Kernel SHA-256:** `24bfe515b6e6bfab70f53602a00384cdcc8f76fe3c7505916b7021fc6c33bb4e`

Run #400 passed both jobs. The XNU job passed the QUP protocol host regression, Lavender ADT checks, native QUP source guard, IOKit platform-match/early-init guards, XNU 8796 build, independent ARM64 Mach-O kernel verification and artifact upload. The downstream package job passed stock boot/DTB extraction, Lavender ADT generation, XNU payload construction, non-flashing boot-image construction, boot-image structure/payload identity verification and upload. The extracted image hash independently matches `SHA256SUMS` and `BUILD-MANIFEST.txt`. Manifest bootargs retain `cpus=1`.

Native implementation added under `xnu/patches/support/LavenderQUPI2C.h` and `LavenderQUPI2C.cpp`, integrated by the QUP patch/script and platform-expert registration, with host protocol tests and `tools/verify-lavender-qup.py`. The service matches `qcom,i2c-msm-v2`, maps the QUP resource, initializes the controller and XO clock path, attaches a workloop interrupt source, implements tagged FIFO transfers/repeated-start construction and NACK/arbitration/timeout reporting, and publishes the `lavender-i2c-transfer` provider interface for a dependent touch driver. Run #400 includes the lifecycle correction for retained workloop ownership and conditional event-source removal.

**Strict scope:** this is a native source/build/link/package milestone, not a functioning physical I2C bus. The current transfer path is FIFO-only. It does not yet implement QUP block mode, TLMM pinmux for GPIO2/3, bus-clear recovery, or the native `vdd_dig` vote required by the clock's voltage class. Since the Novatek read envelope is 68 bytes including the QUP input tag, block mode must be implemented or actual FIFO geometry must be independently proven sufficient before touch integration. The current driver log explicitly reports these remaining dependencies. No physical test has been performed.

### Next QUP work (do not move to touch yet)

1. Implement FIFO/block mode selection from the live `QUP_IO_MODES` register, including interrupt-driven block refill/drain and bounded completion; add host tests for the 68-byte Novatek envelope and malformed/timeout cases.
2. Implement TLMM pinctrl for QUP GPIO2/3 and safe bus-clear recovery; restore the active pinmux state on success and every failure path.
3. Implement or integrate the native `vdd_dig` vote/clock service rather than assuming the bootloader left the rail at a sufficient corner.
4. Add first-successful-transfer/error instrumentation and verify controller ID/status and an actual I2C transaction on hardware.
5. Only after QUP is validated, proceed to SDHCI/eMMC, then Novatek touch with the TLMM GPIO67 interrupt domain, then MDSS/DSI/Tianma.

## Current status

- Run #400 passed XNU 8796 build/source verification/kernel artifact verification and downstream RAM-only packaging. The accepted image and hashes are recorded above.
- The native QUP IOKit provider is now compiled, linked and source-guarded. This is not proof of QUP bus transactions or any physical peripheral operation.
- QUP remains incomplete: block mode, TLMM pinmux, bus-clear recovery and native `vdd_dig` vote are pending. The Novatek 68-byte tagged read must be supported before touch integration.
- SDHCI/eMMC, Novatek HID/input and MDSS/DSI/Tianma panel drivers are not yet implemented or accepted. GPIO67 must use the TLMM/MPM interrupt chain, never a direct GIC interrupt.

## Next-session instruction

Read this chapter and Chapter 01 (device/safety), then inspect the current branch HEAD and latest Actions runs. Continue QUP from Run #400: implement block mode, TLMM pinctrl, bus recovery and the `vdd_dig` clock vote; add tests/source guards, build and package, then physically validate via `fastboot boot iosboot.img`. Only after QUP is validated proceed to SDHCI/eMMC, Novatek touch and MDSS/DSI plus Tianma panel. Do not implement all four as one unreviewable patch, do not copy Linux code, and do not claim hardware support before physical evidence.
