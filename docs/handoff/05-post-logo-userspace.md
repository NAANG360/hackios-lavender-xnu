# Chapter 5 — Post-logo kernel and Darwin userspace

This is the active roadmap after the user-observed Run #261 physical framebuffer logo-rendering milestone. The observed artwork is patch 0094's custom approximation; the user explicitly requires the genuine Apple asset for subsequent images. Keep the XNU kernel, genuine AppleLogo artwork, storage/root device, launchd, graphics/input services and SpringBoard as separate acceptance gates.

## 1. Genuine AppleLogo asset — extracted and integrated in source; full CI acceptance pending

The previous `0094-lavender-logo-and-stage-markers.patch` draws a hand-made diagnostic approximation using ellipses, a bite cut-out, leaf and stem. The user explicitly requires Apple's actual logo. Do not redraw or substitute another approximation.

The exact source resource has been extracted from the iPhone X Global iOS 16.7.16 (20H392) IPSW:
- Archive member: `applelogo@3x~iphone.im4p`
- Source IM4P SHA-256: `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`
- Decoded image: 194 × 238 RGBA PNG, white premultiplied-alpha mask
- Decoded PNG SHA-256: `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`
- Asset bytes are not committed to Git; CI creates a temporary C header from the verified firmware asset.

New integration:
- Patch `0114-lavender-applelogo-from-20h392.patch` replaces only the procedural mark inside `sweet_early_logo()` with the extracted alpha mask. It preserves the full-panel black clear, centered placement, 32-bit framebuffer writes and the existing call after `arm_vm_init()` / permanent framebuffer alias.
- `tools/prepare-lavender-applelogo.sh` retrieves the exact IM4P with `tools/remotezip.py`, verifies its member SHA, decodes it with pinned `ipsw` and generates the header.
- `tools/pack_lavender_applelogo.py` checks the pinned PNG/IM4P hashes, dimensions and white-alpha pixel semantics.
- `.github/workflows/xnu.yml` requires patch 0114, includes both scripts in its cache key and checks the generated header/source before accepting the build.

**CI status:** Run #267 failed because shell parameter expansions were escaped; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` fixed this. Run #268 (`43805363291`) then compiled XNU and passed the exact AppleLogo header/hash/source guard, but failed later because the startup-trace verifier sliced its helper body using the removed `lavender_logo_ellipse` symbol. Commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` changes the slice boundary to `sweet_early_logo(void)`. This fix still requires a new successful build, linked Mach-O verification and RAM-only package. Do not claim the authentic logo is on the phone until a safe physical test confirms it.

The original resource came from Apple's firmware; the raster is not a substitute for the entire Apple boot UI or a proof of a fully booted iOS system.

## 2. Confirm the post-logo XNU frontier

The Run #261 manifest places the logo renderer after `arm_vm_init()`, then publishes a logo-completed marker. The user-supplied 20.295-second recording shows the custom diagnostic mark; it does not expose stage 20 (`kprintf` startup complete), root mounting, launchd or a persistent userspace log. The recording is attributed to #261 by the user but is not bound to the image by a device-side checksum.

Next:
- keep Run #261/#266 as known verified old-logo baselines;
- after the official-logo build passes, add/retain separate evidence for logo completion, stage 20, post-logo init and first BSD/root milestone;
- capture the exact last marker and time on the next safe test;
- do not infer scheduler, interrupt, SMP or userspace readiness from the logo.

The Run #261 package uses `cpus=1`. Keep secondary-core/SMP operation unclaimed until independently proven.

### Exact next kernel boundary from the #268 patched XNU source

In `osfmk/arm/arm_init.c`, the logo-completed marker (stage 17) is followed by:
1. optional `patch_low_glo()` and classic S2R setup;
2. `PE_consistent_debug_inherit()`;
3. `serial_init()`;
4. `kernel_startup_initialize_upto(STARTUP_SUB_KPRINTF)`;
5. `kprintf("kprintf initialized\\n")`, then stage 20.

