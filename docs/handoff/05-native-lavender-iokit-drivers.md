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
- APCS global mailbox is 0x17911000; channel zero rings bit zero at offset 0x8 (0x17911008). The DTS also lists SPI 168. In the pinned Linux source, the zero passed by qcom_glink_rpm to qcom_glink_native_probe is the GLINK feature mask (features=0), not an IRQ number; the native layer obtains IRQ index 0 from the qcom,glink-rpm device-tree node and registers it. The first XNU bring-up deliberately uses bounded polling and does not yet register SPI 168 through an XNU interrupt domain.
- The RPM endpoint is opened dynamically by the remote as GLINK/RPMSG channel rpm_requests. The native implementation sends GLINK v1 VERSION, handles VERSION/ACK, accepts the remote OPEN, sends OPEN_ACK, sends its local OPEN request and waits for OPEN_ACK. RPM packets travel in intentless GLINK TX_DATA messages, matching the Linux qcom_glink_rpm setup (features=0, intentless=true).

### RPM-SMD v0 request for QUP's digital corner

The pinned qcom,rpm-smd node has no format-version register, so Linux defaults to RPM-SMD v0. The request builder mirrors rpm-smd.c's v0 header and KVP layout:

- 40-byte message; v0 request_len=32, data length 12.
- Message IDs 0 and 1 are reserved; the first request uses ID 2.
- Active set is 0.
- Resource type is the little-endian ASCII resource name rwcx (0x78637772), not the regulator-type enum; the vdd_dig resource ID is 0.
- The regulator DTS has qcom,set=3 (active and sleep handles). Linux sends a separate sleep-set request only when its aggregate differs from the active request; with this single XNU client holding the same LOW_SVS floor, the active-set request remains in effect during RPM-assisted collapse. A future multi-client/sleep-aware regulator layer must add a separate sleep vote if its aggregate differs.
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

The previous per-file commits created multiple workflow runs because the old concurrency group was unique per run. This source implementation is batched into an atomic Git tree commit. The workflow now uses one concurrency group per branch with cancel-in-progress enabled, so a newer source commit supersedes older runs created under the new group. Legacy runs created under the former per-run groups cannot be cancelled by this concurrency change; the latest pre-existing run and accepted Run #400 baseline are left untouched. Run #400 remains the accepted baseline until the new RPM/GLINK workflow passes.


## RPM/GLINK build result and queue checkpoint — 21 November 2026, 15:10 IST

**Latest source:** `a74f05bfcfc82becccb24a9cb2189db462a3fb57` (parent implementation commit: `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`). The implementation is present in the branch, but Run `44871413803` failed at the final kernel link. This supersedes any earlier wording that described the RPM/GLINK build as merely pending.

### What Run 44871413803 actually verified

- Lavender QUP host protocol regression: **passed**.
- Lavender RPM/GLINK host protocol regression: **passed**.
- Lavender ADT memory/CPU topology validation and preceding build setup: **passed**.
- The QUP and platform-expert translation units compiled. The RPM/GLINK translation unit did **not** appear in the IOKit build object list.
- XNU link failed with undefined symbol `_lavender_rpm_vote_vdd_dig_low_svs`, referenced from `LavenderQUPI2C::start(IOService*)`.
- The decisive earlier log line is that `0130-lavender-rpm-glink-v0.patch` was skipped as malformed: `patch fragment without header at line 12: @@ -15,4 +16,5 @@`. Because that patch is what adds `LavenderRPMGLINK.cpp` to `iokit/conf/files.arm64` and adds its registration call/nub publication to `LavenderPlatformExpert.cpp`, the RPM implementation never became a kernel object. The undefined symbol is therefore an integration/staging failure, not evidence that the GLINK or RPM protocol code itself failed to compile.
- The RAM-only image packaging job was skipped. No new `iosboot.img`, linked kernel, RPM ACK or physical voltage vote was produced by this run.
- Diagnostic artifacts were retained: `xnu-build-log` artifact ID `14696531915` (ZIP SHA-256 `f06f15dc59a6903bd33f66f7a350157cbfb0dd356cf725d10f519feca6b582b4`) and `xnu-buildtree` artifact ID `14696800920` (ZIP SHA-256 `26bc07525eadafb32119b02df171f91402fbfb28371a9bfb09cc7771b2da2fb4`). The build-tree artifact is diagnostic only, not a kernel image.

### RPM/GLINK implementation now present in source

The native implementation is committed, not merely planned:

- `LavenderRPMGLINK.cpp/.h` implement the SDM660 RPM message-RAM TOC/ring path, APCS mailbox doorbell, GLINK v1 negotiation, remote-created `rpm_requests` channel OPEN/OPEN_ACK, intentless TX_DATA, RPM-SMD v0 request and matching ACK handling.
- The first request is the QUP1 digital-corner vote: resource name `rwcx`, `vdd_dig` resource ID 0, KVP `vlvl`, LOW_SVS value 64.
- QUP clock enable is gated on the acknowledged vote. The RPM transport uses bounded polling; it has not been physically exercised.
- The follow-up commit `a74f05b` names the resource constant explicitly as `LAVENDER_RPM_RESOURCE_ID_VDD_DIG` and records the active/sleep-set semantics. With the one XNU client holding the same LOW_SVS floor, no separate sleep request is added unless the aggregate sleep vote differs from active.

### Queue/concurrency checkpoint

The workflow now uses one branch concurrency group, `xnu-${{ github.ref }}`, with `cancel-in-progress: true`. This supersedes runs created under the new group when a newer commit arrives. The branch message reports these outcomes:

