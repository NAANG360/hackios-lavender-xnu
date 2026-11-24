# Build and flash

This repository does **not** ship Apple binaries, the IPSW, the root image, the stock ROM or the kernel artifact. You must obtain/build them yourself. The commands below are the ones that produced the current lock-screen boot.

## Prerequisites

- A GitHub account with `gh` authenticated (the CI of this repository builds the kernel).
- An iPhone X (`D22AP`) iOS 16.7.16 (20H392) IPSW, verified by SHA-256.
- Windows/Linux with `fastboot` and `adb`.
- macOS with Xcode Command Line Tools for the root image (`hdiutil`, `python3`), or Linux with `apfs-fuse`/`hfsprogs`.
- Docker (or clang/LLD/LLVM) for the shim build (`shim/build.sh`).
- A lavender unit with an unlocked bootloader, verified backups and the stock ROM.

## 1. Kernel

The full patch series is in `xnu/patches/` (see `xnu/README.md`). `.github/workflows/xnu.yml` builds `kernel.development.lavender` on `macos-15`; Xcode 15's `iig` matches the xnu-8796 IORPC ABI. The workflow:

1. runs `tests/run_tests.sh` and the `tests/host/` USB/keys tests;
2. fetches `blacktop/darwin-xnu-build` at a pinned commit and copies `xnu/patches/*` into it;
3. builds with `MACHINE_CONFIG=LAVENDER` and fails if a critical patch (`014x`–`020x`) was skipped.

A push to `main` that touches `xnu/patches/**`, `resources/**` or the workflow starts it. A manual run can choose the build type.

```bash
gh workflow run xnu.yml --ref main -f machine_config=LAVENDER
gh run list --workflow=xnu.yml
gh run watch <run-id>
gh run download <run-id> -n xnu-kernel -D xnu/out
```

The lock-screen kernel (`0192`) was built by the same workflow from a fork of `ios-lavender`.

