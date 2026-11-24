# Restore / rollback

This is a high-level runbook. The exact partition names and ROM layout are device-specific; verify them on your unit.

## What you need

- A verified stock fastboot ROM for `lavender` (the same region/version is safest).
- TWRP 3.7.0_9-0-lavender or a compatible recovery.
- The backups of `persist`, `modemst1`, `modemst2`, `fsg`, `fsc`, `boot`, `dtbo`, `vbmeta`, `recovery` and the GPT head/tail.
- A stable USB cable and a charged battery.

## Restore Android

1. Enter fastboot (Power + Volume Down) or TWRP → Reboot → Bootloader.
2. Verify the device:

```bash
fastboot devices
fastboot getvar product      # lavender
fastboot getvar unlocked     # yes
```

3. Run the stock ROM's `flash_all.bat` (Windows) or the equivalent `flash_all.sh` (Linux/macOS). It restores `boot`, `recovery`, `system`, `vendor`, `cust`, `dtbo`, `vbmeta` and `userdata`.
4. If you flash manually, **do not** erase `persist`, `modemst1/2`, `fsg`, `fsc` or the GPT. Those hold modem/IMEI/calibration data.
5. `userdata` currently contains the HFSX iOS root. The stock ROM's `flash_all` overwrites it with an Android filesystem. If you flash only `boot`/`system`, Android will not mount `userdata`; format it with the stock recovery or `fastboot format userdata` (this erases the iOS root).
6. Boot Android once and verify that the modem, IMEI and sensors are present. If not, restore `persist`/`modemst*` from the backups.

## Restore individual partitions from TWRP

If you only need to recover a partition:

```bash
adb push backup.img /tmp/backup.img
adb shell "dd if=/tmp/backup.img of=/dev/block/bootdevice/by-name/<partition> bs=1M"
adb shell sync
```

Double-check the target partition name and size. Never write a backup to the wrong block device.

## GPT

- Use the **live dump** of your unit's GPT, not the generic GPT from a stock ROM. The lavender layout in the wild differs between units/firmwares.
- The GPT head is at LBA 1 (offset 512) and the tail at the last LBA. Keep both.
- If the GPT is corrupted, restore the head/tail only if you are sure the layout matches; otherwise use the stock ROM's `gpt` images or a full flash.

## Recovering from the current iOS state

The current port has no working buttons and no USB gadget while iOS runs. If the device is stuck at the lock screen:

1. Hold **Power** 20–30 s (PMIC reset).
2. Try **Power + Volume Down** 20–30 s, then **Power + Volume Up** 20–30 s.
3. If nothing works, unplug the charger and let the battery drain.
4. When off, charge and boot with **Power + Volume Up** for the shim, or **Power + Volume Down** for fastboot.
5. To get TWRP back, flash `twrp-3.7.0_9-0-lavender.img` to `recovery` from fastboot.

## Verify

- `fastboot getvar product` is `lavender`.
- The stock recovery/boot boots.
- `persist`, `modemst1/2`, `fsg`, `fsc` are intact.
- The GPT head/tail signatures (`EFI PART` at 512 and at the last LBA) are present.
- Keep a copy of the backups off-device.
