# Troubleshooting

## eMMC CMD24/CMD25 timeouts

**Symptom:** the SYSTEM log ring is empty or header-only; the kernel log shows:

```
WHYRED SDHCI: command 24 failed (arg 0x00220000, INT_STATUS 0x00018000 cmd-timeout, ...)
WHYRED SDHCI: recover: stopping an open-ended transfer
WHYRED SDHCI: recovery incomplete (5)
```

**Cause:** `sdhci_write_pio()` returned after the controller's transfer-complete, but the card was still programming and held DAT0 busy. The next CMD24/CMD25 arrived too early.

**Fix:** patch `0192` adds `sdhci_wait_write_ready()` after `sdhci_wait_transfer_complete()`:

1. delay 1 ms;
2. poll `PRESENT_STATE` until `!CMD_INHIBIT && !DATA_INHIBIT && DAT0` is high;
3. send CMD13 and require `TRAN` + `READY_FOR_DATA`;
4. on timeout, recover and return `ETIMEDOUT`.

The working timeout is **30 s** (`CACHE_FLUSH_TIMEOUT_NS`). A 1 s variant (`CMD6_BUSY_TIMEOUT_NS`) was rejected: it stopped at `cc_fips_test` because some writes need more than 1 s.

**Verify:** the ring header should show a growing `total` and `flush #N`; the log should contain `WHYRED LOG: first flush ok` and no CMD24/25 timeouts.

## The first boot takes forever (DataMigration)

**Symptom:** Apple logo + progress bar for tens of minutes; the log shows `com.apple.datamigrator` and `Early boot complete`, then the bar barely moves.

**Cause:** a fresh iOS root image has no migrated data. `DataMigration` runs on first boot. With `cpus=1` it takes ~30+ minutes and can crawl at 99 %. The boot UI is presented by `backboardd`; `SpringBoard` waits in `BKSSystemShellService _checkIn` until the migration ends.

**Fix / mitigation:**

- Use `cpus=4` (all Gold cores).
- Keep `com.apple.DataMigration.plist` with `LastSystemVersion = 20H392` in the root image. It lets later boots skip the migration; the first boot still migrates.
- Let the first boot finish. Do not reboot in the middle; the migration restarts.
- If it never advances, check `mount: data volume missing` and the data volume/`preboot` setup. The port has a single HFSX root; `/private/var` is a directory, not a separate APFS data volume.

**Whyred reference:** the whyred notes (2026-10-17) document the same gate and the v10 image with the plist. Without the plist the migration repeated on every boot.

## Only one CPU is active

**Symptom:** `SWEET CPU: requested=1 selected=1 (limit=8)`.

**Fix:** add `cpus=4` to the shim command line. The Silver A53 cores cannot run 16 KiB pages; do not try `cpus=8` with the 16 KiB kernel. The safe fallback is `cpus=1`.

## GPU / `IOAcceleratorES`

**Symptom:** `backboardd` repeatedly matches `IOAcceleratorES`; the UI is slow or a frame stalls.

**Cause:** Adreno 512 has no iOS driver. The kernel publishes an **empty** `IOAcceleratorES` service so `CAMetalContextCreate` does not wait 30 s. Rendering is software (`AppleCLCD` + `IOSurface`).

**What to do:** this is expected. The lock screen is reached with software rendering. GPU acceleration is a separate project (Mesa/freedreno, see topaz). Do not remove the empty service without providing a real accelerator; QuartzCore will wait 30 s per process.

**`SWEET FB: swap` stops after a few lines:** that is the logging, not the compositor. The detailed line is printed only while `sweet_fb_swaps_logged < 16` (one line per presented layer) and the total counter only every 1000 commits, so an idle lock screen (one commit per minute for the clock) produces no lines after boot. Patch `0215` logs the first `sweetfblog` commits (default 2000) with the ring's uptime stamp and every change of the layer set; use it before concluding that the UI froze.

## `CRASH PosterBoard` when the lock screen appears

**Symptom:** in some boots, right after the display-off that enters the lock state, the ring shows `exec PosterBoard` and then:

