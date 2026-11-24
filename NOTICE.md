# Notice and third-party credits

This repository contains original documentation, scripts and lavender patches, plus the XNU patch series, shim and tooling imported from `ios-lavender` (below). It does not contain Apple binaries or Apple's XNU source, which is fetched from Apple's open-source release at build time.

## Base projects

- **ios-sweet** — [`dipa-studio/ios-sweet`](https://github.com/dipa-studio/ios-sweet), MIT License, Copyright (c) 2026 dipa-studio. The upstream tooling and methodology.
- **ios-lavender** — [`aLpHa-Git-69/ios-lavender`](https://github.com/aLpHa-Git-69/ios-lavender), branch `lavender`. Imported from commit `bc606b7` (the pinned base `83356567…` plus `0192`): `xnu/patches/0001`–`0192` and the stage scripts, `shim/`, most of `tools/`, `tests/` (except `tests/host/`), the generated `drivers/` mirrors and `resources/lavender_dtb.bin`. Not imported: the HakiOS third-party drivers, the whyred DTB, `qemu-sweet.py`, `sweet-touch-answer.sh`, agent configuration files and `NOTES.md`. Lavender work continues in this repository; `ios-lavender` is not modified. Upstream ships no LICENSE file; this code remains with its authors and is used here under the upstream project's terms (it derives from MIT-licensed ios-sweet). Contact the authors for relicensing. The workflow default and triggers were changed to `LAVENDER`/`main`, and the HakiOS checksum check in `tests/run_tests.sh` became optional.
- **bergamota** — [`ry-diffusion/bergamota`](https://github.com/ry-diffusion/bergamota), `mkiosroot-macos.sh`, `img2simg.py` and shim ideas. The checked-out branch does not ship a LICENSE file; its code remains with its authors.
- **ios-topaz** — [`MakrSas/ios-topaz`](https://github.com/MakrSas/ios-topaz), reference for the A73/A53 topology, Wi-Fi and GPU work.
- **HakiOS** — [`oosexgang/hakios-xnu-kernel`](https://github.com/oosexgang/hakios-xnu-kernel), methodology reference. No `sdm660` support.

## Apple

- **XNU** — Apple open-source `xnu-8796`, Apple Public Source License (APSL).
- The iOS userland, IPSW, device trees, dyld shared cache and other Apple binaries are **not** included and are not licensed by this repository.

## Tools

- **TWRP** — the lavender recovery image used for backups/logs; see [twrp.me](https://twrp.me/xiaomi/xiaomiredminote7.html).
- **LLVM/clang/LLD** — kernel and shim toolchain.
- **Docker** — reproducible shim build.

## Screenshots and photos

`docs/images/lock-screen.jpg`, `settings.jpg`, `siri-setup.jpg` and `boot-self-tests.jpg` are photos of the project author's Redmi Note 7 running the iOS 16.7.16 userland, and `docs/images/home-screen.png` is a screenshot captured from that device's display. They are included as evidence of the milestones.

## Original work

The original documentation, the `0192` write-ready patch, the `0200`/`0201` USB and key patches, `tests/host/`, `tools/fdtdump.py`, `tools/build-shim.sh`, `tools/pull-sweetlog.sh`, `fix-datamigration.sh` and the repository organization are MIT (see `LICENSE`).
