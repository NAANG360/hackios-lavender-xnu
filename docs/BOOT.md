# Boot chain, logs and recovery

## Boot chain

```
Xiaomi ABL (unlocked)
  -> recovery partition: lavender shim (Power + Volume Up)
  -> shim copies the kernel + ADT + boot args, hands off to XNU
  -> XNU: GICv3, timers, 4 Gold CPUs, SDCC v5, framebuffer
  -> HFSX root from userdata (rd=emmcroot0)
  -> launchd -> mount-phase-1/2 -> backboardd -> SpringBoard
  -> iOS 16 lock screen
```

The shim is a minimal bootloader. It is **not** a full UEFI/DarwinBoot implementation; it prepares the CPU, the DT and the payload, then jumps to XNU.

## Flashing the shim

`fastboot boot <image>` is stubbed by this ABL: it returns `OKAY` but does not execute the image. The working sequence is:

```bash
fastboot flash recovery shim-boot-lavender-0192-lite-cpus4-2026-11-19.img
```

Then power off and hold **Power + Volume Up** until the Apple logo/shim console.

Do **not** use `fastboot reboot recovery`. On this unit it boots Android and MIUI restores the stock recovery. If that happens, flash the shim again and use the key combo.

## Boot arguments

| Argument | Purpose |
| --- | --- |
| `rd=emmcroot0` | Root device: HFSX image in `userdata` |
| `emmcrw=1 rootrw=1` | Allow eMMC writes / read-write root |
| `cpus=4` | Four Gold A73 cores; `cpus=1` is the safe fallback |
| `sweetlog=1` | 4 MiB persistent console ring on SYSTEM |
| `sweetfb_native=1` | Use the native panel canvas |
| `sweetfb_noconsole=1` | Stop drawing the kernel console over the GUI |
| `sweetfb_stayon=1` | Do not blank the panel during boot |
| `sweetfb_hz=30` | Software composition refresh target |
| `sweetusb=N` | Patch `0200`. `0` off; `1` (default) read-only USB probe; `2` CDC-ECM gadget + live log; `3` also powers the USB GDSC/clocks first (experimental) |
| `sweetusblog=0` | Keep the gadget, disable the UDP live log |
| `sweetusbhold=1` | Patch `0210`: USB test boot. Stop in `IOFindBSDRoot` after the USB/keys probe: no root mount, no userland, nothing written to `userdata`; USB, live log, keys and the SYSTEM log keep running, with a status line every 10 s. About a minute instead of a full boot |
| `sweetusbfix=N` | Patch `0211`: mask of the DWC3 bring-up steps (1 UTMI clock as PIPE clock, 2 core+PHY soft reset, 4 HS quirks); default 7, 0 = off. Bisect with one `sweetusbhold` boot per value |
| `sweetusbutmi=0` | Patch `0212`: keep the QUSB2 UTMI test mux as ABL left it (default: released) |
| `sweetusbncm=0` | Patch `0213`: CDC-ECM instead of CDC-NCM (NCM is the default on lavender: Windows 10 2004+/11 binds UsbNcm; macOS/Linux drive both) |
| `sweetrtcoff=N` | Patch `0220`: seconds added to the PM660 RTC raw count to get Unix time (Android never writes that RTC, it keeps its own offset). Without it the raw count is used only if it is already >= 2020-01-01; else `sweetrtc` (patch `0216`) gives the calendar. The raw value is logged as `SWEET RTC: PM660 raw …` |
| `sweetkeys=0` | Patch `0201`: disable the PMIC key poller (default on, read-only) |

## Live log over USB (patch 0200)

With `sweetusb=2` (or `3`) the phone appears as a USB Ethernet device (`1209:0001`). It configures itself as `ue0` 192.168.7.2/24 and every 100 ms streams the console as UDP to 192.168.7.1:5140, starting with the last 64 KiB.

