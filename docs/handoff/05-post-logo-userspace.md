# Chapter 5 — Post-logo kernel and Darwin userspace

This chapter is the active roadmap after the Run #261 physical logo-stage result. Keep the XNU kernel, root filesystem, launchd, graphical services and peripheral drivers as separate acceptance gates.

## 1. Logo asset — use Apple's real firmware resource

Patch `0094-lavender-logo-and-stage-markers.patch` currently renders a diagnostic approximation using ellipse equations, a bite cut-out, leaf and stem. The user wants the actual Apple boot logo, not a recreation.

The selected userspace reference is iPhone X iOS 16.7.16 build 20H392 (iPhone10,3 / iPhone10,6). Its IPSW contains an AppleLogo iBoot image under the `Firmware/all_flash` tree. Depending on release, the name/path is under `all_flash.<model>.production/` or a top-level `applelogo@...~iphone.im4p`; do not hardcode a guessed member name.

Suggested extraction/validation path:
1. Use the exact 20H392 IPSW already selected for the iOS root filesystem.
2. Use `tools/remotezip.py` with a narrow `applelogo` filename filter to retrieve only the AppleLogo IM4P member; that helper uses HTTP range requests and validates the ZIP CRC.
3. Inspect/extract the IM4P payload with the `ipsw img4 im4p` tools, then decode the iBoot image with `ipsw fw ibootim`. Check the installed `ipsw` CLI's `--help` before scripting flags.
4. Record source IPSW SHA, archive member path, IM4P tag, decoded dimensions, pixel order, alpha semantics and asset SHA. Do not assume PNG or raw RGBA until decoded.
5. Convert the exact image data to a small generated C/header or raw bitmap that the XNU build can link. Preserve transparency and aspect ratio; composite white logo on black only at the framebuffer layer.
6. Replace only the procedural bitmap generation in `sweet_early_logo()`. Keep its post-`arm_vm_init()` call position and framebuffer alias. Add a host-side decoder/render test and a source/binary size guard.

The AppleLogo's known firmware location and iBoot-image format are documented by [The Apple Wiki — AppleLogo](https://theapplewiki.com/wiki/AppleLogo). The `ipsw fw ibootim` command is documented by [blacktop/ipsw](https://blacktop.github.io/ipsw/docs/cli/ipsw/fw/ibootim/). This is a visual correction only; it does not itself advance kernel startup or enable Darwin services.

## 2. Confirm the post-logo XNU frontier

The Run #261 manifest says the diagnostic logo is drawn after `arm_vm_init()`; the code then publishes a logo-completed marker. The current recording shows the logo but not the following Guardian/kprintf marker or a persistent kernel log.

Next:
- preserve the exact Run #261 image and source SHA as the known logo-stage baseline;
- make the next diagnostic readout distinguish logo completion, stage 20 (`kprintf` startup complete), and the next existing XNU initialization stage;
- capture the exact last marker and time on the next safe test;
- only after the independent recovery/reset safety gate is verified may a fresh RAM-only image be tested.

Do not increase CPU count or alter startup callback order at the same time as logo/marker changes. The packaged Run #261 command line uses `cpus=1`; secondary-core/SMP operation is not physically proven by this video.

## 3. Root filesystem and storage — the principal Lavender porting gap

The inherited Sweet root flow is not portable as-is:
- `tools/mkiosroot.sh` creates a large case-sensitive journaled HFS+ image containing the iOS OS volume and SystemOS/AppOS cryptexes.
- `tools/sweet-root-tweaks.py` applies 20H392 userland bring-up edits (Setup skip, launchd crash handling, CommCenter removal, audio/media workarounds and activation data).
- `mkiosroot.sh` documents the target as `userdata` with `rd=ufsroot0`; the Sweet BSD root-device driver is `bsd/dev/sweet_ufs.c` from patch 0028 and speaks UFSHCI/SCSI.
- Lavender's internal storage is eMMC on Qualcomm SDHCI MSM v5 (`c0c4000.sdhci`), not UFS. The inspected Lavender tree has no eMMC/SDHCI BSD block driver. Do not reuse the Sweet UFS registers, SCSI protocol or UFS root-device name.

**New removable-storage lead (public mainline DTS; live-FDT confirmation still required):** the device-specific mainline `sdm660-xiaomi-lavender.dts` enables a second host via `&sdhc_2`, declares card-detect GPIO 54, and supplies separate VMMC/VQMMC rails. The included SDM630/660 SoC description places that host at `0x0c084000`, 4-bit bus, with host/power IRQs 125/221 and interface/core/XO clocks. This strongly suggests a removable SD-card slot and is a better root target than internal Android eMMC. It is **not yet confirmed** in the exact Build 4 boot-image FDT used by this branch; do not add the node to the Lavender ADT or begin driver MMIO until that embedded FDT is inspected. The public device DTS enables the node and sets GPIO54 card-detect; a downstream kernel patch also reflects this Lavender overlay.