First green build of this repository: run [`44734617963`](https://github.com/pcorezzola/lavender-ios16/actions/runs/44734617963), commit `853fb45`, series `0001`–`0192` + `0200` + `0201` + `0202`. Not yet booted on the device.

| | |
| --- | --- |
| File | `kernel.development.lavender` |
| Size | 17,836,416 bytes |
| SHA-256 | `a089f246fa3d23e17c1c7cfe35140ccdf9bd6d0bc9314516446e3106f5af6af4` |
| Checks | Mach-O arm64; no LSE/LDAPR (`llvm-objdump --mattr=+v8.5a`); carries the `0200`/`0201` driver strings |

A shim built from it with the lock-screen command line (the `0192-lite-cpus4` boot args, identical) is 6,713,344 bytes, SHA-256 `92e7d218468301eabdf77a529c6dcc538781112deced4165058cca72805d242d` (not distributed).

Verify the artifact:

```bash
file xnu/out/kernel.development.lavender
# Mach-O 64-bit arm64 executable

llvm-objdump -d --mattr=+v8.5a xnu/out/kernel.development.lavender \
  | grep -P '\t(cas|ldadd|ldset|ldclr|swp|ldapr)' || echo "no LSE"
```

The known-good kernel from the lock-screen boot is:

| | |
| --- | --- |
| File | `kernel.development.lavender` |
| Size | 17,836,144 bytes |
| SHA-256 | `793d232f7c56be8e351e2fe6246b21515f20835c508e2cdbb72016d3830a87be` |
| Patch | `0192` with the 30 s `CACHE_FLUSH_TIMEOUT_NS` |

The 1 s variant (`CMD6_BUSY_TIMEOUT_NS`, kernel `2a6b5421…`) was rejected: it stopped at `cc_fips_test`.

## 2. Shim

The shim packs the kernel, the product ADT and the boot arguments into an Android boot image. `shim/build.sh` builds it; the lavender board uses `resources/lavender_dtb.bin` and `tools/mkadt.py --board lavender`.

```bash
BOARD=lavender shim/build.sh /path/to/kernel.development.lavender
# -> shim/out/shim-boot.img
# or, with the known-good boot arguments:
tools/build-shim.sh /path/to/kernel.development.lavender
```

The current working command line (inside the payload, not the boot.img header):

```
debug=0x14e serial=1 disable_kprintf_output=0
sweetfb_native=1 sweetcpufreq=0 sweetbattery=0
rd=emmcroot0 -v keepsyms=1 lavender
sweetlog=1 emmcrw=1 rootrw=1
cs_enforcement_disable=1 unentitled_ios_sim_launch=1 launchd_unsecure_cache=1
wdt=-1 wdt_test=no_user_checkin
sweetfb_noconsole=1 sweetfb_stayon=1 sweetfb_hz=30
cpus=4
```

Why these arguments:

- `rd=emmcroot0` — root is the HFSX image in `userdata`.
- `emmcrw=1 rootrw=1` — allow writes; without them the iOS root is read-only and `launchd` cannot finish.
- `cs_enforcement_disable=1 unentitled_ios_sim_launch=1 launchd_unsecure_cache=1` — keep the unsealed iOS userland alive during bring-up.
- `sweetlog=1` — 4 MiB persistent console ring on SYSTEM.
- `sweetfb_noconsole=1 sweetfb_stayon=1 sweetfb_hz=30` — hide the kernel console behind the GUI, keep the panel on, 30 Hz software composition.
- `wdt=-1 wdt_test=no_user_checkin` — do not let the watchdog reboot the bring-up.
- `cpus=4` — all four Gold A73 cores. `cpus=1` is the safe fallback; the Silver A53 cores cannot run 16 KiB pages.

Verify the payload command line and the board:

```bash
python3 tools/payload_cmdline.py --image shim/out/shim-boot.img \
  --require rd=emmcroot0 --require emmcrw=1 --require rootrw=1 \
  --require cpus=4 --require sweetlog=1
```

Known-good shim:

| | |
| --- | --- |
| File | `shim-boot-lavender-0192-lite-cpus4-2026-11-19.img` |
| Size | 6,709,248 bytes |
| SHA-256 | `ba282d7977a23189e084705606f71dbf2c226fbbc02475bc7891a6c4c44e9425` |
| Kernel | `793d232f…` (0192, 30 s) |
| ADT | product `d22ap.adt` from the iPhone X IPSW |

The 1 s timeout shim and the `cs_debug=2`/kprintf-spam shim are diagnostic builds, not the lock-screen build.

## 3. Root image (HFSX)

The root image is built from the iPhone X IPSW with `bergamota/tools/mkiosroot-macos.sh` and `tools/sweet-root-tweaks.py`. It is a 16 GiB HFSX image; `img2simg.py` converts it to the Android sparse image flashed to `userdata`.

```bash
# macOS, from the bergamota tree
sudo bash tools/mkiosroot-macos.sh \
  OS.dmg SystemOS.dmg AppOS.dmg ios16-root.img 16g
python3 tools/img2simg.py ios16-root.img ios16-root.simg
```

The base `sweet-root-tweaks.py` prepares the iOS root. This port adds one file to skip the system data migration on later boots:

```
/var/mobile/Library/Preferences/com.apple.DataMigration.plist
{
  "LastSystemVersion" => "20H392"
}
```

`tools/fix-datamigration.sh` mounts the raw image read-write, creates the plist (501:501, 0600), verifies it with `plutil`, unmounts and regenerates the sparse image. The first boot still performs the migration because the fresh root has no migrated data; later boots should skip it. See `docs/TROUBLESHOOTING.md`.

Known-good root images (not distributed):

| | |
| --- | --- |
| Raw image with plist | hash printed by `fix-datamigration.sh` (not distributed) |
| Sparse image flashed | `cec991082f4e7bf2ae30a5ebcabf75b096d44617f410a93a835b2fc324f971e0` |

## 4. Flash

Back up `persist`, `modemst1/2`, `fsg`, `fsc`, `boot`, `dtbo`, `vbmeta`, `recovery` and the GPT first. `userdata` will be overwritten; Android will not boot until the stock ROM is restored.

```bash
# 1) iOS root in userdata (sparse, several minutes)
fastboot -S 512M flash userdata ios16-root.simg

# 2) shim in recovery
fastboot flash recovery shim-boot-lavender-0192-lite-cpus4-2026-11-19.img
```

Then **do not** use `fastboot reboot recovery`: on this unit it boots Android and MIUI restores the stock recovery. Power off and boot with **Power + Volume Up**.

## 5. Logs

The shim writes the kernel console to a 4 MiB `SANTONILOG1` ring on the SYSTEM partition. Pull it from TWRP:

```bash
adb push tools/pull-sweetlog.sh /tmp/
adb shell sh /tmp/pull-sweetlog.sh
adb pull /tmp/sweetlog.bin sweetlog.bin
python3 tools/sweet-log.py sweetlog.bin boot.txt
```

The decoder prints the header (`version`, ring size, total bytes, flush count) and the reconstructed stream. A header-only log means the write path never flushed; a log with `CMD24`/`CMD25` timeouts means the old eMMC bug is back.

## 6. Verification checklist

- [ ] Kernel is arm64 Mach-O, no LSE instructions.
- [ ] Shim payload command line contains `rd=emmcroot0`, `emmcrw=1`, `rootrw=1`, `cpus=4`, `sweetlog=1`.
- [ ] `check_payload.py` accepts the kernel and the lavender DTB.
- [ ] Root image has `/var/mobile/Library/Preferences/com.apple.DataMigration.plist` with `20H392`.
- [ ] Backups and stock ROM are verified before touching `userdata`.
- [ ] The phone is charged; a `userdata` flash can take ~8 minutes.
