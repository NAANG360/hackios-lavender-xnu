# Legal and safety

## Research project

This is a non-commercial research project. It is not affiliated with Apple, Xiaomi or Qualcomm.
It is not a product, not a daily driver and not a supported way to run iOS on non-Apple hardware.

## Apple software

- **XNU** is open source under the Apple Public Source License (APSL). The XNU source used here is
  `xnu-8796` from Apple's open-source distributions.
- The **iOS userland**, the **IPSW**, the **device trees**, the **dyld shared cache** and the other
  Apple binaries are **not** distributed in this repository. You must obtain them yourself and
  comply with Apple's license terms.
- Downloading and using Apple firmware may be subject to Apple's EULA and to local law. In some
  jurisdictions, circumventing technical protection measures is restricted. You are responsible for
  how you use this information.
- Do not redistribute Apple binaries, extracted userland, the IPSW or the root image from this
  project.

## Device safety

- Flashing `userdata` erases Android and any data on it. Back up `persist`, `modemst1/2`, `fsg`,
  `fsc`, `boot`, `dtbo`, `vbmeta`, `recovery` and the GPT first.
- A buggy storage driver can corrupt the GPT. Boot read-only before enabling writes.
- The port has no working buttons or USB gadget while iOS runs; a stuck boot may require a PMIC
  long-press or battery drain. Do not use EDL/test points unless you know the hardware.
- The port has permissive AMFI/sandbox, an AppleKeyStore stub and no data protection. Do not use
  personal data, an Apple ID, iCloud, or any account on this device.

## Privacy

This repository does not include device serial numbers, IMEI, Apple IDs, personal paths, network
addresses or user data. Boot logs are sanitized before publication. If you contribute a log,
redact the eMMC CID/serial, the preboot UUID, the device serial and any personal information.

## License

The original documentation and scripts in this repository are MIT (see `LICENSE`). The base trees
and patches retain their own licenses and attribution; see `NOTICE.md`. Apple's XNU remains under
the APSL. Nothing in this repository grants a license to Apple's proprietary userland or firmware.
