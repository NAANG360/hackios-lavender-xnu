# Tools

| Script | Purpose |
| --- | --- |
| `pull-sweetlog.sh` | Run inside TWRP: dump the 8193-sector `SANTONILOG1` area from SYSTEM to `/tmp/sweetlog.bin` |
| `sweet-log.py` | Decode `sweetlog.bin` (header + 4 MiB ring) to text; `--salvage` for a blank header |
| `fix-datamigration.sh` | macOS/root: add `com.apple.DataMigration.plist` (`LastSystemVersion=20H392`) to the raw HFSX image and regenerate the sparse image |
| `usbnet-windows.ps1` | Windows host side of the USB network (patch `0213`): find the NCM adapter, set 192.168.7.1/24, open UDP 5140 on it, ping the phone, print/save the live kernel log (elevated PowerShell) |
| `build-shim.sh` | Build a lavender shim from a base checkout + kernel with the known-good boot args |

## Log pull

```bash
# Phone in TWRP, adb available
adb push pull-sweetlog.sh /tmp/
adb shell sh /tmp/pull-sweetlog.sh
adb pull /tmp/sweetlog.bin sweetlog.bin
python3 sweet-log.py sweetlog.bin boot.txt
```

A healthy header looks like:

```
# SANTONILOG1: version 1, ring 4194304, total 110468 bytes, flush #604
```

A header-only log (`total 0`, `flush #0`) means the eMMC write path never flushed.

## DataMigration helper

On the Mac, with the raw image at `~/ios16-root.img` and `bergamota` at `~/bergamota`:

```bash
sudo bash fix-datamigration.sh
# -> ~/ios16-root-dm.simg
```

It writes the plist to `/private/var/mobile/Library/Preferences` (the `/var` symlink resolves there), verifies it with `plutil`, unmounts and regenerates the sparse image. The first boot still migrates; later boots should skip.

## Shim

```bash
tools/build-shim.sh /path/to/kernel.development.lavender
# -> shim/out/shim-boot.img
```

The default command line is the lock-screen build: `rd=emmcroot0`, `emmcrw=1 rootrw=1`, the CS flags, `sweetlog=1`, the GUI switches and `cpus=4`. Override `EXTRA_CMDLINE` to test a different configuration.

`shim/build.sh` uses `resources/lavender_dtb.bin` (tracked) and the product ADT (`d22ap.adt` from the iPhone X IPSW). The ADT is Apple's and is not distributed here.

## Build and analysis tools (imported from ios-lavender `bc606b7`)

| Script | Purpose |
| --- | --- |
| `mkadt.py`, `adt.py` | Build the Apple device tree the kernel boots with (`--board lavender`) from the product ADT |
| `mkpayload.py`, `payload_cmdline.py`, `check_payload.py` | Pack the kernel + ADT + command line; read back and check an image's command line and board DTB |
| `mkbootimg.py`, `unpack_bootv2.py`, `extract_appended_dtb.py`, `prune_dtb.py` | Android boot image pack/unpack and DTB extraction/pruning |
| `mkinit.py`, `santoni-init.S`, `sweet-init.S` | The static Mach-O PID 1 used by RAM-disk test boots |
| `check_patch.py`, `extract_drivers.py` | Patch structure checks; regenerate the `drivers/` mirrors from `xnu/patches/` |
| `sweet-ssh.py`, `ssh-root-image.sh` | SSH over USB: dropbear, dash and toybox, keys and launchd job into the root image; rebuilds the sparse image on macOS (`docs/SSH.md`) |
| `sweet-root-tweaks.py`, `mkiosroot.sh`, `patch-launchd-plist.py`, `sparsediff.py` | Root image preparation |
| `sweet-log-pull.sh` | Host side: pull and decode `SANTONILOG1` over adb from recovery |
| `sweet-usbnet.sh` | Linux host side of the `0200` USB network: wait for the CDC-ECM interface, set 192.168.7.1/24, save the live log |
| `sweet-crash.py`, `xnu-cc.py`, `kbuild.sh`, `check-exports.py`, `fetch-ci-kernel.sh`, `zip_partial.py`, `remotezip.py` | Crash symbolication, local recompiles from the CI build tree, CI kernel download, partial IPSW fetch |
