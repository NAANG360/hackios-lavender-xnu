# Chapter 1 — Device, scope and safety

## Target
- Xiaomi Redmi Note 7 / Note 7S; codename `lavender`; Qualcomm SDM660 / Snapdragon 660; ARM64.
- Goal: XNU 8796 (iOS 16 generation) on physical Lavender; longer-term iOS 16.5-era Darwin userspace.
- Not an Android theme, generic ARM64 kernel, emulator demo or iOS-lookalike launcher.

## Hard safety constraints
- Preserve Android boot/recovery and all non-userdata partitions. The user explicitly selected the existing `userdata` partition for the future iOS HFS+ root, accepting that Android user data will be erased.
- Kernel bring-up remains temporary RAM-only `fastboot boot iosboot.img`; the later root image is a separate, deliberate write to `userdata` only. Never write boot, vendor_boot, recovery, modem, persist or other eMMC partitions.
- The power button is broken. The user now confirms the volume-key reset/recovery route works; preserve it and document the exact sequence before any destructive `userdata` write.
- Returning to Android is expected to require recovery to wipe/format `userdata`; this does not restore personal data.
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
