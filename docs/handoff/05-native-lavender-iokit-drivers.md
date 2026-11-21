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

### Run #400 backlog at acceptance time (superseded by the follow-up below)

1. Implement FIFO/block mode selection from the live `QUP_IO_MODES` register, including interrupt-driven block refill/drain and bounded completion; add host tests for the 68-byte Novatek envelope and malformed/timeout cases.
2. Implement TLMM pinctrl for QUP GPIO2/3 and safe bus-clear recovery; restore the active pinmux state on success and every failure path.
3. Implement or integrate the native `vdd_dig` vote/clock service rather than assuming the bootloader left the rail at a sufficient corner.
4. Add first-successful-transfer/error instrumentation and verify controller ID/status and an actual I2C transaction on hardware.
5. Only after QUP is validated, proceed to SDHCI/eMMC, then Novatek touch with the TLMM GPIO67 interrupt domain, then MDSS/DSI/Tianma.

## Current status — superseding the Run #400 next-step list

- Run #400 / artifact `14615937969` remains the latest independently verified XNU build, ARM64-linked kernel and RAM-only image. Its SHA-256 remains `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`.
- Native QUP follow-up source is now on `ci/lavender-machine-profile` through commit `b94e90b480cb51121776e663c5a369afe3dd56ce`. It is **not** yet a CI-accepted image or a physical-hardware milestone.
- The follow-up contains mode-specific FIFO/BLOCK counters, 68-byte input block draining with an exact four-byte tail, TLMM GPIO2=SDA/GPIO3=SCL active/sleep/bit-bang states, bounded nine-pulse bus recovery, a 100ms BUS_ACTIVE STOP fence, saved pre-reset failure status, 500–5000ms bounded transfer timeouts, runtime FIFO-geometry reporting and first-success/first-error diagnostics.
- Host tests and the source guard have been extended for the timeout clamp, bus-recovery eligibility, mode-specific counters, partial block drain, GPIO mux and failure-status ordering. Their execution and the XNU compile have not been confirmed for this follow-up commit.
- The provider is statically compiled into XNU and is an IOKit service, not a standalone loadable `.kext`. It matches the platform node and publishes `lavender-i2c-transfer` before registering. **No dependent Novatek IOKit client has yet been implemented**, so the endpoint has not yet been observed servicing a physical transfer.
- The confirmed-booting Android 15/Lucid ROM remains read-only. No ROM driver, module, boot image, DTB or Android configuration has been changed.

### QUP's remaining hard dependency: the real SDM660 RPM voltage vote

The pinned SDM660 DTS connects GCC `vdd_dig` to `pm660l_s3_level`. GCC assigns the QUP1 19.2MHz XO source to the LOWER digital corner (the pinned GCC voltage table maps LOWER to LOW_SVS). The RPM regulator uses the `rwcx` resource, ID 0, KVP key `vlvl`, with the requested voltage level sent over Qualcomm RPM messaging.

Lavender's SoC DTS provides `qcom,rpm-smd` / channel `rpm_requests`, RPM interrupt SPI 168, and an RPM GLINK transport through the APCS mailbox and shared RPM message RAM. The pinned `rpm-smd` client is an RPMSG client that sends acknowledged RPM resource requests; it is not a direct PMIC MMIO register. Therefore the XNU implementation needs the appropriate native shared-memory/mailbox/GLINK-RPM transport and acknowledged RPM request path before it can safely request LOW_SVS. Do not synthesize a PMIC register write or assume the previously booted Android system's regulator vote persists across a RAM-only fastboot boot.

### Acceptance and next engineering work

1. Finish and host-test the native RPM/GLINK dependency needed for the `vdd_dig` LOW_SVS request, or provide an independently verified always-on power guarantee before enabling QUP for clients.
2. Run the QUP protocol tests and source guard, then XNU 8796 compile, linked ARM64 verification, RAM-only packaging, manifest verification and SHA-256 verification on the new source head.
3. Only after those gates, physically validate a real transfer with `fastboot boot iosboot.img`, recording the image SHA-256 and QUP match/geometry/first-transfer logs.
4. Add the native Novatek client as a consumer of `lavender-i2c-transfer`; do not let it access QUP MMIO directly. Implement GPIO66 reset and the GPIO67 TLMM/MPM interrupt path separately.
5. Proceed to read-only SDHCI/eMMC and then the complete MDSS/DSI/PHY/Tianma chain. Preserve CQE/ICE disabled and do not claim these devices work until physical evidence exists.

