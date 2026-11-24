# Achievements and timeline

All dates are 2026-11-19 unless noted. This is the lavender-specific delta on top of `aLpHa-Git-69/ios-lavender`.

## Phase 0 — safety net

- Stock fastboot ROM for `lavender_eea` downloaded and verified (MD5/SHA-256).
- TWRP 3.7.0_9-0-lavender verified and flashed to `recovery`.
- 15 partition backups + GPT head/tail, `SHA256SUMS` verified, compressed copy kept offline.
- Restore path documented: stock ROM + TWRP + backups. No personal data on the iOS port.

## First XNU boot

- Kernel built for `LAVENDER` in GitHub Actions (run `44614675208`, 9m25s).
- `kernel.development.lavender`: 17,819,720 bytes, SHA-256 `76c2e0fddfa8d418ed051379764c2bd333c61cbea6e129fe80332d246e99acb6`.
- `llvm-objdump -d --mattr=+v8.5a` found no LSE instructions.
- First shim (`shim-boot-lavender-2026-11-19.img`, SHA-256 `d303ce96…`) booted XNU to `launchd`, then panicked:
  `SWEET EARLY PANIC: unexpected SIGKILL of launchd (CS_KILLED)`.

## Code-signing bring-up

- Rebuilt the shim with:
  `emmcrw=1 rootrw=1 cs_enforcement_disable=1 unentitled_ios_sim_launch=1 launchd_unsecure_cache=1 cs_debug=2 wdt=-1 wdt_test=no_user_checkin`.
- `shim-boot-lavender-csrw-2026-11-19.img`, SHA-256 `6a402bab4a7aac8d8d48179e0ed64f6ed5aacf34b6330f8e3006fb2aee923487`.
- The boot reached `launchd`, `xpcproxy`, `cfprefsd`, `powerd`, `distnoted`, `watchdogd` and `SpringBoard` (pid 26). `SWEET AKS` calls returned `0x0`; platform binaries loaded.

## GUI and the eMMC wall

- `shim-boot-lavender-gui-2026-11-19.img` (`sweetfb_noconsole=1 sweetfb_stayon=1 sweetfb_hz=30`) showed the **Apple logo + progress bar (~25 %)**.
- The SYSTEM log ring exposed the blocker: repeated `CMD24`/`CMD25` `cmd-timeout`, `recover: stopping an open-ended transfer`, `recovery incomplete (5)`.
- `sdhci_write_pio()` waited for the controller's transfer-complete but not for the card's `READY_FOR_DATA`/DAT0. The next write arrived while the card was still programming.
- A read-only shim (`0a8b8f4e…`) also stalled; the write path, not the userland, was the gate.

## Patch 0192 — reliable eMMC writes

- Added `sdhci_wait_write_ready()` after `sdhci_wait_transfer_complete()`:
  poll `PRESENT_STATE`, require `!CMD_INHIBIT && !DATA_INHIBIT && DAT0`, then send CMD13 and require `TRAN` + `READY_FOR_DATA`.
- Commit `bc606b7` in the fork; CI run `44633431380` (7m20s) produced:
  `kernel.development.lavender`, 17,836,144 bytes, SHA-256 `793d232f7c56be8e351e2fe6246b21515f20835c508e2cdbb72016d3830a87be`.
- Result: the SYSTEM ring flushed **38,595 bytes / 24 flushes** and then **110,468 bytes / 604 flushes** with **zero** `CMD24`/`CMD25`/`cmd-timeout`. HFS journal replay, `keybagd`, `usermanagerd`, `SpringBoard` and the GUI all worked.
- The mock caught a real bug before the kernel build: the new helper could not be named `sdhci_wait_card_ready` because the CMD6 helper already has that name. It was renamed `sdhci_wait_write_ready`.

## Four Gold cores

- The log showed `SWEET CPU: requested=1 selected=1 (limit=8)`.
- Adding `cpus=4` to the shim produced `SWEET CPU: requested=4 selected=4 (limit=8)` and `4/4 CPUs initialized, SGI1 verified`.
- The Silver A53 cores stay parked; they do not support 16 KiB pages.

