# HackIOS Lavender

**An experimental XNU bring-up for the Xiaomi Redmi Note 7 / Redmi Note 7S (`lavender`, Qualcomm SDM660).**

This repository is working toward running Apple's open-source **XNU 8796** kernel (the iOS 16 generation) on physical Lavender hardware. The longer-term goal is a real Darwin userspace based on the iPhone X iOS 16.7.16 (20H392) release—not an Android theme, launcher, or emulator-only demonstration.

> **Status: early kernel bring-up.** A verified RAM-only diagnostic boot image exists, and physical footage has reached the IOKit root-nub allocation checkpoint on an older, not-yet-identified image. There is no verified BSD init, root mount, launchd, SpringBoard, or complete iOS userspace boot. CI success is not physical boot success.

## Project status — 18 November 2026

| Item | Current state |
| --- | --- |
| Target | Redmi Note 7 / Note 7S, `lavender`, Snapdragon 660 / SDM660, ARM64 |
| Kernel | XNU 8796, iOS 16 generation |
| Active branch | `ci/lavender-machine-profile` |
| Latest accepted CI image | [Run #358](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44339488789) |
| Accepted image artifact | `lavender-fastboot-boot-test`, artifact ID `14445642030` |
| Image | `iosboot.img`, 16,617,472 bytes |
| SHA-256 | `b6361d9d03147570fb0a5407413d1b25c623d77c20649bcba63c35a3426f508f` |
| Run #358 source | `f1f633f9ac1887736f17937c38eb6c32de489160` |
| Root-nub trace patch | `0127-lavender-root-nub-init-trace.patch`; full build, verifier, linked-kernel and package gates passed |

Run #358 passed the complete XNU 8796 build, Lavender source/IOKit trace verification (including patch 0127 events 126–132), linked ARM64 Mach-O verification, and downstream RAM-only fastboot-image packaging. The downloaded artifact was independently extracted; `SHA256SUMS` passed and the image hash matches `BUILD-MANIFEST.txt`. Manifest bootargs are `debug=0xc serial=3 cpus=1 keepsyms=1 lavender_no_fbcon=1`. This is a verified diagnostic boot package, **not** a completed iOS installation and not proof that it has booted successfully on the handset.

The earlier 18 November clips ending at `IOKIT ROOT NUB ALLOCATED` remain unbound to an image hash. A further recording (`1000318056.mp4`) shows a physical shim-to-XNU transition and early/platform milestones, with generic post-platform trace rows clearly readable through event 106; the later lower rows are not legible enough to establish events 126–132. The video does not show the host-side image SHA-256 or the associated `fastboot boot` invocation, so it is **not yet verified as a Run #358 physical test**. Do not infer root-nub success or failure, or attribute it to Run #358, until the host-side hash matches the accepted artifact.

## Current engineering focus

The immediate diagnostic boundary is the root platform expert's initialization:

1. `InitIOKit()` allocates `IOPlatformExpertDevice` (trace event 120).
2. It calls `gRootNub->init(dtTop)`.
3. `IOPlatformExpertDevice::init(dtRoot)` attempts `IODeviceTreeAlloc(dtRoot)` and, if that returns a registry entry, `super::init(dt, gIODTPlane)`. If the device-tree path is unavailable, it takes the fallback `super::init()` path.
4. A false return causes the caller to panic; event 121 is emitted only after successful initialization.

Patch **0127** adds trace events 126–132 around those operations, including pointer and success values. The splitter, source verifier, and XNU workflow were updated to preserve and check those markers, including when generated early-console source is restored from cache.

| Trace | Meaning |
| ---: | --- |
| 120 | Root platform expert object allocated |
| 121 | Root-nub `init()` returned successfully |
| 122 | Root-nub attached |
| 123 | `InitIOKit()` returned |
| 124–125 | `ConfigureIOKit()` entered and returned |
| 126–128 | Root-nub init entered; device-tree allocation began and returned |
| 129–130 | Device-tree registry superclass init began and returned |
| 131–132 | Fallback superclass init began and returned |
| 133–146 | DeviceTree allocator setup, root/child conversion, registry attachment, interrupt-map pass and return |

Patch 0127 has now passed the complete Run #358 CI gates: patch/source guards, XNU 8796 compilation, IOKit trace verifier, linked ARM64 Mach-O verification, and RAM-only image construction plus identity/checksum verification. The image is ready for a controlled physical RAM-boot test; no physical result is yet established.

The clearest later physical frame reaches event 127, immediately before the DeviceTree allocator call, then remains on that screen before the A53 Guardian reports its last marker. A Run #358 ADT audit found one overlong touchscreen-node property key in the fixed 32-byte ADT name field; the generator now shortens it and rejects invalid property names. This is a confirmed malformed field, not a proven sole cause of the stall.

Patch 0128 adds allocator-internal events 133–146, including sampled child-conversion indices, root attachment, interrupt-map traversal and allocator return. The Guardian now tracks post-stage-20 XNU events in its shared progress cell, and generic console events are throttled so late IOKit diagnostics retain screen space. See Chapter 03 for the event map and current validation state.

## Device and hardware notes

- Qualcomm SDM660 / Snapdragon 660, ARM64, Kryo 260 CPU topology.
- Lavender is ARM64, not arm64e. Do not assume Apple pointer authentication, PPL, or Apple-SoC-specific services.
- Use the bootloader-provided FDT/DTB as the authority for memory and device-tree reservations; Linux-visible memory ranges are not automatically an approved XNU map.
- The display is a 1080×2340 Tianma FHD+ DSI panel; the GPU is Adreno 512.
- Hardware-specific MMIO, interrupt, timer, regulator, storage, and display assumptions must come from Lavender evidence. Do not copy IOS-Sweet's SM7150/Kryo 470 hardware constants.

## Build and package

The build is automated with GitHub Actions; no local macOS XNU toolchain is required.

1. Open the repository's **Actions** tab and select the **xnu** workflow.
2. Run the workflow on `ci/lavender-machine-profile`, selecting machine profile **LAVENDER**. The default XNU configuration is DEVELOPMENT.
3. Wait for the complete build job, including patch application, Lavender source guards, XNU compilation, linked Mach-O verification, and kernel artifact upload.
4. The XNU workflow's downstream package job runs automatically after the build succeeds. Wait for both jobs to complete.
5. Accept the image only if the package job verifies the matching source revision, kernel/payload/DTB identity, Android boot-image structure, manifest and SHA-256, and uploads `iosboot.img`.

The packaging workflow is intentionally separate and manual. It refuses unsuccessful XNU runs and unexpected source branches.

Useful workflow files:

- [XNU build workflow](.github/workflows/xnu.yml)
- [RAM-only package workflow](.github/workflows/package-lavender.yml)
- [Lavender IOKit source verifier](tools/verify-lavender-iokit.py)
- [Lavender early-source splitter](tools/split-lavender-early.py)
- [XNU patches](xnu/patches/)

## Physical testing and safety

**Only use RAM-only boot testing.** After downloading and verifying an accepted `iosboot.img`, the intended test command is:

```sh
fastboot boot iosboot.img
```

This loads the image temporarily; it is not a flash operation. Preserve `cpus=1` in the boot arguments and keep the confirmed volume-key recovery route available.

- Never run `fastboot flash`, `fastboot erase`, `fastboot format`, or repartition commands for this project.
- Never write Android eMMC, `userdata`, boot, vendor_boot, recovery, modem, persist, or any other device partition.
- Do not use an emulator or QEMU result as evidence of physical Lavender compatibility.
- Keep image identity explicit: record the downloaded artifact ID, extracted image SHA-256, and boot arguments for every physical test.
- A visible Apple logo or static diagnostic overlay is not proof of BSD startup, root mount, launchd, SpringBoard, or a usable iOS system.

## Repository map

| Path | Purpose |
| --- | --- |
| `xnu/patches/` | Ordered XNU 8796 patch series for Lavender |
| `shim/` | ARM64 boot shim and RAM-only boot-image support |
| `.github/workflows/xnu.yml` | XNU build, source guards, and linked-kernel verification |
| `.github/workflows/package-lavender.yml` | Separate RAM-only fastboot image packaging |
| `tools/split-lavender-early.py` | Generates the dedicated Lavender early-console source |
| `tools/verify-lavender-iokit.py` | Checks IOKit platform matching and trace ordering/labels |
| `tools/mkadt-lavender.py` | Builds the Lavender Apple Device Tree |
| `tools/mkpayload.py` | Packages kernel, ADT, and boot arguments |
| `docs/handoff/01-device-and-safety.md` | Device facts and non-destructive testing rules |
| `docs/handoff/03-current-debugging.md` | Current diagnostic record and next engineering steps |
| `PROJECT_HANDOFF.md` | Project history and cross-session handoff |
| `AGENTS.md` | Engineering workflow and repository-specific agent instructions |

## What is not implemented or proven

- No verified BSD init, VFS/root mount, launchd, SpringBoard, or complete Darwin userspace boot.
- No accepted physical boot result for the Run #358 image.
- Patch 0127 (root-nub trace events 126–132) passed the complete Run #358 source, build, linked-kernel verification and RAM-only package gates.
- No Lavender eMMC root driver or safe root-mount path is accepted as working.
- Wi-Fi, Bluetooth, cellular, audio, camera, sensors, GPU acceleration, and other iPhone services are not established.

## Credits and source boundaries

This work builds on Apple's open-source XNU and public Qualcomm/Xiaomi kernel and device-tree sources. IOS-Sweet and other projects may provide useful XNU build and porting methodology, but their device-specific hardware configuration is not evidence for Lavender. iOS firmware and userspace are Apple's; this repository does not distribute Apple's proprietary OS image.

---

For the authoritative current diagnostic state, read [Chapter 03 — Current Debugging](docs/handoff/03-current-debugging.md).