```
CRASH PosterBoard[238] sig 5 code 0x600000001 sub 0x211eb0268 slide 0x50738000 mh 0x104868000
CRASH PosterBoard pc 0x211eb0268 lr 0x211eb0268 sp 0x16b61d0a0 fp 0x16b61d230 far 0x22d73ba40
CRASH PosterBoard bt 0x211eb6304 0x211e9cc5c 0x211e9c644 0x211f1e204 0x1f3df1318 0x1ec0eb72c ...
CRASH PosterBoard reason ns 2 code 0x5: (none)
exec PosterBoard
```

`sig 5` is SIGTRAP and `code 0x600000001` is `(EXC_BREAKPOINT << 32) | EXC_ARM_BREAKPOINT`: a deliberate `brk`, not a bad pointer, and not Metal (nothing in the path touches `IOAcceleratorES`).

**Symbolication (iPhone X, 20H392):**

```bash
# 1. the dyld shared cache lives in the Cryptex1,SystemOS DMG (087-86593-047.dmg),
#    System/Library/Caches/com.apple.dyld/ (main file + .01..42 + .symbols)
ipsw extract --dyld --dyld-arch arm64 iPhone10,3_16.7.16_20H392_Restore.ipsw
ipsw dyld info dyld_shared_cache_arm64          # needs the .symbols file
# 2. unslid = pc - slide, then resolve
ipsw dyld a2s dyld_shared_cache_arm64 0x1c1778268 -i -d
ipsw dyld symaddr dyld_shared_cache_arm64 -i /System/Library/PrivateFrameworks/PosterBoard.framework/PosterBoard
```

On Windows, `7z x` extracts the cache out of the cryptex DMG; `ipsw extract --dyld` needs Linux/macOS (it mounts APFS). The addresses for this build:

| slid | unslid | image + offset | symbol |
| --- | --- | --- | --- |
| `0x211eb0268` | `0x1c1778268` | PosterBoard.framework + `0x1d268` | `-[PBFPosterExtensionDataStore _stateLock_setupSwitcherConfigurationForFirstTime] + 2040` |
| `0x211eb6304` | `0x1c177e304` | PosterBoard.framework + `0x23304` | `_stateLock_updateExtensions:refreshDescriptors:powerLogReason:galleryUpdateOptions:queuedUpOperations:` + 5308 |
| `0x211e9cc5c` | `0x1c1764c5c` | PosterBoard.framework + `0x9c5c` | `executeUpdate:refreshStrategy:galleryUpdateOptions:powerLogReason:completion:` + 1508 |
| `0x211e9c644` | `0x1c1764644` | PosterBoard.framework + `0x9644` | `prewarmWithCompletion:` + 144 |
| `0x211f1e204` | `0x1c17e6204` | PosterBoard.framework + `0x8b204` | `PBFPosterExtensionDataStoreXPCServiceGlue server:prewarmWithCompletion:` + 64 |
| `0x1f3df1318` | `0x1a36b9318` | PosterBoardServices.framework + `0xe318` | `-[PRSServer prewarmWithCompletion:]` + 164 |

The trap is the function's entry assertion `_stateLock_switcherConfiguration == nil` (the ivar is at offset `0x98`, read from `_OBJC_IVAR_$_PBFPosterExtensionDataStore._stateLock_switcherConfiguration` in the cache): it is already non-nil, so the "first time" path logs an error and executes `brk #0`. The property's getter (`-[PBFPosterExtensionDataStore _stateLock_switcherConfiguration]`) lazily builds and caches an empty configuration (`_stateLock_buildSwitcherConfigurationWithContext:outMutated:`), so any read of it before the first extension update leaves the data store in exactly the state the setup function refuses. The state is persisted in `/var/mobile/Library/PosterBoard` on the HFSX root (not in the root image), and SpringBoard's `prewarm` XPC arrives when the display goes off into the lock screen.

**It is state-dependent:** `sweetlog-0203-touch-hid` crashed about ten minutes after `exec PosterBoard`, while the live log of a 47-minute boot with manual touches (kernel `0206`, 2026-11-20) never printed `CRASH PosterBoard` and SpringBoard kept answering AppleKeyStore and selector calls. The failing state lives in PosterBoard's data store on the HFSX root, so it depends on what earlier boots wrote; do not treat "no SWEET FB: swap lines" or one crash as proof that the renderer froze.