## DataMigration and the first boot

- The first boot on a fresh root image ran `com.apple.datamigrator` and sat on the progress bar. The log reached `Early boot complete`, `auearlyboot`, `tzinit`, `sysstatuscheck`, `prng_seedctl` and `launchd_cache_loader`, but the bar did not advance.
- The whyred notes document the same gate: `DataMigration` repeats every boot unless the root image carries `com.apple.DataMigration.plist` with `LastSystemVersion = 20H392` (501:501, 0600).
- The lavender image now has that plist. The first boot still migrates because the root is fresh; with `cpus=4` the migration advanced and completed.

## Lock screen

- The first boot completed the migration and reached the **iOS 16.7.16 lock screen**:
  `Wednesday, December 31`, `4:37`, the camera shortcut and the home indicator.
- Evidence:
  - `SWEET CPU: requested=4 selected=4`, `4/4 CPUs initialized`.
  - `Early boot complete. Continuing system boot.`
  - `SWEET FB: display on`, `SWEET FB: swap 1..6`.
  - `SpringBoard` and `backboardd` alive; no panic and no eMMC timeout.
- Known-good shim: `shim-boot-lavender-0192-lite-cpus4-2026-11-19.img`, 6,709,248 bytes, SHA-256 `ba282d7977a23189e084705606f71dbf2c226fbbc02475bc7891a6c4c44e9425`.
- The lock screen is upright; the clock shows 1969/December 31 (RTC unset), and touch/buttons/USB/GPU were still missing at this milestone. (An earlier note called the lock screen "rotated 90°"; the photo shows it is not.)
- The second boot/persistence check is pending because the physical buttons do not work.

## Home screen, Settings and Siri setup (2026-11-21)

- **Home screen on the panel.** A device screenshot (`docs/images/home-screen.png`) shows SpringBoard's home screen: the app grid with Fotos, App Store and Configuración, the dock with Teléfono, Safari and Mensajes (with an unread badge) and the App Library pill (`Buscar`). The two widget slots stay on `Esperando…` because the WidgetKit extension never delivers a first timeline.
- **Settings opens.** The whole Settings list renders and scrolls on the device: Sonidos, Concentración, Tiempo de uso, General, Centro de control, Pantalla y brillo, Pantalla de inicio, Accesibilidad, Fondo de pantalla, Siri y búsqueda, Código, SOS de emergencia, Notificaciones de exposición, Batería and Privacidad y seguridad (`docs/images/settings.jpg`).
- **First-run Siri screen.** The setup flow reaches the Siri page with `Activar Siri` / `Ahora no` (`docs/images/siri-setup.jpg`). There is no microphone or speaker behind it, so this is UI only.
- **Skylight self-tests pass at boot.** While the Apple logo and progress bar are on screen, the userland Skylight window server runs its software blit self-tests (`sk_copy64_8` … `sk_copy64_64x`, `sk_copy32_*`, `sk_zero_16` … `sk_zero_128`, `memcmp`) and every one prints `PASSED`, ending at `sys_memregion: calling kmInit` (`docs/images/boot-self-tests.jpg`). This is the CPU path that draws the UI.
- **Colour and clock caveats.** The status bar still renders as a red/orange band and the home-screen clock reads `mié 4 feb 05:28`, not the real date: the screen-colour and RTC items remain open.

## Rejected variants

- `0192b` with a 1 s `CMD6_BUSY_TIMEOUT_NS` wait: CI run `44636855966`, kernel `2a6b542159cf5f314d907edddc7f7af96311e89c6cfcd11a24af7dcb56130d8b`. It stopped at `cc_fips_test`, earlier than the 30 s version. The 30 s timeout is the working one.
- `cs_debug=2` + `-enable_kprintf_spam`: too much log volume; the lite shim without them reached the lock screen.