| Run | Outcome |
|---|---|
| `44871035770` | Cancelled |
| `44871146744` | Cancelled |
| `44871215312` | Cancelled |
| `44868695080` | Preserved legacy run |
| `44706182280` (Run #400) | Successful; accepted baseline |
| `44871413803` | Completed with link failure |

Legacy runs created under the former per-run concurrency groups are not automatically covered by the new branch group. Do not claim they were cancelled unless their individual status is checked. No manual cancellation capability is assumed.

### Exact next steps

1. Repair `0130-lavender-rpm-glink-v0.patch` as a valid unified diff. The malformed hunk header at line 12 is the first concrete failure to fix. Regenerate the patch from the exact pinned XNU base or use a reliable, idempotent staging script for the platform-expert and `files.arm64` edits; do not hand-edit hunk counts.
2. Add a build-time guard that fails before link if the staged `iokit/Kernel/arm/LavenderRPMGLINK.cpp` is absent or if `LavenderRPMGLINK.cpo` is absent from the IOKit object list. Also verify the platform-expert registration and `/soc/rpm-glink` nub publication survived patch application.
3. Verify the C-linkage declaration and definition for `lavender_rpm_vote_vdd_dig_low_svs`, and verify the object is in the final kernel link file list. Do not paper over this with a weak stub or by removing QUP's fail-closed voltage-vote gate.
4. Run the RPM/GLINK host tests and source guard, then one coherent XNU compile/link/package workflow. Avoid per-file commits and repeated build triggers; preserve the Run #400 artifact.
5. Only after a linked ARM64 kernel and RAM-only image pass all checks should the physical test begin. Record actual remote OPEN, matching RPM ACK, voltage-vote and QUP initialization evidence. No physical success is currently claimed.

The confirmed-booting Android 15/Lucid ROM remains read-only. No ROM driver, kernel module, DTB, boot image, vendor partition or Android configuration was changed. Physical validation must remain RAM-only via `fastboot boot iosboot.img`; never flash or write Android partitions.


## Verified CI evidence addendum — Run 44871413803

The run is complete, not pending. The actual job records and decoded build log were inspected on 21 November 2026.

- Job `build` (ID `136746553887`): **failure**.
- Job `Package RAM-only Lavender fastboot image` (ID `136764738517`): **skipped**.
- QUP host protocol regression: **passed**.
- RPM/GLINK host protocol regression: **passed**.
- XNU 8796 compilation reached the final link stage, then failed because `_lavender_rpm_vote_vdd_dig_low_svs` was undefined from `LavenderQUPI2C::start(IOService*)`.
- The log explicitly reports that patch `0130-lavender-rpm-glink-v0.patch` was skipped as malformed: `patch fragment without header at line 12: @@ -15,4 +16,5 @@`. This prevented the RPM implementation's `files.arm64` inclusion and platform-expert registration changes from reaching the build tree. The RPM translation unit was consequently absent from the IOKit object list.
- This is an integration/staging failure. It does **not** establish that the RPM/GLINK implementation compiles, links, opens the remote channel, receives an RPM ACK, or changes voltage.
- Diagnostic artifacts only: `xnu-build-log`, artifact `14696531915`, ZIP SHA-256 `f06f15dc59a6903bd33f66f7a350157cbfb0dd356cf725d10f519feca6b582b4`; `xnu-buildtree`, artifact `14696800920`, ZIP SHA-256 `26bc07525eadafb32119b02df171f91402fbfb28371a9bfb09cc7771b2da2fb4`. Neither is a bootable kernel/image.
- Run #400 (`44706182280`, artifact `14615937969`) remains the latest accepted build and RAM-only image. Preserve it unchanged.

### Revised next action

Repair the malformed staging patch or replace it with a deterministic, idempotent staging script. Before another build is allowed, add hard pre-link checks that prove all three conditions: (1) `LavenderRPMGLINK.cpp` exists in the staged XNU tree, (2) `LavenderRPMGLINK.cpo` is present in the IOKit object/file list, and (3) the platform expert registers the RPM service and publishes the `/soc/rpm-glink` nub. Verify the C-linkage declaration and definition of `lavender_rpm_vote_vdd_dig_low_svs` and that the object reaches the final link list. Do not use a weak stub, remove QUP's fail-closed clock-vote gate, or change the confirmed-booting Android ROM.

Keep the repair and guards in one coherent source update; do not create per-file commits or trigger a build flood. After reviewing the patch application and guards, run one coherent CI workflow. Only a successful linked ARM64 kernel plus verified RAM-only package can advance to physical testing.


## Branch queue and RPM/GLINK status reconciliation — 21 November 2026, 19:55 IST

This section reconciles the supplied branch-status message with the live GitHub Actions records. The branch message called Run `44871413803` the current queued build, but GitHub now reports that run as **completed / failure**. Do not repeat the stale “build pending” wording.

### Verified run states

The following run records were queried directly:

| Run ID | Head commit | Current result |
|---|---|---|
| `44871035770` | `ecb3bd3` | <text>Cancelled</text> |
| `44871146744` | `a34368a` | Cancelled |
| `44871215312` | `0170148` | Cancelled |
| `44868695080` | `b94e90b` | Completed / failure (preserved legacy run) |
| `44706182280` (Run #400) | `7a15aaad` | Successful; accepted baseline |
| `44871413803` | `a74f05b` | Completed / failure; not queued |

The branch-wide concurrency group is `xnu-${{ github.ref }}` with `cancel-in-progress: true`. It cancels superseded runs that share this new branch group. Legacy runs created under the former per-run group are not automatically included; do not claim cancellation for any legacy run without checking its individual record. The available GitHub connection has no manual cancellation action, so no manual cancellation is claimed.

### RPM/GLINK implementation state

The native `LavenderRPMGLINK` source is present at the latest RPM implementation commit `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`, with follow-up commit `a74f05bfcfc82becccb24a9cb2189db462a3fb57`. The follow-up names the vdd_dig resource ID explicitly and documents active/sleep vote semantics.

The source implements the SDM660 RPM message-RAM TOC/ring transport, APCS mailbox doorbell, GLINK v1 negotiation, remote-initiated `rpm_requests` OPEN/OPEN_ACK, intentless TX_DATA, RPM-SMD v0 request encoding and ACK validation, and the `rwcx` / resource-ID-0 / `vlvl=64` LOW_SVS active vote. QUP clock enable remains fail-closed until a matching successful RPM ACK is received. This is a native XNU implementation and uses bounded polling; it is not a transplanted Linux driver.

The first implementation retains the active LOW_SVS floor. Because the regulator DTS exposes active and sleep sets, the implementation follows Linux's aggregate-vote behavior: it does not send a separate sleep vote when the single client's sleep aggregate does not differ from its active LOW_SVS vote. Future multi-client regulator management must calculate the aggregate before introducing sleep votes or unvotes.

### Run 44871413803 — actual outcome

Run `44871413803` used commit `a74f05bfcfc82becccb24a9cb2189db462a3fb57` and is **completed / failure**:

- QUP host protocol tests: passed.
- RPM/GLINK host protocol tests: passed.
- XNU compile reached final link; link failed because `_lavender_rpm_vote_vdd_dig_low_svs` was undefined from `LavenderQUPI2C::start(IOService*)`.
- The log says `0130-lavender-rpm-glink-v0.patch` was rejected as malformed at line 12: `patch fragment without header at line 12: @@ -15,4 +16,5 @@`.
- Therefore `LavenderRPMGLINK.cpp` was not staged into `iokit/conf/files.arm64`, its object was absent from the IOKit link list, and its platform-expert registration / RPM nub publication did not reach the staged XNU tree.
- RAM-only package job was skipped. No new bootable `iosboot.img`, successful remote OPEN, RPM ACK, physical voltage transition or QUP transaction resulted from this run.
- Diagnostic-only artifacts: `xnu-build-log` ID `14696531915` (ZIP SHA-256 `f06f15dc59a6903bd33f66f7a350157cbfb0dd356cf725d10f519feca6b582b4`) and `xnu-buildtree` ID `14696800920` (ZIP SHA-256 `26bc07525eadafb32119b02df171f91402fbfb28371a9bfb09cc7771b2da2fb4`). Neither is a bootable image.

This is a patch application / build integration failure. It is not evidence that the RPM/GLINK C++ implementation itself compiled, linked or communicated with the remote processor.

### Required next action — one coherent source change, then one build

1. Repair `0130-lavender-rpm-glink-v0.patch` as a valid unified diff generated from the exact pinned XNU base, or replace it with a deterministic, idempotent staging script. Do not hand-edit hunk counts.
2. Add hard pre-link checks proving all of the following: staged `iokit/Kernel/arm/LavenderRPMGLINK.cpp` exists; `LavenderRPMGLINK.cpo` appears in the IOKit object/file list; the platform expert registers the RPM service and publishes the `/soc/rpm-glink` nub; and the C-linkage declaration, definition and final link-list entry for `lavender_rpm_vote_vdd_dig_low_svs` all match.
3. Keep QUP's acknowledged-vote clock-enable gate. Do not add a weak stub, remove fail-closed behavior, bypass the RPM ACK, or write PMIC registers directly.
4. Review the patch application and guards first. Then make one coherent source update and trigger only one build workflow. Do not create per-file commits or another run flood. Preserve Run #400 and its accepted artifact unchanged.
5. After successful host tests, source guards, XNU compile/link and RAM-only package verification, inspect the image manifest and SHA-256. Only then attempt a physical RAM-only `fastboot boot iosboot.img` test and record actual remote OPEN, matching RPM ACK, voltage vote and QUP initialization evidence.

The confirmed-booting Android 15/Lucid ROM remains strictly read-only: no ROM drivers, kernel modules, DTBs, boot images, vendor files or partitions may be changed. No physical RPM or touchscreen functionality is claimed until device evidence exists.


## RPM/GLINK implementation and Actions queue handoff — 21 November 2026, 19:55 IST

This section supersedes earlier wording that the RPM/GLINK build was still pending. The supplied branch message identifies the implementation and queue policy; GitHub job records independently confirm that Run 44871413803 completed with a link failure.

### Source checkpoint

- Repository: `NAANG360/hackios-lavender-xnu`
- Branch: `ci/lavender-machine-profile`
- RPM/GLINK implementation commit: `ecb3bd3fa0c1b36dbd9ee0875665f24d295a7c30`
- Latest source commit: `a74f05bfcfc82becccb24a9cb2189db462a3fb57`
- The latest source commit names the `vdd_dig` resource ID explicitly and documents active/sleep regulator-set semantics.
- The confirmed-booting Android 15/Lucid ROM remains read-only. No ROM drivers, modules, DTBs, boot images, vendor partitions or Android configuration were changed.

### Native RPM/GLINK implementation

The implementation is a dedicated native XNU RPM service, not Linux code transplanted into Darwin and not a general-purpose GLINK framework:

1. XNU `LavenderRPMGLINK` IOKit service.
2. SDM660 RPM shared message RAM, TOC and circular FIFO rings.
3. APCS mailbox doorbell at `0x17911008`, bit 0.
4. GLINK v1 negotiation and remote-initiated `rpm_requests` OPEN / OPEN_ACK.
5. RPM-SMD v0 request and matching ACK.
6. First supported request: `rwcx` resource, `vdd_dig` resource ID 0, `vlvl=64` (LOW_SVS).

The implementation records RPM message RAM base/size as `0x778000` / `0x7000` and currently uses bounded polling. QUP clock enable is gated on a matching successful RPM ACK. If the vote cannot be acknowledged, QUP must fail closed.

The regulator DTS exposes active and sleep sets. With this single XNU client retaining the same LOW_SVS floor, no separate sleep-set request is sent unless the aggregate sleep vote differs from the active vote. Do not add a speculative unvote or independent per-driver voltage writes; future clients require an aggregate regulator-vote layer.

### Actions queue policy and reported cleanup

The workflow now uses one concurrency group per branch with `cancel-in-progress: true`. The supplied branch message reports:

| Run | Reported outcome |
|---|---|
| `44871035770` | Cancelled |
| `44871146744` | Cancelled |
| `44871215312` | Cancelled |
| `44868695080` | Preserved legacy run |
| `44706182280` — Run #400 | Successful; accepted baseline |
| `44871413803` | Completed; link failure |

The old per-run concurrency group is not covered by the new branch group. Do not claim any legacy run was cancelled unless its individual status is confirmed. The available GitHub connection does not expose manual cancellation. Avoid per-file commits and unnecessary build triggers; batch related fixes into one coherent source change.

### Run 44871413803 — verified result

The run is complete, not pending. Its `build` job failed and `Package RAM-only Lavender fastboot image` was skipped.

- QUP host protocol regression passed.
- RPM/GLINK host protocol regression passed.
- XNU reached final link, but the linker reported undefined `_lavender_rpm_vote_vdd_dig_low_svs`, referenced by `LavenderQUPI2C::start(IOService*)`.
- The build log says `0130-lavender-rpm-glink-v0.patch` was skipped as malformed at line 12: `patch fragment without header at line 12: @@ -15,4 +16,5 @@`.
- Therefore the RPM implementation translation unit was not included in the IOKit object list, and its platform-expert registration / nub-publication edits did not reach the staged XNU tree. This is an integration/staging failure. It does not prove the RPM/GLINK translation unit compiles or links, nor that GLINK opens, RPM acknowledges, or voltage changes.
- Diagnostic-only artifacts: `xnu-build-log` ID `14696531915`, ZIP SHA-256 `f06f15dc59a6903bd33f66f7a350157cbfb0dd356cf725d10f519feca6b582b4`; `xnu-buildtree` ID `14696800920`, ZIP SHA-256 `26bc07525eadafb32119b02df171f91402fbfb28371a9bfb09cc7771b2da2fb4`. Neither is a bootable kernel/image.
- Run #400 (`44706182280`, artifact `14615937969`) remains the latest accepted XNU build and RAM-only image. Preserve it unchanged.

### Mandatory next engineering task

1. Repair `0130-lavender-rpm-glink-v0.patch` as a valid unified diff generated against the exact pinned XNU base, or replace fragile hunk edits with a deterministic, idempotent staging script. Do not hand-edit hunk counts.
2. Add hard pre-link checks that fail if staged `iokit/Kernel/arm/LavenderRPMGLINK.cpp` is missing, if `LavenderRPMGLINK.cpo` is absent from the IOKit object/file list, or if platform-expert registration and `/soc/rpm-glink` nub publication are missing.
3. Verify the C-linkage declaration and definition of `lavender_rpm_vote_vdd_dig_low_svs`, and confirm its implementation object is in the final kernel link inputs. Do not add a weak stub, remove QUP's fail-closed vote gate, or bypass RPM ACK.
4. Run the RPM/GLINK host test and source guard after the coherent repair. Inspect staged source and object list before starting exactly one new XNU build/link/package workflow.
5. Accept only after the ARM64 kernel links, the RAM-only package is produced, and manifest/checksum verification passes. Physical validation must record remote OPEN, matching RPM ACK, voltage-vote evidence and QUP initialization.
6. Only after QUP has an acknowledged vote and a physical transaction should the Novatek client consume the transfer service.

Never flash or write Android partitions. Physical tests remain RAM-only using `fastboot boot iosboot.img`.


## RPM/GLINK patch repair and build guards — 21 November 2026

The live branch had advanced to `6eebcfabdbe65e76c1c4863c6181e4cad36ab902` (documentation-only commit) when this repair was prepared. The source baseline remains the native RPM implementation at `a74f05bfcfc82becccb24a9cb2189db462a3fb57`.

Run `44871413803` failed because patch 0130's first hunk count was wrong (declared 5/6, actual 6/7), causing the parser to treat the next hunk header as body text. The `processTopLevel` hunk also declared 4 old lines but contained 6. The repaired diff narrows the first hunk to 3/4 and corrects the later hunk to 6/19. The RPM host-test runner now independently recounts every unified-diff hunk and rejects malformed headers/body counts.

The staged-source guard verifies the driver, RPM registration, `/soc/rpm-glink` nub publication, IOKit source-list inclusion, the QUP acknowledged-vote ordering, and matching C-linkage declaration/definition for `lavender_rpm_vote_vdd_dig_low_svs`. It is invoked in the build script after patch/staging and before XNU compilation. The workflow additionally audits for `LavenderRPMGLINK.cpo` and the vote symbol in the captured build/link transcript.

The object/link audit is a build artifact check, not physical proof. Acceptance still requires successful XNU 8796 compilation and ARM64 link, linked Mach-O verification, RAM-only image packaging, and manifest/SHA-256 verification. Only after those gates may a physical test use `fastboot boot iosboot.img`; never flash or write Android partitions. No RPM OPEN, ACK, voltage vote, QUP transaction or touchscreen functionality is claimed.


## RPM/GLINK staging repair — verified repository checkpoint (21 November 2026)

### Live branch and repair commit

The branch was checked directly before editing. The supplied status identified `6eebcfabdbe65e76c1c4863c6181e4cad36ab902` as HEAD; that commit is the parent of the repair. The current live branch HEAD is now:

- Repository: `NAANG360/hackios-lavender-xnu`
- Branch: `ci/lavender-machine-profile`
- Repair HEAD: `e280830dc978dc6b32c677bfcb88715a1b36670e`
- Parent: `6eebcfabdbe65e76c1c4863c6181e4cad36ab902`
- Commit: [`e280830`](https://github.com/NAANG360/hackios-lavender-xnu/commit/e280830dc978dc6b32c677bfcb88715a1b36670e) — `fix(lavender): repair RPM GLINK staging and link guards`

The commit updates six files together: `0130-lavender-rpm-glink-v0.patch`, `tools/test-lavender-rpm-glink.py`, `tools/verify-lavender-rpm-glink.py`, `.github/workflows/xnu.yml`, `AGENTS.md`, and this chapter.

### Patch accounting and integration guards

The original Run 448714 failure came from incorrect unified-diff hunk accounting. The repair narrows the PlatformExpert include hunk from the malformed 5/6 declaration to 3/4, corrects the `processTopLevel` hunk from 4/19 to 6/19, and retains the other reviewed hunk counts. The updated host-test runner independently recounts declared versus actual old/new hunk lines.

The repair adds:

- Matching C-linkage declaration/definition verification for `lavender_rpm_vote_vdd_dig_low_svs`.
- A staged-source guard invoked after patch/staging and before XNU compilation.
- Checks for RPM service registration, `/soc/rpm-glink` nub publication and IOKit source-list inclusion.
- A post-build audit for `LavenderRPMGLINK.cpo` and the voltage-vote symbol in the captured build/link transcript.

**Important limitation:** the object audit is post-build. A genuine check immediately before the final linker invocation has not yet been established. Do not call this a completed pre-link object guard. The source guard before compilation does not by itself prove that the generated object is present in the final link inputs.

### Validation status — do not overstate

| Gate | Current result |
|---|---|
| Repair commit pushed | Passed — `e280830dc978dc6b32c677bfcb88715a1b36670e` |
| Patch hunk accounting | Verified by the repair review; runner now checks counts independently |
| Previously existing Run 448714 QUP and RPM host tests | Passed on the pre-repair source; they do not validate the new runner/repair |
| Updated host protocol test suites | **Not rerun** after this repair |
| Fresh exact-base patch application/staged-tree audit | **Not yet verified by a new workflow** |
| New GitHub Actions run | **Not triggered / no new run observed** |
| New XNU ARM64 link | **Not verified** |
| New RAM-only `iosboot.img` | **Not produced** |
| Physical RPM OPEN, RPM ACK, voltage transition, QUP transaction | **Not tested** |

Run #400 ([workflow 44706182280](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280), artifact `14615937969`) remains the latest accepted kernel/image. Preserve it unchanged. Run 448714 is still a failed run; its diagnostic artifacts are not bootable. The repair commit is an integration repair, not acceptance of a new RPM-enabled kernel.

### Next engineering sequence

1. Establish a genuine pre-link object-presence hook if the XNU build system exposes a safe point; otherwise make the strongest deterministic staged-object/link-input check available and describe its exact timing honestly.
2. Run both QUP and RPM/GLINK host protocol regression suites on the repaired branch.
3. Apply patch 0130 against the exact pinned/staged XNU source and inspect the resulting source tree, `files.arm64`, generated object list and platform-expert registration/nub publication.
4. Trigger exactly one XNU workflow after all local/host guards pass. Avoid per-file commits and build floods.
5. Accept only after XNU 8796 compiles, the ARM64 Mach-O link includes the RPM implementation and vote symbol, independent verification passes, the RAM-only image is packaged, and manifest/SHA-256 agree.
6. Only then perform physical testing by `fastboot boot iosboot.img`. Record actual remote GLINK OPEN/OPEN_ACK, matching RPM-SMD ACK, voltage-vote evidence, QUP initialization and a successful I2C transaction. Never flash or write partitions.

### Display and panel safety boundary

The current RPM/GLINK stage is not a display implementation. The `vdd_dig` vote and QUP clock gate do not directly configure MDSS/MDP, DSI host/PHY, Tianma panel initialization, display timings, brightness or backlight. Therefore this stage has no expected direct path to screen damage. This is an engineering risk assessment, not a guarantee that all hardware faults are impossible.

Keep display bring-up isolated until RPM/QUP are independently validated. When MDSS/DSI/panel work begins, derive rails, reset order, DSI commands and timings from the confirmed-working Lavender ROM/kernel sources and hardware limits; do not guess voltages or timings. Use bounded waits, explicit failure handling and conservative power sequencing. A black screen or failed panel initialization may be recoverable and is not, by itself, evidence of permanent panel damage. Do not enable experimental display sequences during RPM/QUP validation.

### ROM preservation and physical-test rules

- The Android 15/Lucid OS ROM is confirmed working and remains strictly read-only.
- Do not modify its drivers, kernel modules, DTBs, boot images, vendor files/configuration or any Android partition.
- No physical test is authorized by this source-only repair. Wait for full build/link/package acceptance.
- Subsequent handset testing is RAM-only via `fastboot boot iosboot.img`; never use `fastboot flash`, erase, format, repartition or write any partition. Preserve the independent volume-key recovery route and `cpus=1` boot argument.

### Context boundary for the next session

For this active RPM/GLINK-to-QUP milestone, the next session should read only `AGENTS.md` and this Chapter 05. Do not load `PROJECT_HANDOFF.md` or Chapters 01–04 unless the user explicitly expands the scope. This chapter contains the active driver architecture, current repair status, acceptance gates and hardware-safety boundary.


## RPM/GLINK pre-link hardening and exact staged-source validation — 21 November 2026

- Patch 0130 is regenerated from the exact post-0129 XNU staged files recovered from failed Run 448714's build-tree artifact. Its source-list and PlatformExpert hunks pass `git apply --check` against those files; no hunk counts were hand-edited.
- The RPM TOC parser rejects duplicate RX/TX IDs and overlapping descriptor/FIFO regions before writing shared message RAM. Host regressions cover both malformed cases.
- Both QUP and RPM/GLINK protocol host suites pass in a local rebuild of the fetched C++ test sources. The RPM host suite also exercises the pre-link verifier with valid, wrong-symbol, missing-object, duplicate-object and PRE_LTO fixtures, plus idempotent MakeInc.kernel injection.
- A new build-tool injection adds checks to XNU's actual `makedefs/MakeInc.kernel` link recipes. In the active non-LTO workflow, it runs immediately after `link.filelist` is generated and immediately before the `$(LD)` command. A PRE_LTO hook checks both split lists before that link path as well. The verifier requires exactly one `LavenderRPMGLINK.cpo` entry, confirms the object exists, and uses the selected Xcode `nm` to prove a strong global text definition of `lavender_rpm_vote_vdd_dig_low_svs`.
- The existing post-build audit remains as a second, later check; it is not being represented as the pre-link check.
- This validates the patch, host test logic and hook placement, not the actual ARM64 kernel build. The new source has not yet been compiled by XNU and no new linked kernel or image exists. Run #400 remains the accepted baseline. No physical GLINK OPEN, RPM ACK, voltage transition or QUP transaction is claimed.
- The Android 15/Lucid OS ROM remains read-only. Physical testing remains blocked until the full XNU/link/Mach-O/RAM-only package/manifest/checksum acceptance sequence passes; then use only `fastboot boot iosboot.img`, never flash or write an Android partition.


## RPM patch-hunk CI correction — 21 November 2026

- Run `44935573529` (source `93f8e2b1238b1c4f2a5386702c80ad54f4b19627`) passed the QUP host regression but stopped at the RPM patch hunk-accounting test. The third PlatformExpert hunk contained one extra context line relative to the exact recovered staged-source diff.
- No XNU compilation, kernel link, image package or physical test occurred in that run. The RPM patch was corrected directly from the recovered post-0129 source diff; hunk accounting now matches 6/19 for that hunk. Do not treat Run `44935573529` as a build acceptance result.
- Run #400 and artifact `14615937969` remain the accepted baseline. The branch still requires both host suites, the exact-source staging guard, a real pre-link object/symbol check, XNU 8796 link, independent Mach-O verification, RAM-only image packaging and manifest/checksum agreement before handset testing.


## RPM PRE_LTO test-fixture correction — 21 November 2026

- Run `44935844499` (source `c68deac68f0786306618fc1ffcfa497916943a18`) passed QUP protocol tests, RPM protocol compilation, patch hunk accounting, and both valid pre-link verifier fixtures.
- It then stopped in the RPM host runner's MakeInc injection fixture: the fixture used shell variables `$lto` / `$nonlto`, while the real Makefile recipe uses Make-escaped `$$lto` / `$$nonlto`. The injector's expected PRE_LTO anchor matched the real XNU `MakeInc.kernel`; the regression fixture did not model that syntax.
- Corrected the fixture to use the exact doubled-dollar Make syntax. No XNU compilation, link, packaging or physical test occurred in Run `44935844499`. Run #400 remains the accepted phone-test baseline.
- Rebuilt both host suites locally after the fixture change; QUP and RPM/GLINK tests pass, including the PRE_LTO injection/idempotence test. The revised injector also passes against the recovered full `MakeInc.kernel` and inserts exactly one guard per final-link path. A fresh CI run is still required.


## RPM/GLINK validation update — corrected patch diagnosis and recovered staged-tree evidence (22 November 2026)

The exact failure description is narrower than the earlier checkpoint's wording: in patch 0130, the offending unprefixed blank lines are **inside the second and fourth unified-diff hunks**, not merely separators between hunks. The RPM host runner counts hunk body lines and rejects lines that are not valid context/addition/deletion (apart from the no-newline marker). Do not weaken that validation to accept malformed patch structure.

The supplied engineering update reports that the patch was regenerated from the staged XNU build-tree recovered from failed Run 44871413803, specifically from the post-0129 state containing the Lavender PlatformExpert and QUP integration. The report says the regenerated four-hunk patch:
- Adds RPM driver registration in the platform expert.
- Publishes only the \`/soc/rpm-glink\` nub.
- Adds \`LavenderRPMGLINK.cpp\` to \`iokit/conf/files.arm64\`.
- Passes \`git apply --check\` against a fresh copy of that recovered staged tree.

Treat those as **reported exact-tree validation results from the supplied continuation**, not as a new GitHub Actions build, final ARM64 link, or physical RPM communication. The source branch should be rechecked before further changes; the live branch has advanced since the prior \`e280830\` checkpoint.

### Current pre-link enforcement — source-level confirmation

The current branch includes:
- \`tools/inject-lavender-rpm-link-guard.py\`, which injects checks in both the non-LTO and PRE_LTO recipes in XNU's \`makedefs/MakeInc.kernel\`.
- \`tools/verify-lavender-rpm-link-input.py\`, which reads the generated final-link file list(s), requires exactly one \`LavenderRPMGLINK.cpo\` input, checks that the object exists and is non-empty, and uses \`nm\` to require exactly one strong global text definition of \`lavender_rpm_vote_vdd_dig_low_svs\`.
- \`tools/test-lavender-rpm-glink.py\`, which tests valid, missing-object, duplicate-object and wrong-symbol cases and exercises hook injection/idempotence using a MakeInc fixture.

The intended hook timing is immediately after XNU generates \`link.filelist\` and immediately before \`$(LD)\` for the non-LTO recipe. For PRE_LTO, it checks the aggregate LTO and non-LTO file lists before the PRE_LTO linker path is entered. The post-build audit in \`.github/workflows/xnu.yml\` remains a separate, later diagnostic check.

The supplied continuation reports:
- Both QUP and RPM/GLINK host suites compiled and passed locally after the Make-escaped \`$$lto\` / \`$$nonlto\` fixture correction.
- The RPM runner's valid/wrong-symbol/missing-object/duplicate-object and PRE_LTO fixtures passed.
- Patch application and staging checks passed against a fresh copy of the recovered post-0129 build tree.
- The link-hook injector was exercised against the recovered \`MakeInc.kernel\` and inserted one guard per final-link path.

These results are host/staging evidence. They do **not** prove that the current GitHub branch has completed a fresh XNU build, that an ARM64 kernel was linked with this object, or that the RPM remote endpoint acknowledged a request.

### Remaining acceptance gates

1. Re-fetch the live branch HEAD and verify the patch/test/tool file hashes before the next source change.
2. Re-run QUP and RPM host suites on the exact live checkout; keep warnings-as-errors and negative fixtures.
3. Re-apply patch 0130 to a fresh copy of the exact post-0129 staged XNU source and inspect \`LavenderRPMGLINK.cpp\`, \`files.arm64\`, generated IOKit object list, PlatformExpert registration and \`/soc/rpm-glink\` publication.
4. Verify the real MakeInc hook placement against the pinned build-tool revision. For both non-LTO and PRE_LTO, show the command ordering and confirm the file lists are the same inputs consumed by the immediately following linker invocation.
5. Trigger exactly one XNU workflow only after the above gates pass. Require XNU 8796 build, final ARM64 link, independent Mach-O verification, RAM-only package creation and matching manifest/SHA-256.
6. Only after package acceptance may a RAM-only physical test use \`fastboot boot iosboot.img\`. Capture remote GLINK OPEN/OPEN_ACK, matching RPM-SMD ACK, accepted LOW_SVS vote, QUP initialization and a successful I2C transaction. No physical success is implied by host tests.

### Fail-closed and preservation invariants

- No weak vote-symbol stub; no fake or bypassed RPM ACK; no removal of QUP's acknowledged-voltage-vote gate; no direct PMIC register workaround.
- RPM/GLINK remains limited to the SDM660 shared message RAM/TOC/FIFO, APCS mailbox, GLINK v1 \`rpm_requests\` OPEN/OPEN_ACK, RPM-SMD v0 request/ACK and the initial \`rwcx\` resource ID 0 / \`vlvl=64\` LOW_SVS vote.
- No Novatek client integration until the RPM vote and a physical I2C transaction are independently demonstrated.
- Android 15/Lucid remains read-only. Do not modify Android/Linux drivers, modules, DTBs, boot images, vendor configuration or partitions.
- No display/panel bring-up in this milestone. Keep MDSS, DSI, panel timings, brightness and Tianma commands separate.
- Preserve \`cpus=1\`, the independent volume-key recovery path and RAM-only boot. Never flash, erase, format, repartition or write an Android partition.


## Engineering process cross-check — public Lavender/SWEET comparison (22 November 2026)

A public comparison was made against [sanrobin/ios-lavender](https://github.com/sanrobin/ios-lavender), including its README, agent instructions, and Lavender porting report. This is an independent reference project and is **not** part of LIOS, not a source of accepted LIOS build evidence, and not a basis for importing its patches or claims.

### What is useful to adopt as engineering practice

- Keep a source-backed device matrix that distinguishes SDM660/Lavender values from SM7150/Sweet values. Every MMIO base, interrupt, memory bank, panel mode, and storage interface must cite a target-specific DTS/source revision or an observed boot artifact.
- Record each bring-up milestone as an evidence ladder: source inspection, host test, staged-source validation, kernel compilation, final link, Mach-O verification, image packaging, RAM-only physical boot, and actual subsystem operation. Never collapse these into one "works" status.
- Preserve failure archaeology: exact image/source SHA, last visible marker, relevant log excerpt, hypothesis, falsifying test, and whether a change was actually confirmed on the device.
- Treat diagnostics as first-class engineering output. Prefer bounded, stage-specific markers and retrievable logs over conclusions inferred from a static screen or a successful compiler exit.
- Document unsupported capabilities explicitly. A working framebuffer or IOKit provider does not imply GPU acceleration, a working touch client, a complete storage stack, or Darwin userspace.

These practices are incorporated as documentation discipline only. No external source files, code, device constants, userland modifications, or claims of physical success were copied into LIOS.

### Explicit non-transfer boundary

The reference project README describes \`sweet\`/SM7150 as well as \`lavender\`/SDM660 material, and reports a separate writable \`userdata\` root-filesystem path. Its described installation and storage policy conflicts with LIOS's preservation requirements. LIOS therefore does **not** adopt its userdata flashing/root strategy, security relaxations, device identity, SM7150 constants, Goodix/GENI hardware assumptions, display parameters, IRQ mappings, RAM layout, or claims of full userland operation.

For LIOS, the confirmed Android 15/Lucid ROM remains read-only. All future phone testing remains RAM-only via \`fastboot boot iosboot.img\`; no Android partition writes are authorized. Retain \`cpus=1\` and the independent volume-key recovery route.

### Technical comparison result

The external report contains potentially useful Lavender-specific topics for future source audits: SDM660 GICv3 redistributor placement, the upper DRAM bank and framebuffer mapping under a 4 KiB page regime, the eMMC/SDHCI path, and the distinction between the Lavender framebuffer and Sweet's. These are **audit leads only**, not accepted hardware facts for LIOS. Before any implementation uses them, independently reconcile each value with LIOS's pinned bootloader DTB, Android 15 DT sources and/or the pinned Lavender Linux source at the exact commit. Do not copy an external report's numerical values without checking the exact target source and range/semantics.

The external README states that project has a separate iOS 16.7.16 userland path and documents several incomplete or disabled subsystems. That status belongs exclusively to that repository. LIOS's own acceptance language remains tied to its own image hash, logs and device observations.

### Provenance

This comparison was performed on 22 November 2026. External references:
- \`sanrobin/ios-lavender\` README on \`main\`.
- \`sanrobin/ios-lavender\` AGENTS.md on \`main\`.
- \`sanrobin/ios-lavender\` \`docs/porting_ios16_lavender_report.md\` on \`main\`.

No claim is made that the external project's reported boot state, tests, source changes, or hardware results were reproduced in LIOS.


## RPM nub iterator compile repair — 22 November 2026

### Verified failure

Run #469 (`44936263614`), source commit `7abd0cf29074da5a34dee77da58fd022b6912534`, completed with failure. Its QUP and RPM/GLINK host protocol regression steps passed. XNU compilation stopped at:

```
LavenderPlatformExpert.cpp:86:51:
error: cannot initialize a parameter of type 'OSIterator *'
with an lvalue of type 'OSArray *'
```

The failing expression passed the one-entry `rpmNubs` array directly to `IODTPlatformExpert::createNubs()`. The XNU API requires a consumed `OSIterator *`; this was a compile-time IOKit API mismatch, not an RPM protocol failure. The package job was skipped.

### Repair committed

Commit [`6c52dae`](https://github.com/NAANG360/hackios-lavender-xnu/commit/6c52daeb43a9663feb133adf076e0596b2324d10) changes `xnu/patches/0130-lavender-rpm-glink-v0.patch` and `tools/verify-lavender-rpm-glink.py`.

The staged PlatformExpert now creates the narrowly scoped one-entry array, constructs an `OSCollectionIterator`, releases the temporary array, and passes the iterator to `createNubs(this, rpmIterator)`. `createNubs()` owns and releases the consumed iterator. The retained `rpmGlink` and `soc` registry entries continue to be released. The source verifier requires this ownership path and rejects the invalid array call.

### API evidence and validation boundary

The API was checked against Apple's public XNU tag `xnu-8796.141.3`:
- `iokit/IOKit/IOPlatformExpert.h`: `createNubs(IOService *, LIBKERN_CONSUMED OSIterator *)`.
- `libkern/libkern/c++/OSCollectionIterator.h`: `OSCollectionIterator::withCollection(const OSCollection *)`.
- `iokit/Kernel/IOPlatformExpert.cpp`: `createNubs()` iterates and releases its consumed iterator. `IORegistryEntry::getChildIterator()` demonstrates the same collection-to-iterator ownership pattern.

All four unified-diff hunks in the repaired patch were independently recounted; declared and actual old/new line counts match.

Run #470 (`45018828868`) is now validating this repair on source commit `6c52dae`. QUP and RPM/GLINK host regressions passed; the patch applied and the staged-source verifier passed. XNU 8796 compilation is still in progress. Final ARM64 link, linked-object verification, image package and physical RPM/QUP operation remain unverified.

### Required next gates

1. Re-fetch live HEAD and Actions before continuing.
2. Run QUP and RPM/GLINK host tests on the exact live checkout.
3. Apply patch 0130 to a fresh exact post-0129 staged XNU tree; run source verifier and inspect PlatformExpert source plus `files.arm64`.
4. Verify non-LTO and PRE_LTO pre-link guard placement and object/symbol inputs.
5. Run one XNU build; require final link, independent Mach-O verification, RAM-only packaging and matching manifest/SHA-256.
6. Keep Run #400 artifact unchanged. Do not physically test until the complete acceptance ladder passes.

No Android/Lucid ROM, partition, boot image, recovery image, DTB or vendor file was modified. No physical test was performed.


## Run #470 live status — 22 November 2026

- Workflow: [Run #470 / 45018828868](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45018828868)
- Source commit: `6c52daeb43a9663feb133adf076e0596b2324d10`
- Passed: Lavender QUP host protocol regression, Lavender RPM/GLINK host protocol regression, patch application, staged-source verifier and build preparation.
- In progress at checkpoint: `Build XNU 8796`.
- No final ARM64 link, pre-link object verification, Mach-O artifact, RAM-only package, checksum-bound image or physical test is yet confirmed. Run #400 remains the latest accepted image baseline.
