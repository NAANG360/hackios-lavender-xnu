# Chapter 5 — Post-logo kernel and Darwin userspace

This is the active roadmap after the Run #261 physical diagnostic-logo milestone. Keep the XNU kernel, genuine AppleLogo artwork, storage/root device, launchd, graphics/input services and SpringBoard as separate acceptance gates.

## 1. Genuine AppleLogo asset — extraction succeeded, XNU build integration pending

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

**CI status:** Run #267 failed before compilation could proceed because shell parameter expansions in `prepare-lavender-applelogo.sh` were backslash-escaped and `mktemp` received a literal `${TMPDIR:-/tmp}`. Commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` removes the erroneous backslashes. Run #268 (`43805363291`) is validating that correction. Do not claim the authentic logo is on the phone until a new image passes build, linked verification, packaging and a later safe physical test.

The original resource came from Apple's firmware; the raster is not a substitute for the entire Apple boot UI or a proof of a fully booted iOS system.

## 2. Confirm the post-logo XNU frontier

The Run #261 manifest places the diagnostic logo after `arm_vm_init()`, then publishes a logo-completed marker. The user-supplied 20.317-second recording shows the diagnostic mark but does not expose stage 20 (`kprintf` startup complete), root mounting, launchd or a persistent userspace log.

Next:
- keep Run #261/#266 as known verified old-logo baselines;
- after the official-logo build passes, add/retain separate evidence for logo completion, stage 20, post-logo init and first BSD/root milestone;
- capture the exact last marker and time on the next safe test;
- do not infer scheduler, interrupt, SMP or userspace readiness from the logo.

The Run #261 package uses `cpus=1`. Keep secondary-core/SMP operation unclaimed until independently proven.

## 3. Root filesystem and storage — removable SDHC2 is confirmed in the exact Build 4 FDT

The inherited Sweet root flow is not portable:
- `tools/mkiosroot.sh` creates a large case-sensitive journaled HFS+ image containing the OS volume and SystemOS/AppOS cryptexes.
- `tools/sweet-root-tweaks.py` contains 20H392 userland bring-up edits; audit each before reuse.
- Sweet's `bsd/dev/sweet_ufs.c` (patch 0028) reuses a UFSHCI state left active by the Sweet bootloader, sends SCSI commands and exposes a BSD disk named `ufsroot0` (rooted with `rd=ufsroot0`).
- Lavender is not UFS. It has internal eMMC on SDHC1 and a separate removable SD host on SDHC2. Do not reuse Sweet UFS registers, SCSI protocol or `ufsroot0`.

### Exact Build 4 FDT — SDHC2 removable slot

The Build 4 OTA archive used by this project was checksum-verified before its `boot.img` and appended `qcom,sdm660` FDT were extracted. The actual embedded FDT contains:

**SDHC2 — removable SD card**
- Node: `sdhc@c084000`
- `reg = <0x00 0x0c084000 0x00 0x1000>`
- `compatible = "qcom,sdhci-msm-v5"`
- `status = "ok"`
- `bus-width = <4>`
- `cd-gpios` points to TLMM GPIO 54; status interrupt is mapped to GPIO 54.
- Host IRQ: GIC SPI 125; power IRQ: GIC SPI 221.
- Clock names: `iface_clk`, `core_clk`; GCC SDCC2 AHB/APPS clock IDs are 93/94 in the matching SDM660 downstream clock bindings.
- `vmmc-supply` maps to PM660L L5; `vqmmc-supply` maps to PM660L L2.
- The node includes distinct default/sleep pinctrl states and a 4-bit bus configuration.
- No CQE/ICE resource is assigned to SDHC2 in this FDT node.

**SDHC1 — internal eMMC (do not use for the iOS root)**
- Base: `0x0c0c4000`, 8-bit bus, `non-removable`, CQE/ICE resources.
- Host/power IRQs 110/112.
- This is Android's internal storage. Keep it untouched.

This is stronger than the public mainline device-tree lead because it is the actual FDT embedded in the Build 4 boot image. It confirms the slot/controller description, not that a card is physically inserted, powered or readable at XNU entry.

### SDHCI MSM V5 power sequencing — not optional

The downstream Xiaomi SDM660 kernel's `drivers/mmc/host/sdhci-msm.c` selects the V5 register layout with these power-control registers relative to the controller's V5 core base:
- `CORE_PWRCTL_STATUS = 0x240`
- `CORE_PWRCTL_MASK = 0x244`
- `CORE_PWRCTL_CLEAR = 0x248`
- `CORE_PWRCTL_CTL = 0x24c`

The power IRQ is not merely a notification. Its handler:
- acknowledges/clears the latched status and orders register writes;
- on BUS_ON, enables VMMC, selects active pinctrl and sets the requested VQMMC level, then sends BUS_SUCCESS/BUS_FAIL;
- on BUS_OFF, disables the rail/selects sleep pins and acknowledges the result;
- on IO_LOW/IO_HIGH, changes the I/O rail and acknowledges success/failure;
- updates the vendor pad power-switch bit after the request is serviced.

Therefore, do not just map the host and issue SD commands. First determine controller/clock/power state at XNU entry. The bootloader's use of eMMC does not prove it initialized SDHC2 or enabled the removable-card rails.