**What to do:** the poster process dies with the lock transition, so start by isolating it: boot with `sweetnopb=1` (patch `0217`), which makes its exec fail with EPERM and logs `SWEET EXEC: refusing PosterBoard`. If the lock screen then stays alive, the gate is PosterBoard's service; options are resetting its data store (delete `/var/mobile/Library/PosterBoard` from a writable root), or the userland workarounds bergamota uses for the same HFS-without-data-protection problem (`refs/bergamota/tools/sweetnet/sweetfp.m`, `sweetpb26.m`: swizzle the poster data-store checks instead of fixing the filesystem).


## Add New Wallpaper is empty or stays on a loader

Verified on Lavender with iOS 16.7.16 (20H392) on 2026-11-21. The physical iPhone X on the same
build, inspected through Xcode 16.2, launched `com.apple.WallpaperKit.CollectionsPoster` and
loaded the default wallpaper `7400`.

1. **Check the plugin records and the extension point.** `lsd` reported `Unable to find AMFI`
   while opening `/System/Library/xpc/extensions.plist`. Moving the precompiled catalog aside
   enabled native enumeration. After backing up and reseeding both LaunchServices stores,
   `lsd` logged `registered app extension` for `GradientPosterExtension` and `CollectionsPoster`.
   The records survived a shim change. A subsequent direct query still returned `-10814` for
   `com.apple.posterkit.provider`, even as `mobile`. Adding
   `/System/Library/ExtensionKit/ExtensionPoints/com.apple.posterkit.provider.appexpt` using the
   original `NSExtensionSDK` definition from `PosterKit.framework/Info.plist` resolved that query
   for iOS (platform 2). The definition retains the private extension point and the required
   `com.apple.runningboard.posterkit.host` entitlement. After restarting `lsd`, `pkd`, and
   PosterBoard, both providers started. The trial changes were made over SSH on the iOS root in
   `userdata`. The explicit declaration was added after the shim change.

2. **Check the Home Screen preview separately.** The `SIGABRT` from
   `-[PBFHomeScreenPreviewImageBuilder _imageForView:]` was an Objective-C exception when creating
   an options dictionary with a nil `kCARendererMetalCommandQueue`. A probe confirmed that Metal
   returned neither a device nor a queue on Lavender. The
   [CPU preview fallback](../tools/posterboard-preview.m) renders the view's layer at its original
   scale when a Metal queue is unavailable. It generated valid previews through the actual
   preview method in a standalone process and was then installed in the running image.
   Opening the gallery after the changes produced a **Color** preview, with no recurrence of that
   `SIGABRT` in the observed log. Applying the color and verifying the lock and Home screens
   remain pending.

3. **Check extension startup if the loader persists.** The live log shows both Astronomy
   (`AegirPoster`) and `CollectionsPoster` repeatedly terminated with
   `extension check-in timeout`. The framebuffer continued to swap and touch events continued
   to arrive. The stock Astronomy executable links Metal. The cause of its check-in timeout
   has not yet been isolated, and the CPU fallback covers the PosterBoard preview method.

The live trial preserved the original catalog as `extensions.plist.wallpaper-trial-stock` in
`/System/Library/xpc`, the previous LaunchServices stores in
`/private/var/tmp/lavender-ls-stock.Sp5KpG`, and the original PosterBoard executable in
`/private/var/tmp/posterboard-live-trial.cVrax8/PosterBoard.stock`. The scoped extension-point
declaration and CPU preview library were installed on the Redmi after the initial catalog-only
trial image was built.

## Touch does not work

**Cause:** the Novatek NT36672 driver is not implemented. The series has probe/stub patches (`0184`–`0188`) but no finished Novatek protocol, no PM660 LDO11 power-up and no `SweetHIDTouch` integration.

**Next:** implement the Novatek I2C protocol on QUP1 (`0x62`), GPIO 66 reset / GPIO 67 IRQ, PM660 LDO11, then feed events to `SweetHIDTouch`. Check the panel variant: Synaptics uses `0x20`.

## Physical buttons do nothing / USB is not detected