Research references: [Lavender device-tree change showing enabled `sdhc_2` and GPIO54](https://lists.openwall.net/linux-kernel/2025/06/19/1081); [Qualcomm SDM660 SDHCI MSM driver](https://github.com/torvalds/linux/blob/master/drivers/mmc/host/sdhci-msm.c).

Audited target details in `xnu/patches/LAVENDER-PORT-STATUS.md`:
- SDHCI MSM v5 register windows: `0x0c0c4000/0x1000`, `0x0c0c5000/0x1000`, `0x0c0c8000/0x8000`.
- eMMC is 8-bit, non-removable; clocks include `iface_clk`, `core_clk`, `ice_core_clk`; the FDT lists IRQs 110 and 112.
- The current code has not implemented the Lavender eMMC controller driver or HFS+ root path.

### Non-destructive root constraint

The original Sweet README instructs flashing the HFS+ image to `userdata`. That is **not allowed** for Lavender: preserve Android, recovery, calibration and all boot-critical partitions. No partitioning or storage writes.

Before root testing, choose a storage design that honours this constraint. Candidate architectures to evaluate—not yet approved—are:
- a separate removable/external block device, if its controller and boot-time enumeration can be supported;
- a read-only root image exposed through a shim-loaded RAM disk, subject to actual size and reserved-memory limits;
- a read-only image file accessed through a supported host filesystem/container layer, which would require its own safe filesystem access path.

Do not silently choose one or adapt the Sweet `userdata` flash instructions. The XNU root device must support read-only bring-up first; write support is a separate later gate.

### RAMDisk capacity check — do not append the current full root image

The Lavender ADT exposes one conservative contiguous DRAM aperture from `0xA2000000` to `0xEA848000` (panic-log area excluded). The kernel's last segment ends at approximately physical `0xA4020000`, leaving roughly `0x46828000` bytes (about 1.10 GiB) for an appended image. By contrast, `tools/mkiosroot.sh` defaults to a **16 GiB logical HFS+ image**. Even if the host file is sparse, its logical size is still 16 GiB; it cannot be placed wholly in that RAM aperture.

The current `tools/mkpayload.py --ramdisk` path appends the whole image to the kernel payload and advertises it as `RAMDisk` in the ADT. Add/retain a pre-read bounds check against `dram-base + dram-size`; fail before reading an oversized file. Do not attempt to fit the 16 GiB image by overlapping the panic reservation, other RAM, framebuffer, kernel or ADT. This capacity mismatch makes a removable SD-card root materially more attractive. A small initramfs remains possible only if its actual page-aligned size fits the validated remaining aperture.

### Recommended driver sequence

1. Inspect the exact Build 4 embedded FDT for `sdhc_2` at `0x0c084000`, GPIO54 card detect, IRQs, clocks, reset, pinctrl and regulator phandles. Compare it to the public Lavender DTS; do not infer that the branch's generated ADT already contains this node.
2. If confirmed, implement the removable SD host first as a conservative polled, read-only path. Begin at low clock with PIO and no CQE/ICE/DMA; validate card detection, CMD0/CMD8/ACMD41, CID/CSD, RCA/select and single-block reads before GPT or HFS+. Keep separate from the internal eMMC driver.
3. Read the exact Linux SDHCI MSM v5 implementation and Lavender FDT to map clock/reset, power, bus width, voltage, tuning and DMA behavior. Linux sources are hardware references, not drop-in XNU code.
4. Audit whether ABL has already initialized the removable controller and whether its state survives the shim. Never assume the host's card/clock/power state from the device-tree node alone.
5. Expose a BSD block device and GPT partition discovery with a stable Lavender-specific root-device name. Do not reuse `ufsroot0`.
6. Mount an HFS+ root read-only, verify journal/root metadata handling, then test APFS cryptex integration and dyld shared-cache paths using the existing 20H392 root build.
7. Only then add narrowly scoped write/sync support, with explicit approval and a safe non-Android target.

The exact SDHCI MSM hardware programming sequence must be grounded in the Lavender controller revision and Linux/vendor source. Do not copy register constants from Topaz or Sweet.

## 4. Userland boot sequence

The Sweet reference explains the broad sequence:
`XNU → root block device/HFS+ → dyld + shared cache → launchd → early system daemons → backboardd → SpringBoard`.

Lavender still needs the root-device layer above before launchd is a realistic target. Once the root mounts:
1. Verify the root is the exact 20H392 iPhone X tree and that the SystemOS/AppOS cryptex contents and dyld cache links resolve.
2. Apply and audit the existing `sweet-root-tweaks.py` as a reference; keep only changes justified for Lavender.
3. Capture launchd's earliest logs and resolve missing symbols, trust-cache/AMFI, entitlement, sandbox and platform-service dependencies one at a time. Do not disable entire subsystems blindly.
4. Get essential launch daemons stable before attempting `backboardd`.
5. Treat SpringBoard/lock screen as a later milestone after graphics and input services exist.

## 5. Device-facing services after root

The port-status audit identifies further Lavender-specific gaps:
- **Display:** framebuffer console/logo works, but an IOMobileFramebuffer/IOFramebuffer service for the iOS graphics stack is not established.
- **Touch:** Lavender uses Novatek `novatek,NVT-ts` on I²C address `0x62`, reset GPIO 66 and IRQ GPIO 67. Sweet patches 0060/0062 are Goodix/GENI/board-specific and must not be reused. There is no Lavender NVT driver.
- **I²C/TLMM/PMIC:** controller and GPIO/power sequencing must be implemented from the Lavender FDT and vendor source.
- **Buttons/recovery:** power button is broken; the volume-key chord has not been verified as a bootloader recovery selector.
- **SMP:** keep `cpus=1` until stage-by-stage secondary CPU startup and synchronization are verified.
- **Wireless, cellular, audio, camera, GPU and SEP-dependent functions:** unsupported until their Lavender-specific hardware/service dependencies are implemented.

## Acceptance ladder

1. Run #261: compile + verify + package — passed.
2. Run #261 physical image: diagnostic logo screen — user-reported and video-supported; not checksum-bound.
3. Genuine AppleLogo asset displayed — not yet done.
4. XNU stage 20 and post-logo kernel continuation — not yet proven.
5. Lavender eMMC/root-device read-only path — not implemented.
6. HFS+ root and cryptexes mount — not done.
7. launchd and essential daemons — not done.
8. backboardd / IOMobileFramebuffer / input — not done.
9. SpringBoard / lock screen — not done.

No device test until the recovery/reset safety gate is met. RAM-only boot only; never overwrite Android userdata or boot partitions.