With patch `0213` the function is CDC-NCM (default; `sweetusbncm=0` = CDC-ECM). Windows 10 2004+/11 binds its in-box UsbNcm driver. From an elevated PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File tools\usbnet-windows.ps1 -Log live.txt
```

It waits for the adapter, sets 192.168.7.1/24, allows UDP 5140 on that interface, pings the phone and prints the live console. On macOS/Linux:

```bash
sudo ip addr add 192.168.7.1/24 dev <usb-if> && sudo ip link set <usb-if> up   # Linux
nc -ul 5140            # live console
ping 192.168.7.2       # link check
```

After a panic, or before USB comes up, the recovery pull of `SANTONILOG1` is still the only path.
| `cs_enforcement_disable=1` | Do not kill the unsealed root |
| `unentitled_ios_sim_launch=1` | Allow the iOS platform binaries to exec |
| `launchd_unsecure_cache=1` | Skip the AMFI service lookup in the cache loader |
| `wdt=-1 wdt_test=no_user_checkin` | Keep the bring-up watchdog quiet |
| `serial=1` | Serial console (UART) |

The full line used for the lock-screen boot is in `docs/BUILD.md`.

## What a good boot looks like

```
Darwin Kernel Version 22.6.0
SWEET CPU: requested=4 selected=4 (limit=8)
SWEET CPU: 4/4 CPUs initialized, SGI1 verified
WHYRED SDHCI: enumeration complete: 25 MHz, 8-bit, high-speed
BSD root: emmcroot0
hfs: mounted iOSRoot on device b(2, 0)
exec launchd
WHYRED LOG: first flush ok (13300 bytes, header ok)
Early boot complete. Continuing system boot.
SWEET FB: display on
SWEET FB: swap 1..6
exec SpringBoard
```

A boot with no `CMD24`/`CMD25` timeouts and a growing `SANTONILOG1` total is healthy. The lock screen appears after the first-boot data migration.

## Log ring

- Location: first 8193 sectors of the SYSTEM partition.
- Sector 0: `struct santoni_log_header` (36 bytes): magic `SANTONILOG1`, version 1, ring size 4 MiB, total bytes, flush sequence.
- Sectors 1..8192: the circular ring.
- The kernel appends every console character; the flusher writes whole 512-byte blocks.

Pull it from TWRP:

```bash
adb push tools/pull-sweetlog.sh /tmp/
adb shell sh /tmp/pull-sweetlog.sh
adb pull /tmp/sweetlog.bin sweetlog.bin
python3 tools/sweet-log.py sweetlog.bin boot.txt
```

If the header says `total 0` or `flush #0`, the eMMC write path did not flush. If it has `CMD24`/`CMD25` `cmd-timeout`, the old SDHCI bug is present (see `docs/TROUBLESHOOTING.md`).

## Clock (patches 0216, 0220)

The PM660 RTC keeps counting while the phone is off, but Android never writes it: its raw count is not Unix time. Measure this phone's offset once, in TWRP, against the PC clock:

```bash
echo $(( $(date -u +%s) - $(adb shell cat /sys/class/rtc/rtc0/since_epoch | tr -d '\r') ))
```

and build shims with it (`SWEETRTCOFF=<offset> tools/build-shim.sh <kernel>`, which also adds `sweetrtc=<build time>` as the fallback). The boot log shows `SWEET RTC: PM660 raw <count> s ..., sweetrtcoff <offset> -> <unix time>`. The offset stays valid while the battery keeps the RTC running. On the test unit it is `1564989948` (raw `230220074` at Unix `1795210022`).

## Recovery when iOS is stuck

Power and the volume keys reach iOS (patch `0201`), and with `sweetusb=2` the USB network and live log work (patches `0210`–`0213`). If the device is still stuck:

1. Hold **Power** for 20–30 seconds (PMIC hardware reset).
2. Try **Power + Volume Down** 20–30 s, then **Power + Volume Up** 20–30 s.
3. If nothing works, unplug the charger and let the battery drain. The iOS root is on eMMC and is not lost.
4. When it powers off, charge it and boot with **Power + Volume Up**.

To get back to TWRP, flash `twrp-3.7.0_9-0-lavender.img` to `recovery` from fastboot. TWRP can read the SYSTEM log area and the raw partitions.

## First boot vs later boots

- **First boot on a fresh root image:** `DataMigration` runs and can take tens of minutes. With `cpus=4` it is faster than the `cpus=1` reference, but still slow. Let it finish.
- **Later boots:** the image carries `com.apple.DataMigration.plist` with `LastSystemVersion = 20H392`; `DataMigration` should skip. The second-boot check is still pending; it needs the `0201` buttons confirmed on the device.
- If the migration never completes, the likely cause is the missing data volume (`mount: data volume missing`) or a migration that cannot record completion. See `docs/TROUBLESHOOTING.md`.

## Known non-fatal messages

- `libignition: failed to lookup preboot: 25` — the preboot volume is missing; the boot continues unsealed. whyred reached its home screen with the same message.
- `mount: data volume missing, but not required in env: 0` — the port uses the root HFSX for `/private/var`; no separate APFS data volume.
- `SWEET AKS: securityd sel 10 … -> 0xe00002c7` — `WrapKey` is not implemented in the AKS stub; `kIOReturnUnsupported`.
- `CRASH driverkitd` — known userspace crash; it does not stop the boot.
- `CODE SIGNING: cs_validate_page … no hash to validate` — expected with the unsealed root; not fatal.
