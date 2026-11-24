# LIOS Lavender — ROM installer handoff

## Output contract

The `lavender-rom-installer-payload` workflow artifact contains two Android boot-image-v0 files for the Redmi Note 7 / 7S (codename `lavender`):

| File | Role |
| --- | --- |
| `boot.img` | Preferred verbose boot: console text stays visible over the Apple boot mark; persistent log ring and detailed code-signing diagnostics enabled. |
| `boot-quiet.img` | Reduced-diagnostics fallback using the same XNU/ADT/boot path. |

It also contains `SHA256SUMS`, `manifest.json` and `INSTALLER-INTEGRATION.md`.

## Installer integration

Use `boot.img` as the final boot image written by the existing custom ROM installer to the normal Android `boot` partition. Do not send it to `recovery`, and do not depend on `fastboot boot`. Preserve the previous ROM's original `boot.img` so the device can be restored.

The artifact does not rewrite `vbmeta`, `dtbo`, modem, persist or other partitions. It does not flash or repartition `userdata`.

## Boot trust

The build checks:
- the XNU kernel is a Lavender arm64 Mach-O;
- the appended DTB matches Lavender's Qualcomm MSM/board IDs;
- the embedded XNU payload carries the required root and bring-up arguments;
- the boot image is within the ABL size limit;
- the output files match the SHA-256 manifest.

These are build-time checks, not a claim that the physical ABL accepts the image. The handset must have an unlocked bootloader, and actual acceptance by that unit's boot chain remains to be tested. No vbmeta/AVB modification is performed automatically.

## Root filesystem dependency

The current reference boot configuration uses `rd=emmcroot0`. A compatible iOS HFSX root must already exist at that device. The root image is not included in this artifact, and this workflow does not decide or change the user's storage layout.

## Verified package (2026-11-23)

The following artifact was built and uploaded by workflow run [#45192899569](https://github.com/NAANG360/lavender-ios16-lios-integration/actions/runs/45192899569) on branch `integration/lios-native-platform`, source commit `3afbbe90031c30f97b0d467bd376ff03ed261f4a`.

- Artifact: `lavender-rom-installer-payload` (artifact ID `14825703756`)
- Archive size: 12,371,892 bytes
- Kernel source artifact: `xnu-kernel` from successful run `45181405193` at `a4e9aed12478877b510cbc588323384b432431af`; XNU/driver sources were unchanged between that kernel build and this packaging run. The shim/loader source was compiled from the current integration branch.
- Host USB/key tests: passed.
- Kernel and Lavender DTB validation: passed.
- Embedded payload checks: passed.
- Both Android boot images: passed the 7,340,032-byte ABL size ceiling.

| Image | Size | SHA-256 |
| --- | ---: | --- |
| `boot.img` (preferred verbose console) | 6,782,976 bytes | `032d14ba0b1485a0ee63575c6071658128b657513f452136442ec738e2db6229` |
| `boot-quiet.img` | 6,782,976 bytes | `297ef10e55d654094bafc16966e242943104651792a4cf5eacb4f51581218752` |

The ZIP contains `boot.img`, `boot-quiet.img`, `manifest.json`, `SHA256SUMS` and `INSTALLER-INTEGRATION.md`. The checksums above were recomputed from the downloaded artifact archive and match the included checksum file.

**This verifies a buildable boot payload, not a successful physical boot.** ABL trust and root-device availability remain on-device validation gates.
