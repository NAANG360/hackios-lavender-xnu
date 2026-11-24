# References

## Base trees

| Repository | Branch / commit | Role |
| --- | --- | --- |
| [`aLpHa-Git-69/ios-lavender`](https://github.com/aLpHa-Git-69/ios-lavender) | `lavender`, `83356567124670f50a320b8ecc7c04f7ccf8f8e0` | **Main base**: XNU tree, lavender/whyred shim, eMMC SDCC v5 driver, DTB, notes |
| [`aLpHa-Git-69/UniXNU-lavender`](https://github.com/aLpHa-Git-69/UniXNU-lavender) | `main`, `a4828bbfd683be2b96c4d0ceae4f11a436ad3d7d` | UEFI/DarwinBoot bring-up, Gold/RAM probes, iOS checkpoint |
| [`MakrSas/ios-topaz`](https://github.com/MakrSas/ios-topaz) | `topaz`, `e4772911e116ab4ab6c4ac1f38e6106fefe8b483` | Advanced reference: Wi-Fi, GPU, home screen; same A73+A53 topology |
| [`ry-diffusion/bergamota`](https://github.com/ry-diffusion/bergamota) | `develop`, `37a7a7a917fc09945ed2f7d1ae68b84d4525b4cb` | Poco F1 (SDM845) port; `mkiosroot-macos.sh`; shim ideas |
| [`dipa-studio/ios-sweet`](https://github.com/dipa-studio/ios-sweet) | `main`, `45263bee51df4a0f55cdf9c649af886399488ddb` | Upstream tooling (sweet, SM7150); MIT |

## Methodology

- [`oosexgang/hakios-xnu-kernel`](https://github.com/oosexgang/hakios-xnu-kernel) and the
  [HakiOS guide](https://hackios-web.vercel.app/en/guide/your-device): one repository per device,
  incremental driver enablement, UEFI loader. No `sdm660` layer; methodology only.

## Apple open source

- [`apple-oss-distributions/xnu`](https://github.com/apple-oss-distributions/xnu) — XNU source,
  APSL. The port uses `xnu-8796` (Darwin 22 / iOS 16 era).
- The iOS 16.7.16 (20H392) iPhone X IPSW is used only to build the local userland/root image.
  It is **not** distributed here.

## Vendor sources for the SDM660 audio path

- [`MiCode/Xiaomi_Kernel_OpenSource`](https://github.com/MiCode/Xiaomi_Kernel_OpenSource),
  branch `lavender-q-oss` (the phone's own Android 10 kernel): `sound/soc/codecs/`,
  `sound/soc/msm/`, `arch/arm64/boot/dts/qcom/` and the `tfa98xx` tree are the
  register/protocol reference for the PM660L analog codec, the LPASS digital codec,
  SoundWire and the AFE ports. Read as behaviour, not as portable code.
- Linux `drivers/soc/qcom/subsys-pil-tz.c`, `pil-q6v5.c`, `smp2p.c`,
  `glink_smem_native_xprt.c` and `drivers/clk/qcom/gcc-sdm660.c`: TrustZone PAS
  command numbers, Q6SS offsets, SMP2P item layout and the LPASS GDSC/vote registers.
  The SDM660 vendor kernel is `msm-4.4`; mainline carries the same PAS/Q6SS names.

## Tools

- [TWRP for lavender](https://twrp.me/xiaomi/xiaomiredminote7.html) — recovery used for backups and
  log pulls.
- [Android sparse image tools](https://android.googlesource.com/platform/system/core/) — `img2simg`,
  `simg2img`.
- [LLVM](https://llvm.org/) — kernel/shim toolchain; `llvm-objdump` for the LSE check.
- [Docker](https://www.docker.com/) — reproducible shim build (clang/LLD).

## Whyred notes

The whyred branch of `ios-lavender` has a long `NOTES.md` that documents the same bring-up on a
sibling SoC. The DataMigration plist, the `sweetfb_hz` work and the AKS/SpringBoard behavior are
directly relevant. Read it before changing the boot UI or the data migration.
