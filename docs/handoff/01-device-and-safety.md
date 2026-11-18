# Chapter 1 — Device, scope and safety

## Target
- Xiaomi Redmi Note 7 / Note 7S; codename `lavender`; Qualcomm SDM660 / Snapdragon 660; ARM64.
- Goal: XNU 8796 (iOS 16 generation) on physical Lavender; longer-term iOS 16.5-era Darwin userspace.
- Not an Android theme, generic ARM64 kernel, emulator demo or iOS-lookalike launcher.

## Hard safety constraints
- **Current authorization is RAM-only kernel testing.** Use `fastboot boot iosboot.img` only. Do not flash, erase, format, repartition, or write Android eMMC, `userdata`, boot, vendor_boot, recovery, modem, persist, or any other device partition.
- Earlier project notes discussed a possible future HFS+ root on `userdata`. That is historical planning only, is not part of the current bring-up authorization, and must not be treated as permission to write or wipe the partition. Any future storage write requires a separate, explicit user decision after a reviewed recovery/data-loss plan.
- Preserve Android boot/recovery and all user data. Keep the confirmed volume-key recovery/reset route available during every physical test. The power button is broken; the Guardian watchdog-first reset fallback remains independently unverified.
- QEMU is allowed only for narrow isolated tests; never as proof of Lavender compatibility.
- Keep raw dumps, serials, calibration/private data and firmware blobs out of Git.
- Exact delivered image filename: `iosboot.img`, no date/build suffix.

## Hardware facts (audited)
- 8 Kryo 260 logical CPUs; observed boot CPU MPIDR `0x100` (Gold/A73 cluster).
- Linux-visible RAM: 2,803,968 kB (~2.8 GiB on 3 GB model).
- Android 15 / SDK 35; running Linux kernel observed as `4.19.325-NYXION-SUSFS`.
- Display: Tianma FHD+ DSI, 1080×2340, `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`.
- GPU Adreno 512; storage controller `c0c4000.sdhci` (Lavender docs indicate eMMC 5.1).
- Device-tree compatibles include `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp`.
- Lavender is ARM64, not arm64e. Do not assume Apple pointer authentication, PPL or Apple SoC services.
- Derive memory/reservations from bootloader FDT/DTB. Linux-observed ranges are not an approved XNU map.
- Do not import IOS-Sweet SM7150/Kryo 470/Redmi Note 10 Pro hardware constants.

## Source authority
Use HakiOS/hackIOS for architecture and boot methodology; IOS-Sweet for XNU 8796/iOS 16 staging patterns; public Lavender/SDM660 sources for all target-specific facts. This repository inherits sweet/SM7150 material: treat it as reference only unless independently validated for Lavender.
