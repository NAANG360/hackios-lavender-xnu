# Hardware and constraints

The port targets the **Redmi Note 7** (`lavender`, `lavender_eea`), Qualcomm **SDM660 / Snapdragon 660**. The test unit is 4 GB RAM / 64 GB eMMC, with a **Novatek** touch controller. Other lavender variants may use a Synaptics panel/touch; check the I2C bus before assuming this exact configuration.

## SoC and CPU

| | |
| --- | --- |
| SoC | Qualcomm SDM660 |
| CPU | 4× Cortex-A73 “Gold” + 4× Cortex-A53 “Silver” |
| ISA | ARMv8.0-A; **no LSE atomics** (`fp asimd evtstrm aes pmull sha1 sha2 crc32`) |
| Page size | XNU/iOS uses 16 KiB. Only the A73 supports `TGran16=1`; the A53 does not |
| Cores used | 4× Gold. The Silver cores are parked via PSCI because they cannot run 16 KiB pages |
| Kernel codegen | `-mcpu=cortex-a53` baseline, no LSE |

The 8-core / 4 KiB route is a separate research line (see `docs/ROADMAP.md`); it is not the current boot path.

## Memory map (relevant)

| Region | Address |
| --- | --- |
| Framebuffer | `0x9d400000`, 1080×2340, stride 4320, `a8r8g8b8` |
| SDCC v5 (eMMC) | `0x0c0c4000`, GIC SPI 110 |
| GIC distributor | `0x17a00000` |
| GIC redistributor | `0x17b00000` |
| QUP1 I2C (touch) | `0x0c175000`, SPI 95 / INTID 127 |
| UART | Debug console at 115200 8N1 (board-specific) |

## Storage

- eMMC 5.1 behind the Qualcomm **SDCC v5** controller.
- Root filesystem: HFSX image staged in `userdata`; `rd=emmcroot0`.
- The SYSTEM partition holds the 4 MiB `SANTONILOG1` console ring (sectors 0..8192).
- The GPT is fragile: a buggy storage driver can overwrite LUN0. Backups and read-only first boots are mandatory.
- The eMMC has no usable cache enabled (`CACHE_CTRL 0x00`); writes are synchronous and slow. Patch `0192` waits for `READY_FOR_DATA` after each PIO write.

## Display

- 1080×2340 panel, 4320-byte stride, `a8r8g8b8`.
- ABL leaves a continuously scanned splash framebuffer at `0x9d400000`; MDSS/DSI keep scanning it after handoff.
- The kernel publishes an AppleCLCD/IOSurface path and composes in software. There is no Adreno GPU driver.
- The lock screen is portrait and upright on the panel (see `docs/images/lock-screen.jpg`), and the home screen uses the same orientation (`docs/images/home-screen.png`). The date, clock and status bar render red/orange on both; whether that is the wallpaper tint or a framebuffer colour-format issue has not been checked.

## Touch

- **Novatek NT36672** on QUP1 I2C, address `0x62` on this unit.
- IRQ GPIO 67, reset GPIO 66, VDDIO from PM660 LDO11.
- The Novatek NT36672A is driven by patches `0193`–`0208`, `0230`, `0231` and `0234` over a bit-banged QUP1 I2C (the hardware I2C block is not brought up yet). It works on the device: taps, icon taps and the swipe-up unlock, with 0 I2C errors and ~14 ms per idle read.
- The alternative panel variant uses Synaptics at `0x20`; do not assume it.

## Buttons and PMIC

- PM660 / PM660L PMIC.
- SPMI PMIC arbiter **v3** (DTB `qcom,spmi@800f000`): core `0x0800f000`, chnls `0x08400000`, obsrvr `0x09400000`, intr `0x0a400000`, cnfg `0x0800a000`, ee 0. PM660 is SID 0; PM660L is SID 2/3.
- Keys (DTB): Power = PM660 PON KPDPWR, Volume Down = PM660 PON RESIN, Volume Up = PM660L GPIO7 (`gpio@c600`, active low). Lavender has no camera key; the DTB `camera_focus`/`camera_snapshot` entries (TLMM 64/113) come from the reference board.
- Patch `0201` polls these keys read-only and feeds `SWEET HID`; Power, Volume Up and Volume Down reach iOS on the device. A stuck iOS boot can still be recovered with a PMIC long-press (Power 20–30 s), a key combo at boot, or battery drain.

## USB

- Synopsys DWC3 behind the Qualcomm `dwc-usb3-msm` wrapper (DTB `ssusb@a800000`): core `0x0a800000`, QSCRATCH `0x0a8f8800` (core + `0xf8800`).
- PHYs: QUSB2 **v1** HS PHY at `0x0c012000` (`major-rev 1`), QMP SS PHY at `0x0c010000`.
- GCC `0x00100000` (`qcom,gcc-sdm660`); the USB30 GDSC is at `0xf004`. The DT node declares no `iommus`.
- Patch `0200` drives the controller in device mode (CDC-ECM). On other SoCs ABL leaves USB powered after `fastboot boot`. Lavender boots the shim from `recovery`, and the USB state after that path has not been measured yet.

## GPU

- Adreno 512. There is no iOS driver, no firmware and no Mesa/freedreno path in this port.
- The kernel publishes an **empty `IOAcceleratorES`** service so `CAMetalContextCreate` does not wait 30 s. Rendering falls back to software; the boot UI and SpringBoard are slow.
- The whyred/topaz ports have more advanced GPU work; see `docs/REFERENCES.md`.

## Bootloader quirks

- The bootloader is unlocked (`fastboot getvar unlocked: yes`).
- On the test unit, `fastboot boot <image>` returns `OKAY` but the ABL does not execute the image. The working path is to flash the shim to `recovery` and boot with **Power + Volume Up**.
- `fastboot reboot recovery` boots Android, and MIUI restores the stock recovery. Do not use it for the shim.
- A USB link drop during a `userdata` flash can leave fastboot in a `????????????` state; reconnect the cable and re-enter fastboot.

## Reference device

The iOS userland is from an **iPhone X (`D22AP`) iOS 16.7.16 (20H392)** IPSW. The product ADT (`d22ap.adt`) comes from that IPSW. The port does not distribute the IPSW, the ADT or any Apple binary.
