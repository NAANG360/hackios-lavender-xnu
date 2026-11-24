# Roadmap

The port boots to the iOS 16.7.16 lock screen and home screen; touch, buttons and USB already work. The next work is ordered by what blocks actual use.

## P0 — make the device usable

1. **Novatek NT36672 touch**: done (patches `0193`–`0208`, `0230`, `0231`, `0234`).
   - Taps, icon taps and the swipe-up unlock work on the device; 0 I2C errors and ~14 ms per idle read.
   - Next: move from the bit-banged I2C to the QUP1 hardware block, and cut the per-frame `delay()` cost.
   - Check the panel variant: Synaptics uses `0x20`; this unit is Novatek.

2. **PM660/PM660L buttons**: done (patch `0201`, confirmed on the device).
   - Power (PM660 KPDPWR), Volume Down (RESIN) and Volume Up (PM660L GPIO7) are mapped to HID. Lavender has no camera key.
   - Check the `SWEET KEYS:` lines in `SANTONILOG1` and the `key=N down=…` events on a press.
   - Power long-press should also be handled cleanly; the PMIC hardware reset is the fallback.
   - This unblocks rebooting and testing the second boot.

3. **USB gadget**: done. CDC-NCM network and live log work from Windows (patches `0200`, `0210`–`0213`). Next: adb-style shell or file transfer over it.
   - DWC3 device mode with CDC-ECM and the UDP live log: `sweetusb=1` (read-only probe), `2` (gadget), `3` (also power up the GDSC/clocks).
   - On the first boot, read `SWEET USB: GDSC … (on|off)`. If ABL leaves the PHY regulators off, the next step is the PM660 LDOs plus the DTB `qcom,qusb-phy-init-seq`.
   - Windows has no CDC-ECM driver: add an NCM function, or use macOS/Linux as the host.
   - This gives `adb`/live logs and makes recovery much easier.

4. **Screen colours** (to verify)
   - The lock screen and the home screen are upright (an earlier "rotated 90°" note was wrong), but the date, clock and status bar render red/orange in both.
   - Check whether that is the iOS 16 wallpaper tint or a framebuffer pixel-format (channel order/alpha) issue before changing anything.
   - Touch coordinates map to the panel as it is: native portrait 1080×2340.

5. **PosterBoard / lock-screen poster**
   - SpringBoard's `prewarm` XPC reaches `PosterBoard`, which traps in
     `-[PBFPosterExtensionDataStore _stateLock_setupSwitcherConfigurationForFirstTime]`
     (assertion on `_stateLock_switcherConfiguration`) and is restarted by launchd.
   - Reproduce with `sweetnopb=1` (patch `0217`) and the swap telemetry (`0215`): if the UI
     lives without the poster, the gate is its data store, not the renderer.
   - Fix directions: reset the persisted data store on the HFSX root, or adopt bergamota's
     userland swizzles for the HFS/no-data-protection checks (`sweetfp.m`, `sweetpb26.m`).
   - Not caused by the empty `IOAcceleratorES`: the symbolicated stack never enters Metal.

6. **Second boot / boot time**
   - Done: the second boot skips the data migration (`DataMigration` plist).
   - Still ~18 min to the lock screen: use the USB live log (`sweetusb=2`) to find where the time goes.

## P1 — platform completeness

7. **Data volume / preboot**
   - `libignition: failed to lookup preboot: 25` and `mount: data volume missing` are still present.
   - Create a proper data volume or bind the preboot paths; reduce the first-boot migration.
   - whyred reached its home screen without a full preboot, so this may be a warning rather than a hard gate.

8. **GPU**
   - Adreno 512 has no iOS driver. The empty `IOAcceleratorES` is only a workaround.
   - Investigate a Mesa/freedreno path or a minimal `IOAccelerator` that serves `IOSurface`.
   - The topaz port has GPU work on a related Adreno generation; use it as a reference, not a transplant.

9. **RTC**
   - Set the PMIC RTC so the date is not 1969/December 31.

10. **Wi-Fi / Bluetooth**
   - WCN3990 firmware/driver is missing. The whyred/topaz trees have reference code.

11. **Audio**
    - The phone is **not** a WCD9335/SLIMbus board: the live DTBO disables `slim_aud`, and
      the enabled card is `qcom,sdm660-asoc-snd` with the PM660L analog codec (SPMI
      `0xf000`), the LPASS digital codec (`0x152c0000`)/SoundWire master (`0x152c1000`) and
      the ADSP (PAS 1). Speaker = AFE `INT4_MI2S_RX`; headphones = `INT0_MI2S_RX`;
      microphone = `INT3_MI2S_TX`.
    - `0500` (read-only platform probe) is written and its host tests pass; the ladder
      through the TrustZone/PAS load, GLINK/APR, codec, AFE stream and the iOS HAL plug-in
      is in [`docs/AUDIO.md`](AUDIO.md) and
      [`docs/AUDIO-BRIEF.md`](AUDIO-BRIEF.md).
    - Next: build/boot `0500` (`lavaud=1`), then `0501` power-up and `0502` firmware read.

12. **Battery gauge**
    - The current fallback is static (100 %, 4200 mV). Add the PM660 gauge.

## P2 — research

13. **4 KiB pages / 8 cores**
    - XNU supports 4 KiB on arm64, but the iOS 16 userland assumes 16 KiB.
    - The 16 KiB Gold-only path is the supported one; 4 KiB/8 cores is a separate experiment.

14. **Performance**
    - Software rendering, synchronous eMMC writes and a single boot CPU were the main costs.
    - With `cpus=4` and the eMMC write fix, the lock screen is reachable; the UI is still slow.

15. **Daily driver**
    - Not a goal. The port has no data protection, no SEP, permissive AMFI/sandbox and no usable cellular/Wi-Fi/audio.

## How to help

- Test on other lavender variants (panel/touch differences) and report the I2C addresses.
- Provide sanitized boot logs and `SANTONILOG1` dumps.
- Send patches for touch, buttons, USB or orientation with a clear test procedure.
- Do not include Apple binaries, personal data or Apple IDs in issues.