The user video shows the logo but not stage 20. Therefore the next kernel milestone is not assumed to be BSD/root yet. Add bounded diagnostic markers around steps 2–4 so the next safe test can distinguish a post-logo stall from kprintf startup completion. After stage 20, continue through `PE_create_console()`, `PE_init_printf()`, `PE_init_platform()`, `machine_startup()` and the BSD bootstrap/root selection path. Keep each boundary independently verifiable.


## 3. Root filesystem and storage — internal eMMC SDHC1, root image in userdata

The user has explicitly clarified the storage plan: **the iOS root filesystem belongs in Lavender's existing `userdata` partition**, not on a removable SD card. The user accepts that Android user data will be erased and plans to return to Android by using the working volume-key recovery/reset route to wipe/format `userdata`. Keep Android boot/recovery and every non-userdata partition intact.

### Topaz is the architecture reference, not the transport driver

The exact reference is [MakrSas/ios-topaz](https://github.com/MakrSas/ios-topaz), branch `topaz`. Its README documents:
- an iPhone X iOS 16.7.16 (20H392) HFS+ root on the existing `userdata` partition;
- XNU booted through a RAM-loaded shim (`fastboot boot`), without replacing boot partitions;
- BSD root selection through `rd=ufsroot0`, with separate `ufsrw=1 rootrw=1` write-enable arguments;
- the progression from XNU root mount through dyld/shared cache, launchd, backboardd and SpringBoard;
- a UFS 3.0 host driver on the Redmi Note 12 4G (SM6225 / Snapdragon 685).

This validates the *deployment pattern* the user wants. It does **not** validate using Topaz's UFS driver on Lavender. Lavender is SDM660 and uses internal eMMC through SDHC1. Reuse only the architecture and generic XNU/BSD patterns: root-device selection, block-device registration, GPT partition discovery, HFS+ mount integration, explicit read/write policy, and staged userspace validation. Do not copy Topaz UFSHCI/SCSI commands, registers, DMA descriptors, clock/rail values, DT/ADT nodes or SoC-specific driver code.

### Exact Lavender storage targets

The Build 4 embedded FDT distinguishes two hosts:

**SDHC1 — internal eMMC (the iOS root target)**
- Base: `0x0c0c4000`.
- 8-bit bus, `non-removable`.
- FDT includes CQE/ICE resources. Initial XNU driver must not assume CQE/ICE is initialized or required; start with a conservative polled PIO path.
- This is the controller that backs the existing `userdata` partition. Confirm its exact FDT node, IRQs, clocks, rails, pinctrl and V5 power-event resources from the same Build 4 DTB and matching Xiaomi SDM660 downstream source before writing driver code.

**SDHC2 — removable SD slot (not the root target)**
- Base: `0x0c084000`, 4-bit bus, GPIO54 card detect, host IRQ125, power IRQ221, PM660L L5/L2 and GCC SDCC2 clock IDs 93/94.
- These are SDHC2-specific details and must not be applied to SDHC1.
- No removable card is required for the planned iOS root.

Do not infer SDHC1 clock/IRQ/regulator details from SDHC2. The V5 controller power protocol may be shared at the IP level, but the host's own FDT resources and rail/pinctrl wiring must be independently resolved.

### Root image and Android-return model

- Build the iPhone X 20H392 root as a case-sensitive journaled HFS+ volume with the required OS/SystemOS/AppOS content and cryptex/shared-cache layout. Topaz's `tools/mkiosroot.sh` is a useful host-side layout reference: one HFS+ volume, OS root plus SystemOS/AppOS under `/private/preboot/Cryptexes/{OS,App}`, and hard-linked dyld shared-cache lookup paths. Adapt and validate for Lavender; it is not a storage transport driver.
- The eventual image is written to the existing `userdata` partition. This intentionally replaces Android's user-data filesystem and will erase files/accounts stored there.
- Do not repartition the eMMC or overwrite Android boot, vendor_boot, recovery, modem, persist or other partitions.
- The user's recovery/reset route is now confirmed working. Before the first userdata image write, preserve the exact key sequence and verify that recovery can format userdata back to Android's expected filesystem. Wiping userdata restores the Android data partition format; it does not recover erased personal files.
- Keep the root filesystem's write policy explicit. First prove block reads and HFS+ metadata using a read-only driver. Only after that and the recovery path are validated, add writes scoped to the userdata partition and its verified bounds. No raw writes outside userdata.

### Lavender SDHC1/eMMC driver plan

1. **Read-only discovery first.** Parse the exact SDHC1 node from the Build 4 FDT; validate the register window, IRQs, clocks, regulator/pinctrl states, bus width, eMMC identity and controller version. Map only the verified MMIO window and inspect documented identity/capability/present-state/version registers. Do not write controller, clock, TLMM, PMIC or power-control registers during this first probe.
2. **Audit the SDHCI MSM V5 power handshake for SDHC1.** Resolve BUS_ON/OFF and IO_LOW/HIGH semantics, power IRQ routing, rail requests and pinctrl transitions from the matching SDM660 downstream `sdhci-msm.c` and FDT. Do not reuse SDHC2's GPIO54, IRQ125/221, PM660L rails or SDCC2 clock IDs.
3. **Bring up conservative eMMC PIO reads.** Use the eMMC protocol and host state established by the bootloader only after inspecting it. Bound every wait; handle command/data inhibit, response and data-ready/error states. Start without DMA, CQE, ICE, HS200/HS400 tuning, hotplug or write commands. Establish the safe timing/width from the live controller and downstream driver, not assumptions.
4. **Read GPT and identify userdata.** Verify capacity, logical block size, GPT primary/backup consistency, partition GUID/type, start/end LBAs and bounds. Do not guess the userdata LBA or size from another device.
5. **Validate the HFS+ image read-only.** Read the volume header, allocation metadata, journal and expected 20H392 paths; ensure all block requests remain inside userdata. Only then register a Lavender-specific BSD block device and root selection (for example `rd=lavenderroot0`), distinct from Sweet/Topaz's `ufsroot0`.
6. **Mount and progress userspace.** Initially mount HFS+ read-only; verify cryptex and dyld shared-cache resolution. Add a deliberate root-write option only after read-only boot and recovery tests pass, and enforce partition-bound checks so writes cannot reach adjacent partitions.
7. **Keep failure recovery practical.** A failed iOS root attempt must leave boot/recovery/modem/persist intact; use the confirmed recovery route to wipe userdata and return to Android's expected data filesystem.

### XNU integration pattern

The exact XNU 8796 buildtree has no Lavender SDHCI/eMMC driver. The existing `bsd/dev/sweet_ufs.c` and Sweet-specific `IOFindBSDRoot()` branch demonstrate the BSD-facing pattern (`bdevsw_add`, `cdevsw_add_with_bdev`, `devfs_make_node`, strategy/ioctl/psize and root-device selection), but its UFSHCI/SCSI transport, DMA descriptors, cache handling and `ufsroot0` name are not Lavender's implementation. Create a separate `bsd/dev/lavender_sdhci.c` and a separate `rd=lavenderroot0` root-selection path.

### RAMDisk capacity — full root cannot be appended

The Lavender ADT exposes one conservative contiguous DRAM aperture from `0xA2000000` to `0xEA848000` (panic-log area excluded). The kernel ends near physical `0xA4020000`, leaving around `0x46828000` bytes (~1.10 GiB). A 16 GiB logical HFS+ image cannot fit as a kernel-appended RAMDisk, even if the host-side file is sparse. Keep the payload builder's oversize-RAMDisk rejection. The persistent root belongs in `userdata`, not in the appended RAMDisk and not on removable SDHC2.

## 4. Darwin userspace boot sequence

The broad sequence is:
`XNU → root block device/HFS+ → dyld + shared cache → launchd → early system daemons → backboardd → SpringBoard`.

After a read-only root mounts:
1. Verify the root is the exact iPhone X 20H392 tree and the SystemOS/AppOS cryptex contents and dyld cache paths resolve.
2. Audit `sweet-root-tweaks.py` for Lavender relevance; keep only individually justified changes. In particular, Topaz removes CommCenter launch jobs because its README says cellular data is not working, and disables some audio/recorder and Setup paths. Do not apply those changes blindly: preserve CommCenter and required Apple services for Lavender's intended future cellular bring-up.
3. Capture launchd's first logs and resolve missing symbols, trust-cache/AMFI, entitlements, sandbox and platform-service dependencies one at a time. Do not disable entire subsystems blindly.
4. Stabilize essential early daemons before attempting `backboardd`.
5. Treat SpringBoard/lock screen as a later milestone, after graphics and input services exist.

## 5. Device-facing services after root

Topaz is useful as a subsystem reference, with strict hardware boundaries:
- **Wi-Fi:** Topaz patches 0113–0130 demonstrate Qualcomm PAS/TrustZone modem boot, GLINK/QRTR, modem host services, WLFW, WMI/HTT and the WPA2 station/data path. Lavender's public SDM660 runtime log identifies WCN3990 on the ath10k SNOC path; validate the exact Lavender firmware, memory, clocks, regulators, IRQs and TLMM before porting. Topaz does not establish cellular data support.
- **Display and cache:** Topaz patches 0092–0112 demonstrate direct scanout, double buffering, cache maintenance and brightness-service integration. Reuse the concepts only; Lavender's MDSS/DSI panel and Adreno 512 are different.
- **IOSurface:** Topaz patches 0131–0139 are a reference for CoreAnimation-facing IOSurface semantics and release/attachment behavior, independent of the underlying GPU.
- **Input:** Topaz's FocalTech FT5452 driver is not applicable; Lavender's Novatek NT36xxx controller at I²C 0x62 needs its own driver.
- **Battery/audio/BT/cellular:** Topaz's README lists cellular data, Bluetooth and audio as not working in that port. Its battery path is PMIC-specific. Use separate Lavender/SDM660 and topaz-woa research only as protocol references, not as claims of working iOS drivers.

Further Lavender-specific gaps:
- **Display:** framebuffer console/logo works, but an iOS IOMobileFramebuffer/IOFramebuffer service is not established.
- **Touch:** Lavender uses Novatek `novatek,NVT-ts` on I²C address `0x62`, reset GPIO 66 and IRQ GPIO 67. Sweet patches 0060/0062 are Goodix/GENI-specific and must not be reused. No Lavender NVT driver is present in the inspected source.
- **I²C/TLMM/PMIC:** controller, GPIO and rail sequencing must come from Lavender's FDT/vendor sources.
- **Buttons/recovery:** power button is broken; the user confirms the volume-key reset/recovery route works. Preserve and document the exact sequence before the userdata image write.
- **SMP:** keep `cpus=1` until secondary CPU startup and synchronization are verified stage by stage.
- **Wireless, cellular, audio, camera, GPU and SEP-dependent functions:** unsupported until the required Lavender hardware/services are implemented.

## Acceptance ladder

1. Run #261 XNU build, verification and package — passed.
2. Physical Run #261-attributed image — custom diagnostic logo-rendering stage observed in user video; not image-checksum-bound.
3. Run #266 build, verification and package — passed with old diagnostic logo; not separately physically tested.
4. Genuine 20H392 AppleLogo — asset extracted/hash-verified and patch 0114 integrated; full CI acceptance pending after verifier fix commit `5dc07a9`.
5. Genuine AppleLogo physical display — not yet tested; wait for full CI acceptance and recovery-safety gate.
6. XNU stage 20 and post-logo kernel continuation — not physically proven.
7. SDHC1/eMMC read-only probe — not implemented; this is the next software/hardware discovery task.
8. SDHC1 V5 power/state audit and bounded PIO eMMC reads — not implemented.
9. GPT/userdata bounds, Lavender BSD block device and HFS+ read-only root mount — not done.
10. iOS 20H392 cryptexes/dyld, launchd and early daemons — not done.
11. backboardd, IOMobileFramebuffer, input and SpringBoard — not done.

The user confirms the volume-key reset/recovery route works despite the broken power button. Keep RAM-only XNU boot for kernel bring-up. A later explicit root-image write to userdata is in scope; never repartition or write boot, vendor_boot, recovery, modem, persist or any non-userdata eMMC partition.