Do not label this QUP follow-up accepted until its own CI and physical acceptance gates pass. Physical tests remain RAM-only; never flash, erase, format, repartition or write Android partitions.


## Working QUP follow-up after Run #400 — source only, not CI accepted

**Working source branch head at handoff update:** `4d7773d6acdd299ae5aaf07a9bf6908c3d827e17`. Run #400 remains the latest verified build/package baseline; the changes below must not be described as CI accepted until a new build and package run succeeds.

The confirmed-booting Android 15/Lucid ROM is an immutable reference for this task. No ROM driver, kernel module, boot image, DTB, vendor partition or Android configuration has been modified. The QUP implementation changes are confined to the native LIOS/XNU source tree, its host tests, source guards and this handoff.

### Additional QUP source changes

- QUP transfer counters now follow the hardware mode: FIFO uses `MX_READ_COUNT/MX_WRITE_COUNT`; BLOCK uses `MX_INPUT_COUNT/MX_OUTPUT_COUNT`. The Novatek report transport is 68 input bytes (66 controller report bytes plus the two-byte QUP tag), and selects BLOCK with the observed 16-byte block / 32-byte FIFO geometry when `IO_MODES=0`.
- The block reader now drains only `ceil(remainingBytes / 4)` FIFO words for each block. This matters for the 68-byte report's final four-byte tail; reading a complete 16-byte block at that point would over-consume the FIFO. Host tests cover the four 16-byte blocks plus one 4-byte tail, and reject invalid final-block sizes.
- Native TLMM handling maps the SOUTH tile at `0x03100000`, and programs GPIO2/GPIO3 using SDM660's per-pin control registers. Active pin state is `blsp_i2c1`, 2mA, no pull; sleep retains the I2C mux with a pull-up. Bit-bang recovery selects GPIO mode, pull-up, 2mA, input (released/open-drain) by default.
- The exact pinned BLSP1 QUP1 DTS source is `arch/arm64/boot/dts/vendor/qcom/sdm660-blsp.dtsi` at Linux commit `da9b19cb9ec452281cbe1270c5e733e20d69475a`: `qcom,i2c-dat = <&tlmm 2 ...>` and `qcom,i2c-clk = <&tlmm 3 ...>`. Thus **GPIO2 is SDA and GPIO3 is SCL**. The XNU recovery aliases and host regression tests encode this mapping. Do not reverse the lines.
- A bounded bus-recovery path now checks `BUS_ACTIVE`, `BUS_MASTER` and SDA state, switches to GPIO mode, releases both lines, emits at most nine SCL low/release pulses with a bounded clock-stretch wait, attempts a STOP condition, samples both lines before restoring the I2C mux, then checks the QUP bus-active status. The interrupt event source is disabled during recovery and re-enabled afterward. This is source implementation only; it has not run against the physical bus.
- Runtime checkpoints distinguish IOKit personality/provider match, controller/pin initialization and the first successful client transaction. The shared `lavender-i2c-transfer` resource is published before `registerService()`, so a dependent IOKit client can only match after the transfer endpoint exists.

### Actual service inclusion versus use

`LavenderQUPI2C.cpp` is statically compiled into the XNU kernel via `files.arm64`; the Lavender platform expert registers its IOKit personality; the service probes the `qcom,i2c-msm-v2` platform node and publishes its `callPlatformFunction` transfer endpoint. It is **not** a standalone dynamically loaded `.kext`.

This establishes kernel inclusion and IOKit registration in Run #400, but not a completed bus transaction: no Novatek client has yet been accepted or physically observed calling the transfer endpoint. The new first-transfer log is deliberately the runtime proof point for that next step.

### Still blocking QUP acceptance

- Native GCC `vdd_dig` corner voting is not implemented. The pinned `gcc-sdm660.c` source maps the 19.2MHz QUP1 source to the `LOWER` digital corner; `vdd_dig` is the `pm660l_s3_level` supply on the GCC node. Do not assume the ROM/bootloader's prior vote survives a RAM-only XNU boot.
- The controller's actual `IO_MODES` value and FIFO geometry still need on-device confirmation.
- The QUP GPIO recovery source path has not been built or run on hardware. No physical I2C transaction, Novatek identification or input event is claimed.
- A new XNU 8796 compile, linked ARM64 verification and RAM-only package for these follow-up commits have not yet been confirmed. Run #400 / artifact `14615937969` remains the last accepted image.

