# LIOS integration audit — reference-first migration

Date: 2026-11-23

## Integration direction

The base of this integration branch is the public `pcorezzola/lavender-ios16` repository at reference snapshot `41d4acf060fe6a3561b0b98ad8defbaea9aa04cf`. Its `main` branch remains the unmodified reference baseline. The source LIOS repository `NAANG360/hackios-lavender-xnu` and its `ci/lavender-machine-profile` branch are also left unchanged. This branch is `integration/lios-native-platform` in the fork `NAANG360/lavender-ios16-lios-integration`.

The recursive source-tree comparison at those pins found 439 reference files and 229 LIOS files, with 120 shared paths (91 byte-identical, 29 modified), 319 reference-only files, and 109 LIOS-only files. The reference tree supplies the completed Lavender kernel/storage/root-filesystem, touch/input, USB, display and Darwin userspace path. LIOS is a source of selective platform hardening and candidate native drivers, not a replacement for the working boot stack.

## Confirmed reference performance baseline

The reference documents SDCC v5 eMMC with PIO writes, CMD13/DAT0 card-ready completion fencing, and ADMA2 reads in 32 KiB chunks with cache maintenance, PIO fallback and a three-failure disable threshold. Its XNU README reports about 16 MB/s reads after ADMA2 versus about 465 KB/s in the previous PIO path, and approximately 2–3 minutes to lock screen after prior 23-minute boots. These are the reference project's reported device measurements, not a new LIOS benchmark; the integration does not claim a further measured speedup.

The reference also records on-device touch operation after its delay-to-timebase spin fix and reduced partial-frame polling. That path owns GPIO2/3 as bit-banged I²C. It must not be displaced by LIOS QUP's GPIO mux/clock/controller setup until an IOKit touch client and a physical transaction test are ready.

## LIOS contributions and disposition

| LIOS contribution | Decision | Reason / gate |
| --- | --- | --- |
| 16 KiB granule capability check | Ported in this commit | Fail-closed bootloader and MMU checks, with host regression tests. |
| Reserved-range-aware ADT generator and panic-log allocation | Retain as a separate candidate | Useful against the reference's simpler 1 GiB aperture, but must validate its FDT and must default to A73 MPIDR 0x100 / four Gold cores, not its source default 0x000. Do not replace the working generator until the actual target FDT and image fit are checked. |
| Native QUP-v2 IOKit provider | Defer activation | Its start path remuxes GPIO2/3, takes a voltage vote, changes GCC clocks and resets QUP. It conflicts with the reference bit-banged touchscreen owner unless the input path is explicitly migrated. No physical transaction has been accepted. |
| RPM/GLINK voltage-vote provider | Defer activation | It writes AP-owned shared-memory ring indices and sends a GLINK handshake; a physical ACK and shared-memory layout must be confirmed first. |
| Boot guardian / watchdog reset path | Do not port by default | It relies on direct PMIC/SPMI/watchdog MMIO and A53 guardian operation. It is not needed in the reference's Gold-only boot path and requires separate hardware review. |
| Extra XNU bring-up tracing / patch validators | Selectively reuse | Preserve the reference's more complete boot/IOKit path; add only non-invasive checks where they provide a measurable regression gate. |

## ROM-installer and boot-trust integration

The reference recovery-shim sequence is not the deployment target for this branch. The integration workflow now builds two Lavender Android boot-image artifacts from the successful XNU kernel: `boot.img` (preferred, visible verbose console over the Apple boot mark) and `boot-quiet.img` (reduced diagnostics). The `lavender-rom-installer-payload` artifact is intended to be consumed by the user's existing custom ROM installer, with `boot.img` written as the final image to the normal `boot` partition. No recovery-partition write or `fastboot boot` dependency is part of this path.

The package job validates the exact Lavender Mach-O kernel, the appended Lavender DTB, the embedded payload command line, the 7,340,032-byte ABL image limit, and SHA-256 manifests. It does not flash the phone, repartition storage, include the iOS root filesystem, rewrite `vbmeta`, or modify `dtbo`. The bootloader must be unlocked. The build checks do not prove that this handset's ABL/verified-boot policy accepts the image; actual boot trust remains a physical-device acceptance gate.

Both boot images use `rd=emmcroot0`. They therefore still require a compatible iOS HFSX root at that device, matching the reference project's current root layout. The root image is not bundled in this artifact. The custom installer must preserve unrelated partitions and retain a restorable copy of the Android ROM's original boot image. Its exact script is not part of this repository, so this branch delivers a deterministic boot-image payload rather than guessing at installer-specific commands.

## Legal provenance

Keep `LICENSE` and `NOTICE.md` intact. The reference NOTICE scopes MIT to specified original docs/scripts and patches; XNU modifications retain APSL requirements; imported `ios-lavender` and `bergamota` portions have separate provenance and no blanket MIT relicense is asserted. A repository's lack of a GitHub license badge or formal recognition does not waive copyright. Every copied or adapted file must retain its applicable notices and attribution.

## Acceptance gates

1. Host tests, patch-integrity checks and Lavender ADT checks pass.
2. XNU builds with the Lavender machine config and all required patches are applied (a compile is not proof of boot).
3. Packaged image passes manifest/hash and payload/ADT bounds checks.
4. Non-destructive physical boot on the user's actual 3 GB / 32 GB handset.
5. Separate physical acceptance for each driver: storage reads/writes, QUP/Novatek I²C, RPM voltage ACK, touch HID, USB and display.
6. Only after verified backups, capacity and recovery does a persistent userdata-backed root volume become eligible for consideration.

## References pinned for this audit

- Working base: `pcorezzola/lavender-ios16@41d4acf060fe6a3561b0b98ad8defbaea9aa04cf`
- LIOS comparison: `NAANG360/hackios-lavender-xnu@2c7c89bff7c05875bfac8b53d7f5ceb1a1867d35`
- Reference source: `https://github.com/pcorezzola/lavender-ios16`
- LIOS source: `https://github.com/NAANG360/hackios-lavender-xnu`
