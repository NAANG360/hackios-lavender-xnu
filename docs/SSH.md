# SSH over USB

A root shell on the phone over the USB network link (patch `0213`, CDC-NCM): useful for `log stream`, `ps`, copying files with `scp` and changing the root without reflashing.

- Server: dropbear 2026.94, key-only login as root (`-s`: no passwords), port 22.
- Shell: dash 0.5.13.5 as `/bin/sh`. Tools: toybox 0.8.15 in `/usr/local/bin` (ls, ps, cat, grep, find, tar, vi, ...).
- Started by launchd: job `org.sweet.dropbear`, in `System/Library/xpc/launchd.plist` (read with `launchd_unsecure_cache=1`) and `System/Library/LaunchDaemons`.
- Address: the phone is `192.168.7.2`, the PC `192.168.7.1` (`tools/usbnet-windows.ps1`).

## Build the image (macOS)

1. Build the binaries with bergamota's `tools/ssh/build.sh` (needs Xcode, GNU sed as `gsed`, network for the sources; it checks their SHA-256). It leaves `research/ssh-build/out/{dropbear,dash,toybox}` for iOS 16 arm64, ad hoc signed with `platform-application`, `no-container`, `no-sandbox` and `skip-library-validation`, plus `dropbear-mac` and `toybox-mac`.
2. Make a key for the phone on the PC and copy only the **public** half to the Mac:

   ```powershell
   ssh-keygen -t ed25519 -N '""' -C lavender-ios16 -f $HOME\.ssh\id_ed25519_lavender
   scp $HOME\.ssh\id_ed25519_lavender.pub <mac>:lavender-keys/
   ```

3. On the Mac, with this repo's `tools/sweet-ssh.py` and `tools/ssh-root-image.sh` next to each other:

   ```bash
   sudo bash tools/ssh-root-image.sh ~/ios16-root.img ~/ios16-root-ssh.simg ~/lavender-keys/*.pub
   ```

   It mounts the image, runs `sweet-ssh.py` and writes the sparse image (`img2simg.py`). `sweet-ssh.py` also adds a static configd service for `ue0` (192.168.7.2/24 via 192.168.7.1), so configd keeps the address the kernel set, and creates the BindFS / factory-data directories the boot log reports missing. It is idempotent.

## Flash and connect

```powershell
fastboot -S 512M flash userdata ios16-root-ssh.simg
```

Flashing userdata replaces the whole iOS data volume: Setup runs again. Boot a shim with `sweetusb=2` (USB network), wait for the lock screen, then:

```powershell
powershell -ExecutionPolicy Bypass -File tools\usbnet-windows.ps1
ssh -i $HOME\.ssh\id_ed25519_lavender root@192.168.7.2
```

The host key is generated on the Mac when the image is built. A new image has a new host key, so remove the old line with `ssh-keygen -R 192.168.7.2`.

## Useful commands on the phone

```sh
launchctl list | grep -i widget
ps -A
ls /private/var/db/diagnostics      # unified log (os_log) stores: copy with scp, read with `log show` on a Mac
```

If `ssh` times out: check `ping 192.168.7.2`, then the boot log for `org.sweet.dropbear` (dropbear writes to `/dev/console`).