Research source: [Xiaomi SDM660 downstream SDHCI MSM driver, branch 11-EAS](https://github.com/xiaomi-sdm660/android_kernel_xiaomi_sdm660/blob/11-EAS/drivers/mmc/host/sdhci-msm.c). Treat Linux as a hardware-behavior reference, not code to copy wholesale into XNU.

### Non-destructive driver plan

1. **Read-only discovery/probe first.** Map SDHC2 and inspect standard SDHCI version/capability/present-state/clock/interrupt registers, V5 CORE_MCI_VERSION and CORE_PWRCTL status/mask/control, GPIO54 and relevant FDT nodes. Do not write controller, clock, TLMM or PMIC registers in this first probe. Record whether ABL left the host/rails usable.
2. Audit the exact SDHC2 pinctrl states, PM660L L5/L2 regulator nodes, clock provider/IDs and IRQ mapping. Determine how XNU can safely service power IRQ 221 and host IRQ 125; do not assume a generic GIC handler alone is sufficient.
3. Implement the V5 power-event handshake and rail/pinctrl transitions with bounded timeouts and explicit error acknowledgements. Preserve the controller's interrupt ordering/barrier requirements. Keep initial signalling at the safe 3.3-V/low-speed mode supported by the actual board; do not request 1.8-V UHS until voltage switching is implemented and verified.
4. Add standard SD-card protocol and a **polled PIO read-only path**: card detect, CMD0, CMD8, CMD55/ACMD41, CID/CSD, RCA/select, block-length where needed, then CMD17 single-block reads. Start with no DMA, CQE, ICE, tuning, write commands or hotplug claims. Respect command/data inhibit bits, response types, buffer-ready and timeout/error bits.
5. Verify sector reads against known GPT and HFS+ metadata from a test card image. Only then expose a Lavender-specific BSD block device (for example `sdroot0`) with writes refused unconditionally. Root selection should be a separate `IOFindBSDRoot` case such as `rd=sdroot0`, not a reuse of Sweet's UFS path. Expose `bdevsw`/`cdevsw` and devfs nodes only after media capacity and partition bounds are known.
6. Mount HFS+ read-only, verify the journal/volume metadata and SystemOS/AppOS cryptex and dyld shared-cache paths against the 20H392 root tree. No write support until a separate non-Android target is approved.

### RAMDisk capacity — full root cannot be appended

The Lavender ADT exposes one conservative contiguous DRAM aperture from `0xA2000000` to `0xEA848000` (panic-log area excluded). The kernel ends near physical `0xA4020000`, leaving around `0x46828000` bytes (~1.10 GiB). `tools/mkiosroot.sh` defaults to a 16 GiB logical HFS+ image. It cannot fit as a kernel-appended RAMDisk, even if the host-side file is sparse.

`tools/mkpayload.py` now checks a page-aligned RAMDisk's logical size against the ADT aperture before reading it. The shim workflow's ADT tests pass, including a synthetic fitting image and rejection of the 16 GiB image. Keep this guard. Do not overlap kernel, panic area, framebuffer or other reserved memory.

The main viable design is a large removable SD-card root. A small initramfs may still fit in the RAMDisk tail, but only after its exact size is validated.

## 4. Darwin userspace boot sequence

The broad sequence is:
`XNU → root block device/HFS+ → dyld + shared cache → launchd → early system daemons → backboardd → SpringBoard`.

After a read-only root mounts:
1. Verify the root is the exact iPhone X 20H392 tree and the SystemOS/AppOS cryptex contents and dyld cache paths resolve.
2. Audit `sweet-root-tweaks.py` for Lavender relevance; keep only individually justified changes.
3. Capture launchd's first logs and resolve missing symbols, trust-cache/AMFI, entitlements, sandbox and platform-service dependencies one at a time. Do not disable entire subsystems blindly.
4. Stabilize essential early daemons before attempting `backboardd`.
5. Treat SpringBoard/lock screen as a later milestone, after graphics and input services exist.

## 5. Device-facing services after root

Further Lavender-specific gaps:
- **Display:** framebuffer console/logo works, but an iOS IOMobileFramebuffer/IOFramebuffer service is not established.
- **Touch:** Lavender uses Novatek `novatek,NVT-ts` on I²C address `0x62`, reset GPIO 66 and IRQ GPIO 67. Sweet patches 0060/0062 are Goodix/GENI-specific and must not be reused. No Lavender NVT driver is present in the inspected source.
- **I²C/TLMM/PMIC:** controller, GPIO and rail sequencing must come from Lavender's FDT/vendor sources.
- **Buttons/recovery:** power button is broken; volume-key chord is not a verified recovery selector.
- **SMP:** keep `cpus=1` until secondary CPU startup and synchronization are verified stage by stage.
- **Wireless, cellular, audio, camera, GPU and SEP-dependent functions:** unsupported until the required Lavender hardware/services are implemented.

## Acceptance ladder

1. Run #261 build, verification and package — passed.
2. Physical Run #261 image — diagnostic logo stage observed in user video; not image-checksum-bound.
3. Run #266 build, verification and package — passed; not separately physically tested.
4. Official 20H392 AppleLogo — extracted and hash-verified; patch 0114/build integration pending Run #268.
5. XNU stage 20 and post-logo kernel continuation — not physically proven.
6. SDHC2 read-only probe and removable-card PIO reads — not implemented.
7. BSD block device and GPT/HFS+ root mount — not done.
8. iOS 20H392 cryptexes/dyld and launchd/early daemons — not done.
9. backboardd, IOMobileFramebuffer, input and SpringBoard — not done.

No handset retest until a power-button-independent recovery/reset route is physically verified. RAM-only boot only; never flash, erase, repartition or write Android userdata/internal eMMC.