**Cause:** on kernels without patches `0200`/`0201`, the keys are not read and the USB device driver never starts. With the patches, check `SANTONILOG1`:

- `SWEET KEYS: unsupported arbiter version=…`: the arbiter is not the expected v3 layout. `… volume-up withheld`: the PM660L GPIO7 is not an enabled input. `read error`: the SPMI reads fail.
- `SWEET USB: controller unpowered or unclocked` with `sweetusb=1`: ABL left USB off; retry with `sweetusb=3`. `GDSC … TIMEOUT` or `clk … STILL OFF` with `sweetusb=3`: the GCC alone is not enough.
- No `SWEET KEYS`/`SWEET USB` lines at all: the kernel does not have `0200`.
- The host sees no network: Windows has no CDC-ECM driver. Use macOS/Linux and set 192.168.7.1/24 on the USB interface.

There is still no `adb`/`fastboot` while iOS runs.

**Recovery:**

1. Hold **Power** 20–30 s (PMIC hardware reset).
2. Try **Power + Volume Down** 20–30 s, then **Power + Volume Up** 20–30 s.
3. If nothing works, unplug the charger and let the battery drain. The eMMC root is not lost.
4. When off, charge and boot with **Power + Volume Up**.

Do not use EDL/test points unless you know the hardware; the phone is not bricked.

## Lock screen date is 1969 / colours look red

- The orientation is correct (portrait, upright). The red/orange date, clock and status bar in the lock screen photo are not explained yet: wallpaper tint or framebuffer colour format.
- The PM660 RTC is not read, so `PEGetUTCTimeOfDay()` answers 0 and iOS starts at 1970-01-01 (the lock screen shows 1969-12-31 16:MM PST plus uptime). Patch `0216` takes `sweetrtc=<unix seconds>` as the calendar base when the platform clock answers 0; the shim build passes its own build time (`EXTRA_CMDLINE="sweetrtc=$(date +%s)"`). Look for `SWEET RTC: calendar base` in the ring and for a 2026 date on the lock screen. The proper fix remains reading the PM660 RTC over SPMI (see `sweet_keys.c` for the arbiter) or letting iOS sync time.
- The clock also needs the UI to keep committing frames: if it is stuck, check for `CRASH PosterBoard` and for the swap telemetry (`0215`) before blaming the RTC.

## `launchd` dies with `CS_KILLED`

Use the bring-up flags:

```
cs_enforcement_disable=1 unentitled_ios_sim_launch=1 launchd_unsecure_cache=1
```

Without them, the unsealed root and the iOS platform binaries are killed.

## `libignition: failed to lookup preboot: 25`

The preboot volume is missing. The boot continues unsealed; whyred reached its home screen with the same message. It is not the current lock-screen gate. A proper preboot/data volume is future work.

## `securityd sel 10 -> 0xe00002c7`

`0xe00002c7` is `kIOReturnUnsupported`. Selector 10 is `kAKSWrapKey`; the built-in `AppleKeyStore` stub does not implement it. The boot continues. If it becomes a gate, implement `WrapKey`/`UnwrapKey` in the AKS stub.

## `CRASH driverkitd`

Known userspace crash (`sig 9`, reason namespace 1 code 0x7). It does not stop the boot. Do not chase it before touch/buttons/USB.

## `fastboot boot` does nothing / MIUI restores recovery

- `fastboot boot` is stubbed by this ABL: it returns `OKAY` but does not execute the image.
- `fastboot reboot recovery` boots Android and MIUI restores the stock recovery.
- Use `fastboot flash recovery <shim>` and **Power + Volume Up**.

## USB link drops during `userdata` flash

A long sparse flash can drop the link (`????????????`, "no link"). Reconnect the cable, re-enter fastboot and retry. Do not disconnect while the flash is in progress.

## Pulling the log

```bash
adb push tools/pull-sweetlog.sh /tmp/
adb shell sh /tmp/pull-sweetlog.sh
adb pull /tmp/sweetlog.bin sweetlog.bin
python3 tools/sweet-log.py sweetlog.bin boot.txt
```

`tools/sweet-log.py` accepts the raw 4,194,816-byte area. If sector 0 is blank, use `--salvage` only when the ring start really looks like a kernel log.