**Next:** verify the current QUP changes with host tests and the source guard, complete the native GCC `vdd_dig` vote through the correct SDM660 power-control path, then build/link/package. Keep the ROM read-only. Only after QUP is accepted and physically verified should the Novatek client begin consuming this endpoint; do not proceed to SDHCI/eMMC or display on the strength of source compilation alone.


## Native SDM660 RPM/GLINK rpm_requests — working source follow-up

**Status: source implementation staged in this branch; not yet XNU-build accepted or physically verified.** Run #400 remains the latest accepted image/build baseline. The Android 15/Lucid ROM is a read-only reference; this work changes only LIOS/XNU source and its build/test scaffolding.

### Confirmed transport topology

The pinned Lavender Linux tree at da9b19cb9ec452281cbe1270c5e733e20d69475a shows:

- qcom,glink-rpm is the GLINK transport node below /soc. It references rpm_msg_ram and mailbox channel zero on apcs_glb.
- The message RAM node is 0x00778000, size 0x7000. The 256-byte TOC is at the end of that region, magic 0x67727430; TX FIFO ID is ap2r (0x61703272) and RX FIFO ID is r2ap (0x72326170). Each descriptor is tail, head, then FIFO bytes.
- APCS global mailbox is 0x17911000; channel zero rings bit zero at offset 0x8 (0x17911008). The DTS also lists SPI 168. The Linux qcom_glink_rpm driver passes IRQ 0 into qcom_glink_native_probe, so this first XNU transport uses bounded polling rather than inventing an IRQ route.
- The RPM endpoint is opened dynamically by the remote as GLINK/RPMSG channel rpm_requests. The native implementation sends GLINK v1 VERSION, handles VERSION/ACK, accepts the remote OPEN, sends OPEN_ACK, sends its local OPEN request and waits for OPEN_ACK. RPM packets travel in intentless GLINK TX_DATA messages, matching the Linux qcom_glink_rpm setup (features=0, intentless=true).

### RPM-SMD v0 request for QUP's digital corner

The pinned qcom,rpm-smd node has no format-version register, so Linux defaults to RPM-SMD v0. The request builder mirrors rpm-smd.c's v0 header and KVP layout:

- 40-byte message; v0 request_len=32, data length 12.
- Message IDs 0 and 1 are reserved; the first request uses ID 2.
- Active set is 0.
- Resource type is the little-endian ASCII resource name rwcx (0x78637772), not the regulator-type enum; resource ID is 0.
- KVP key is little-endian ASCII vlvl (0x6c766c76), value size 4, value 64.
- GCC maps QUP1's 19.2 MHz XO source to VDD_DIG_LOWER; vdd-level-660.h maps LOWER to RPM_REGULATOR_LEVEL_LOW_SVS = 64.

The new LavenderRPMGLINK IOKit service validates the TOC, owns the two shared-memory rings, rings the APCS mailbox, opens rpm_requests, submits the v0 active-set vote and accepts only a matching successful RPM ACK. QUP now fails closed before GCC clock enable unless this LOW_SVS request is acknowledged. The active floor vote is retained; this first implementation deliberately does not send a zero/unvote that could lower a shared supply used by another client. A future multi-client regulator layer must aggregate the highest active corner before sending updates.

### Integration and acceptance gates

- The platform expert explicitly publishes only the /soc/rpm-glink nub, registers the RPM/GLINK personality before QUP, and files.arm64 compiles LavenderRPMGLINK.cpp.
- Patch 0130-lavender-rpm-glink-v0.patch and staging script 0130-lavender-rpm-glink-v0.sh copy the implementation into the XNU build tree.
- Host protocol tests cover TOC validation, ring wrap, GLINK OPEN/TX_DATA encoding, the 40-byte RPM v0 request and ACK decoding. A source guard verifies the nub, service, voltage-vote ordering and kernel inclusion.
- The transport is polled bring-up code, not a general-purpose GLINK framework. No physical remote OPEN, RPM ACK, voltage transition or QUP transaction is claimed yet. Run the new host tests, source guard, XNU compile/link, RAM-only package verification and then a physical fastboot boot iosboot.img test before calling it accepted.
- Do not change the confirmed-booting Android ROM, its modules, vendor drivers, DTB, boot image or partitions. Never flash or write Android partitions.

### Workflow queue note

The previous per-file commits created multiple workflow runs because the old concurrency group was unique per run. This source update is batched into one Git tree commit. The workflow is changed to keep one active build per branch and replace only older pending runs; it does not cancel an in-progress build. Existing older runs have not been cancelled, and the latest pre-existing run is left untouched. Run #400 remains the accepted baseline until the new RPM/GLINK workflow passes.
