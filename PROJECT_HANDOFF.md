# LAVENDER XNU PORT — MASTER PROJECT HANDOFF

> **Read this file first in every new agent/chat session.** This is the authoritative cross-session handoff for the Redmi Note 7 / Lavender XNU port. Update it after every meaningful inspection, patch, CI run, artifact, or device test. Keep observations, hypotheses, and completed verification clearly separate.
>
> Last consolidated: **2026-11-12 22:18 IST (Asia/Kolkata)**  
> Project repository: https://github.com/NAANG360/hackios-lavender-xnu  
> Active engineering branch: `ci/lavender-machine-profile` (always re-resolve live HEAD before source writes; latest CI source commit under investigation is `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`)  
> Latest verified failed CI head: `1842f223f3e3926710b9f53c86b586dfd9b62a99`  
> Newest CI attempt observed: run `43259106099` (#84), queued at head `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`

---

## 1. Mission and definition of done

Port the open-source Apple XNU kernel (the iOS 16 generation, XNU 8796) to the **Xiaomi Redmi Note 7 / Redmi Note 7S, codename `lavender`, Qualcomm Snapdragon 660 / SDM660**. The immediate deliverable is a real, correctly built ARM64 XNU kernel for this hardware target. The larger goal is to boot a Darwin/iOS 16-era userspace on the actual phone.

The project is **not** complete when:
- the board profile passes grep-based validation;
- a build tree or object files are uploaded;
- a generic ARM64 XNU kernel compiles without the Lavender target config;
- a QEMU/emulator test works;
- a build workflow exits successfully but does not contain a kernel.

### Immediate milestone: working Lavender XNU kernel

A kernel artifact is accepted only after all of these are true:

1. XNU 8796 compiles with `CURRENT_ARCH_CONFIG=ARM64` and `CURRENT_MACHINE_CONFIG=LAVENDER`.
2. The output is a real ARM64 Mach-O kernel (not an empty artifact, object bundle, KDK library dump, or build tree).
3. Architecture, Mach-O file type, load/entry information, symbols/sections, and relocations are inspected with appropriate Apple binary tools.
4. No unresolved symbols or missing platform-specific closed-library dependencies remain unexplained.
5. The build log, exact source commit, build configuration, artifact SHA-256, and verification results are recorded here.
6. Only after static verification should a RAM-only boot payload/shim be prepared for the physical phone.

**A successful compile is not the same as a successful device boot.** Keep these milestones separate.

### Overall later milestones

- [ ] Reliable boot chain / shim / loader on actual Lavender.
- [ ] XNU reaches early kernel initialization and produces persistent diagnostics.
- [ ] CPU topology and secondary-core startup are correct.
- [ ] GICv3, architectural timers, MMU and memory map are stable.
- [ ] Platform expert and device tree are accepted by XNU/IOKit.
- [ ] Basic display output on the actual panel.
- [ ] Touch input.
- [ ] Storage and a Darwin root filesystem.
- [ ] `launchd`, core daemons, `backboardd`, SpringBoard.
- [ ] Software rendering first; GPU/Adreno 512 later.
- [ ] Other hardware drivers only after the core boot path is reliable.

Do not represent any unchecked item as working.

---

## 2. User requirements and operating constraints

- Target the **real Redmi Note 7 (lavender), SDM660**, not an emulator.
- QEMU/emulation may be used for isolated development experiments only when useful; it must never be presented as proof of Lavender hardware support or used as a substitute for the actual OS/device.
- Use **hackIOS/HakiOS**, **IOS-Sweet**, and public Lavender / SDM660 sources as engineering references. Compare code and architecture; do not blindly copy device-specific drivers or hardware constants from a different SoC.
- User wants practical progress, actual source changes, CI builds, artifacts, and precise reports—not repeated plans or claims that work will be done later.
- Aim for a working kernel first; do not get distracted by SpringBoard, cosmetics, or GPU before the kernel bring-up foundation works.
- Initial verification should be **RAM-only / temporary** where possible. Prefer a temporary boot path (for example, a compatible `fastboot boot` chain) so Android remains recoverable.
- Do not flash, erase, repartition, or overwrite boot-critical partitions as part of kernel experimentation. Never risk the user's only working recovery path. Get explicit confirmation before any destructive operation.
- User's power button is broken. Do not make workflows depend on pressing it. The user previously requested volume-key-based recovery/reboot workarounds; verify exact supported key behavior for the current recovery before relying on it.
- The user has repeatedly asked not to be told to run commands or gather more phone data when the information is already in uploaded audits or repository artifacts. Use those sources first.
- Do not promise an Actions job can run indefinitely in the background. CI jobs have platform timeouts and this assistant cannot guarantee unattended work after a chat/session ends. Each iteration must leave durable commits, logs, artifacts, and an updated handoff.

---

## 3. Target hardware — verified device audit

Two uploaded audits were supplied in this conversation:
- `lavender-final-audit.txt` — final audit, captured 2026-11-12 11:23 IST.
- `lavender-hardware-report.txt` — reconnaissance report, captured 2026-11-12 10:29 IST.

These are conversation uploads, not currently committed repository files. **Do not commit the raw dumps**: they contain device-specific identifiers and potentially sensitive runtime details. This handoff records only the relevant sanitized facts.

### Identity and software

| Property | Observed value |
|---|---|
| Device | Xiaomi Redmi Note 7 |
| Device codename | `lavender` |
| SoC / board platform | Qualcomm SDM660 / `sdm660` |
| Hardware vendor | Qualcomm (`qcom`) |
| Boot storage controller | `c0c4000.sdhci` |
| Android userspace | Android 15, SDK 35 |
| Running Linux kernel | `4.19.325-NYXION-SUSFS` |
| Kernel architecture | AArch64 |
| Running kernel toolchain | Android clang 19.0.0 / LLD 19.0.0 |
| RAM visible to Linux | `MemTotal: 2,803,968 kB` (roughly 2.8 GiB usable; device is the 3 GB configuration) |
| CPU topology | 8 logical CPUs, two clusters |
| Device-tree compatible strings | `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp` |
| Device-tree model string | Qualcomm SDM660 PM660 + PM660L MTP F7A overlay |

### CPU and SoC facts

Qualcomm's public SDM660 specification describes:
- 64-bit Armv8 Kryo 260, octa-core, dual cluster;
- four performance cores up to 2.2 GHz and four efficiency cores up to 1.8 GHz;
- Adreno 512 GPU;
- Hexagon DSP;
- LPDDR4/LPDDR4x memory support;
- display/DSI, camera/CSI, and eMMC/UFS-class platform interfaces.

Do not assume Apple-specific CPU features or Apple SoC services exist. In particular:
- Lavender is plain ARM64, not arm64e.
- Do not enable Apple pointer-authentication assumptions.
- Do not inherit SWEET's Kryo 470 / SM7150 assumptions.
- Do not assume Apple's PPL/monitor implementation is available.
- Cache-line values in the current profile are set to 64 bytes (6 = log2(64)); this should be checked against authoritative SDM660/Kryo 260 documentation and Linux device/kernel sources.

### Memory map / reserved memory facts

The audit reports several physical ranges and reservations, including:
- System RAM regions beginning around `0x40000000`, `0x88f00000`, `0x95000000`, and `0xa0400000`;
- persistent RAM around `0xa0000000`–`0xa03fffff`;
- reserved DSP/CDSP firmware region around `0x94a00000`;
- CMA reservation reported as 278,528 kB.

**These are observations from a running Android kernel, not yet an approved XNU memory map.** Derive a complete map from the actual bootloader-provided FDT/DTB and reserved-memory nodes. Never hard-code a guessed contiguous RAM span or overwrite firmware, modem, secure, crash-log, or CMA reservations.

### Interrupt / platform observations

The audit shows:
- GICv3 interrupt controller paths (also an MPM/GIC presentation);
- architectural timer IRQ;
- SDHCI / MMC controllers;
- KGSL GPU interrupt;
- ARM SMMU fault IRQs;
- I²C / SPI / UART;
- camera ISP/CSI;
- MDSS display interrupt;
- Qualcomm modem / ADSP / CDSP / WLAN-related interrupts;
- PMIC arbiter and power-key-related IRQs.

The device tree reports a Qualcomm MTP-compatible SDM660 tree. That is a strong starting point for SoC-level discovery, but the phone's exact FDT, board overlays, regulator assignments, and panel data still need to be handled carefully. A generic MTP compatible string is not proof that all MTP board wiring matches Lavender.

### Display and touch

- Kernel boot argument identifies a Tianma FHD+ DSI panel:
  `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`.
- Display resolution is 1080 × 2340.
- Touch controller appears as `NVTCapacitiveTouchScreen`.
- Linux input audit reports multitouch axes: X 0–1079, Y 0–2339, slots 0–9, pressure 0–1000.
- This identifies a likely NVT touch path; exact I²C address, IRQ, reset GPIO, regulators, and protocol must come from the actual device tree and kernel driver source. Do not guess them.

### Storage and boot

- The audit identifies the boot device as `c0c4000.sdhci`.
- Public Lavender device documentation describes eMMC 5.1; distinguish this from generic SDM660 capability tables and from any other storage interface visible on a different board.
- Boot arguments include DTB/DTBO indices and the panel name. These are runtime bootloader facts, not values to copy blindly into an Apple Device Tree.
- The user's bootloader is unlocked (per project conversation). Keep the initial path temporary and reversible.

---

## 4. Current repository and branch state

Repository: https://github.com/NAANG360/hackios-lavender-xnu

Known active branch: `ci/lavender-machine-profile`.

Latest head SHA positively associated with the latest recorded failed run:
`1842f223f3e3926710b9f53c86b586dfd9b62a99`

Latest recorded run:
- Run ID: `43198618602`
- Job ID: `131115903232`
- Branch: `ci/lavender-machine-profile`
- Run head SHA: `1842f223f3e3926710b9f53c86b586dfd9b62a99`
- Result: **failed** at the XNU build step.
- Workflow file: `.github/workflows/xnu.yml`
- Target: XNU 8796, `DEVELOPMENT`, `ARM64`, `LAVENDER`.

The current branch contains a Lavender-specific patch series. Confirm actual current branch HEAD before every new write; do not assume this SHA is still the branch tip.

### Relevant files already observed

- `.github/workflows/xnu.yml` — CI build workflow.
- `tools/validate-lavender-profile.sh` — static Lavender profile preflight.
- `xnu/patches/0082-lavender-machine-profile.patch` — machine config, plain ARM64, Lavender board header.
- `xnu/patches/0083-lavender-cache-config.patch` — 64-byte cache-line definitions.
- `xnu/patches/0085-lavender-gic-register-definitions.patch` — GICv3 register definitions.
- `xnu/patches/0087-lavender-linker-and-platform-expert.patch` — Lavender platform expert and IOKit personality.
- `xnu/patches/0088-lavender-config-groups.patch` — standard kernel configuration groups and feature sets.
- `xnu/patches/0089-lavender-exclude-sweet-sampler.patch` — limits SWEET-specific sampler to SWEET.
- `xnu/patches/0090-lavender-no-apple-monitor.patch` — no-op Apple monitor hook for Lavender as well as SWEET.

The profile validator checks expected symbols and patch syntax and prints:
`Lavender profile preflight passed (compile profile only; not boot-ready).`
This is explicitly a static preflight, **not** evidence that the kernel builds or boots.

### Existing repository README caveat

The branch's existing `README.md` and `NOTES.md` are SWEET-derived documentation for the Redmi Note 10 Pro / SM7150 port. They contain useful IOS-Sweet engineering context, but they are not an accurate current Lavender status report. This handoff supersedes their target-device status for this project. Preserve upstream SWEET history and don't silently rewrite SWEET facts as Lavender facts.

---

## 5. CI build configuration and latest failure

The observed workflow:
- runs on `macos-15-intel` (comment says macos-14 was retired);
- explicitly selects `/Applications/Xcode_16.0.app`;
- fetches pinned `blacktop/darwin-xnu-build` at:
  `663906528d1cd5ea8fde7de6f16de73db9511549`;
- stages repository patches into `dxb/patches/`;
- runs `tools/validate-lavender-profile.sh LAVENDER`;
- invokes `bash build.sh` with `MAKEFLAGS=-k` by default;
- builds `DEVELOPMENT / ARM64 / LAVENDER`;
- uploads build log, build tree, KDK library inspection output, and kernel output directory.

### Exact compile blocker from run 43198618602

The actual job log contains this compiler diagnostic:

```
.../xnu/iokit/KernelConfigTables.cpp
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```

This is the confirmed fatal compiler error in the recorded log. Earlier summaries mentioned `kalloc.h` and atomic headers, but the extracted fatal diagnostic above is the one confirmed from this run. Other output around `kalloc.h` / `atomic_private*.h` includes warning/note expansion context; do not mislabel those as confirmed fatal errors without a new log proving it.

The diagnostic points into the generated/compiled IOKit personality table. Patch `0087` inserts a Lavender platform-expert personality into `iokit/KernelConfigTables.cpp` using a conditional block. Its current added fragment is:

```cpp
#if defined(ARM64_BOARD_CONFIG_LAVENDER)
    "   {"
    "     'IOClass'         = LavenderPlatformExpert;"
    "     'IOProviderClass' = IOPlatformExpertDevice;"
    "     'IONameMatch'     = 'qcom,sdm660-mtp';"
    "     'IOProbeScore'    = 10000:32;"
    "   },"
#endif
```

**Primary next investigation:** inspect the exact upstream XNU 8796 source around `iokit/KernelConfigTables.cpp` and the patch application result, then inspect the generated file around line 93 from the CI build tree/log. Determine whether:
- the surrounding C++ string literal was already terminated before this conditional;
- this source file is generated from a personality source and the patch is applied at the wrong stage;
- the conditional splits an initializer/string literal in a way that is invalid C++;
- the correct mechanism is to add a personality to the generated source input rather than directly patching the generated table;
- or the string encoding/IOKit personality grammar is invalid.

Do not just delete the Lavender platform expert or silence the compiler. Fix the source-of-truth generation/patch mechanism. A safe first experiment is to remove the injected conditional personality from the generated C++ file and instead register the service through a mechanism supported by this XNU tree, but only after understanding how the upstream table is generated and how SWEET's personality is declared.

### Artifact reality from the failed run

The workflow used `if: always()`, so artifacts exist even though the build failed:
- `xnu-buildtree`: artifact ID `13936585997`, 22,350,351 bytes, SHA-256 `dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf`. Useful for local diagnosis; excludes many compiled objects.
- `xnu-kernel`: artifact ID `13935079884`, 178,298 bytes, SHA-256 `58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4`. This is **not a usable kernel**. The output directory contained only `kdk-libs/` metadata dumps; `out/` had no `kernel.*` file. The log explicitly says `shasum: kernel.*: No such file or directory`.

The workflow also tried to copy Xcode's private `Kernel.framework/.../PrivateHeaders/platform` directory, but the path was absent:
`cp: .../MacOSX15.0.sdk/.../PrivateHeaders/platform: No such file or directory`.
That is a separate packaging/build-environment issue to investigate; it was not the primary compile error. The build-tree artifact was still uploaded.

**Never call the 178 KB `xnu-kernel` artifact a working kernel.**

---

## 6. Current Lavender profile design — observed, not yet boot-validated

### Machine configuration

Patch `0082`:
- adds `LAVENDER` to `SUPPORTED_ARM64_MACHINE_CONFIGS`;
- defines `MACHINE_FLAGS_ARM64_LAVENDER = -DARM64_BOARD_CONFIG_LAVENDER -march=armv8-a`;
- prevents Lavender from being promoted to arm64e;
- adds `pexpert/pexpert/arm64/LAVENDER.h`;
- defines `MAX_CPUS 8`, `MAX_CPU_CLUSTERS 2`, `HAS_GIC_V3 1`, `GIC_SPURIOUS_IRQ 1023`, `GICR_PE_SIZE 0x20000`, and architecture-level flags;
- comments explicitly state that no RAM map, timer INTID, GIC security routing, MMIO base, SDHCI, panel, PMIC, or NVT programming is implied.

Patch `0083` sets:
- `MMU_I_CLINE 6`;
- `MMU_CLINE 6`;
for the Lavender branch.

Patch `0085` adds GICv3 register constants, including distributor control and redistributor offsets. These definitions require review against the exact GIC implementation and the public SDM660 device tree before relying on them at runtime.

Patch `0087` adds `LavenderPlatformExpert`, a minimal `IODTPlatformExpert` subclass. It reports target name `lavender` and product name `Redmi Note 7`, processes the device tree and tries to match `qcom,sdm660-mtp`. It is not a hardware driver and does not initialize display, touch, storage, power, or CPU services.

Patch `0088` imports the standard XNU config feature groups (BSD, filesystems, networking, IOKit, libkern, scheduler, VM, security, etc.) for the Lavender config. It includes options that may assume Apple-specific infrastructure; audit these rather than treating the broad feature set as automatically valid for Qualcomm.

Patch `0089` excludes the SWEET-specific process sampler for non-SWEET builds.

Patch `0090` makes `ml_enable_monitor()` a no-op for Lavender as well as SWEET because the Apple monitor/PPL implementation is not present for this Qualcomm target. Verify every call path and security consequence; this is a bring-up compatibility measure, not a complete security architecture.

### Critical architecture questions still open

- What is the actual expected XNU boot contract on this Qualcomm bootloader: direct kernel handoff, shim, UEFI, or another loader?
- Which boot image format and kernel entry ABI will be used?
- How will the bootloader's FDT be preserved and translated into an Apple Device Tree / IOKit registry tree?
- What page size is required by this XNU configuration and supported by the target MMU/loader? The profile currently labels 16K pages as the iOS 16 target, but this is not runtime-verified.
- What is the actual GICv3 distributor/redistributor base and security group routing?
- What is the architectural timer interrupt ID and timer frequency?
- What are the PSCI conduit and CPU enable method for each CPU cluster?
- What RAM ranges are safe for the kernel, page tables, boot arguments, initrd, framebuffer and logs?
- Which Apple platform services can be stubbed safely and which are essential to reach `launchd`?
- Does the XNU 8796 build depend on Apple-only KDK libraries or platform headers unavailable on the generic Xcode runner? Inventory unresolved symbols and link inputs instead of assuming.

---

## 7. External reference projects and how to use them

### A. HakiOS / hackIOS

- Main repository: https://github.com/oosexgang/hakios-xnu-kernel
- Porting guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/porting/index.md
- Documentation index: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/index.md
- Build/boot documentation is available through the HakiOS documentation site: https://hackios-web.vercel.app/

Use HakiOS as a reference for:
- modern XNU-on-Android port architecture;
- per-device and per-SoC source separation;
- UEFI/loader/boot chain design;
- booting from RAM and preserving stock boot partitions;
- device-tree-driven port bring-up;
- driver option gating in XNU config;
- separating common XNU code from device-specific code;
- a staged port process: first boot/CPU/memory/interrupts, then individual drivers.

Do not transplant a different phone's SoC drivers or addresses into Lavender. HakiOS's supported Qualcomm devices use other SoCs (for example SC7280-family hardware); they are architectural examples, not proof of SDM660 compatibility.

### B. IOS-Sweet

- Main repository: https://github.com/dipa-studio/ios-sweet
- README: https://github.com/dipa-studio/ios-sweet/blob/main/README.md
- Engineering notes: https://github.com/dipa-studio/ios-sweet/blob/main/NOTES.md

IOS-Sweet is especially relevant because it ports XNU 8796 / iOS 16-era userland to another Xiaomi Qualcomm Android phone (Redmi Note 10 Pro, codename `sweet`, SM7150 / Snapdragon 732G). It demonstrates:
- a patch series applied in CI;
- a RAM-boot shim;
- CPU, GICv3 and timer bring-up;
- an Apple Device Tree generated from Android hardware data;
- framebuffer diagnostics and persistent logging;
- a staged path from kernel boot to IOKit services and Darwin userspace;
- software display rendering before GPU acceleration.

The SWEET hardware is different. Do not copy:
- CPU topology assumptions (Cortex-A55/A76 layout and enabled cores);
- Kryo 470-specific code;
- MMIO addresses, RAM banks, IRQ numbers, timer IDs, GIC routing;
- UFS controller assumptions;
- display/touch controller drivers or panel details;
- power management and regulator sequencing.

Port the method and generic XNU techniques, not SWEET's hardware constants. Check licensing before copying code; retain required notices and comply with the upstream licenses.

### C. Public Lavender / SDM660 references

- OrangeFox Lavender device tree: https://gitlab.com/OrangeFox/device/lavender
- Lavender kernel source cited by that tree: https://github.com/pix106/android_kernel_xiaomi_southwest-4.19/tree/main-dynamic
- LineageOS Lavender device information: https://wiki.lineageos.org/devices/lavender/
- Qualcomm SDM660 product page: https://www.qualcomm.com/processors/application-processors/products/sdm660

Use these to derive:
- the real Lavender board tree and overlays;
- regulators, clocks, GPIOs, I²C/SPI buses, panel and touch wiring;
- the Qualcomm SDM660 platform topology;
- kernel driver implementations and DT bindings;
- board-specific differences from generic SDM660 MTP.

Prefer the exact running device's DTB/DTBO and matching kernel source over generic product marketing specifications.

---

## 8. Recommended next engineering sequence

### P0 — Make the current CI build compile

1. Fetch current branch HEAD and inspect the complete patch list/order and current workflow.
2. Reproduce the exact XNU 8796 source tree from the pinned `dxb` revision and patch sequence.
3. Inspect `iokit/KernelConfigTables.cpp` around the offending line and trace how it is generated.
4. Inspect the generated file around line 93 from the build-tree artifact if it can be downloaded; compare with the original source and the post-patch source.
5. Fix the source-of-truth patch/generator for the Lavender personality.
6. Add a lightweight validation that compiles or syntax-checks the affected C++ translation unit, not merely greps for strings.
7. Run a single focused CI build. Keep `MAKEFLAGS=-k` if it provides useful complete diagnostics, but avoid repeated expensive full builds without a new hypothesis.
8. If the next blocker is link-time, capture all unresolved symbols and identify their provider/ABI. Do not stub symbols blindly.
9. Fix the workflow packaging so a missing kernel causes a hard failure rather than a misleading successful artifact upload. Use explicit checks such as `test -s out/kernel.development.*` (adapt to the actual expected filename) and validate Mach-O type/architecture before uploading the kernel artifact.

### P1 — Verify the kernel artifact

- Confirm file exists and is non-empty.
- Inspect with `file`, `lipo -info`, `otool -hv`, `otool -l`, `nm`, and the appropriate XNU/Mach-O tooling.
- Confirm ARM64 (not arm64e, x86_64, or an Apple Silicon-only binary).
- Inspect kernel entry, segments, link address, relocations, symbol resolution, and any required platform/KDK imports.
- Hash and attach artifact; record all checks and failures.

### P2 — Prepare safe physical boot

Before a phone boot:
- identify and document the bootloader handoff and shim requirements;
- derive FDT and memory map from the real device;
- verify all memory ranges and reserved regions;
- implement early diagnostics that survive panic/hang (framebuffer and/or persistent RAM; serial availability is not established);
- ensure the image is temporary and has a tested return path to Android/recovery;
- do not overwrite boot, recovery, vbmeta, modem, persist, or GPT.

### P3 — Bring-up order

1. Entry point and early exception/vector setup.
2. MMU and page tables.
3. GICv3 distributor/redistributor and interrupt masking/dispatch.
4. Architectural timer.
5. CPU topology / PSCI / secondary CPU start.
6. Physical memory allocator and VM.
7. Platform expert / DT discovery.
8. Basic console/framebuffer.
9. Storage, initrd/root mount.
10. IOKit essentials, touch, power, etc.

Use one hardware subsystem per patch and keep early-boot logging explicit. Never hide a failed initialization behind a fake success flag.

---

## 9. Latest status snapshot

| Area | Status | Evidence |
|---|---|---|
| Hardware identity | <span>Observed</span> | Two device audit files, 2026-11-12 |
| CPU / SoC identification | <span>Observed</span> | SDM660, 8 CPUs, 2 clusters |
| Basic Lavender XNU machine profile | <span>In source</span> | Patches 0082–0090 |
| Static profile preflight | <span>Passed</span> | Run 43198618602 step 5 |
| XNU compile | <span>Failed</span> | Run 43198618602, KernelConfigTables.cpp line 93 |
| Kernel Mach-O artifact | <span>Not produced</span> | xnu-kernel artifact had only KDK library metadata |
| Boot shim / loader verified for Lavender | <span>Not established</span> | No verified device boot evidence in this project record |
| Actual XNU boot on Lavender | <span>Not achieved</span> | No device boot logs |
| Display / touch under XNU | <span>Not achieved</span> | Linux audit only |
| Darwin userspace / SpringBoard | <span>Not achieved</span> | Future milestone |

Avoid assigning a flattering overall percentage. Report concrete milestone states and evidence. A theoretical progress percentage must be labelled as an estimate and must not be confused with build/boot success.

---

## 10. Cross-session working protocol

At the start of each new session:

1. Read this file first.
2. Inspect current branch HEAD and latest Actions runs; this document can become stale.
3. Inspect the newest failed build log and exact artifacts before making claims.
4. Check whether the latest blocker is already fixed on the branch.
5. Use HakiOS, IOS-Sweet and public Lavender/SDM660 references as specified above.
6. Make the smallest source-of-truth fix, commit it, and run the corresponding validation/build.
7. Do not rerun unchanged failing code without a reason.
8. Update this file after each milestone with:
   - date/time;
   - branch and commit SHA;
   - exact workflow run and job IDs;
   - exact diagnostics;
   - patch/file changes;
   - artifact IDs and hashes;
   - whether the kernel was actually produced and verified;
   - remaining next action.
9. Keep raw device dumps, serials, unique IDs, calibration data, firmware blobs and personal data out of Git.
10. Never claim a build, boot, test, or flash happened unless tool/device evidence proves it.

### Update template

```markdown
## Update YYYY-MM-DD HH:MM IST
- Branch / commit:
- Change:
- CI run / job:
- Result:
- Exact first fatal diagnostic:
- Artifact:
- Kernel verification:
- Physical device test:
- Next concrete action:
```

---

## 11. Source links

- Project repository: https://github.com/NAANG360/hackios-lavender-xnu
- HakiOS XNU port: https://github.com/oosexgang/hakios-xnu-kernel
- HakiOS porting guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/porting/index.md
- IOS-Sweet: https://github.com/dipa-studio/ios-sweet
- IOS-Sweet engineering notes: https://github.com/dipa-studio/ios-sweet/blob/main/NOTES.md
- OrangeFox Lavender device tree: https://gitlab.com/OrangeFox/device/lavender
- Lavender 4.19 kernel source: https://github.com/pix106/android_kernel_xiaomi_southwest-4.19/tree/main-dynamic
- LineageOS Lavender device page: https://wiki.lineageos.org/devices/lavender/
- Qualcomm SDM660: https://www.qualcomm.com/processors/application-processors/products/sdm660
- Failed CI run: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602
- Failed build job: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602/job/131115903232
- Build-tree artifact: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602/artifacts/13936585997
- **Not-a-kernel** output artifact (diagnostic only): https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602/artifacts/13935079884


---

## 12. Consolidated continuation notes — 2026-11-12 22:10 IST

This section records the latest session's additional verified findings and clarifies the reference-source strategy. The sections above remain the primary engineering handoff.

### User's project intent, in plain terms

The user wants a real XNU kernel port for the physical Redmi Note 7 / Lavender (SDM660), with the kernel as the immediate deliverable. The broader project is an iOS/Darwin-on-Lavender experiment (the user has casually referred to it as LIOS / “Lucid iOS”). The current priority is **kernel compilation and static verification first**. Do not spend time on SpringBoard, polish, GPU, or a full iOS root filesystem until the kernel artifact exists and is verified.

The user has explicitly asked that engineering use all three reference families:
1. hackIOS / HakiOS;
2. IOS-Sweet;
3. public Lavender and SDM660 device/kernel documentation.

Treat these as complementary evidence sources, not as interchangeable hardware targets.

### Reference-source findings and transfer boundaries

**HakiOS / hackIOS**
- Repository: https://github.com/oosexgang/hakios-xnu-kernel
- Porting guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/porting/index.md
- The porting guide's useful method is to start with the phone's own FDT/device tree and firmware, establish a bootable ARM64/Qualcomm boot chain, then add device services/drivers one at a time.
- Its documented Qualcomm device examples are not SDM660 Lavender. Use its architecture, loader, boot-from-RAM, device-description and staged-porting methods. Do not import device-specific MMIO addresses, PSCI details, IRQs, panel data or driver assumptions without Lavender evidence.

**IOS-Sweet**
- Repository: https://github.com/dipa-studio/ios-sweet
- It is a separate Redmi Note 10 Pro port (sweet, SM7150 / Snapdragon 732G), using XNU 8796 / iOS 16-era components.
- Its public README documents a shim → XNU → storage/root → Darwin userspace chain, and its NOTES record concrete lessons about GICv3, timers, PSCI, MMU, framebuffer diagnostics, generated Apple Device Tree, touch and software rendering.
- Its engineering notes are valuable for XNU-specific mechanics and failure analysis. The Sweet hardware constants are **not** Lavender constants. In particular, do not copy Sweet's Kryo 470 / A55-A76 CPU map, UFS assumptions, RAM layout, timer/IRQ assignments, touch controller, panel, regulators or MMIO addresses.
- The Lavender branch's root README and NOTES are inherited Sweet documentation. They are useful reference material only; they are not a record that Sweet code or drivers have been ported successfully to Lavender. This file is the canonical Lavender status.

**Public Lavender / SDM660 material**
- Xiaomi SDM660 common platform tree: https://github.com/xiaomi-sdm660/android_device_xiaomi_sdm660-common
- Xiaomi Lavender device tree: https://github.com/xiaomi-sdm660/android_device_xiaomi_lavender
- Alternative Lavender device tree: https://github.com/ParsimoniousLavender/device_xiaomi_lavender
- Alternative SDM660 common tree: https://github.com/ParsimoniousLavender/device_xiaomi_sdm660-common
- Public SDM660 device/kernel source index: https://github.com/Device-Tree-for-Redmi-Note-7-lavender
- Xiaomi open-source kernel collection (contains lavender-p-oss and lavender-q-oss source releases): https://github.com/MiCode/Xiaomi_Kernel_OpenSource
- Qualcomm's SDM660 specification: https://www.qualcomm.com/processors/application-processors/products/sdm660
- OrangeFox Lavender device tree and its cited 4.19 kernel source: https://gitlab.com/OrangeFox/device/lavender and https://github.com/pix106/android_kernel_xiaomi_southwest-4.19/tree/main-dynamic
- These sources describe different kernel generations (4.4, 4.19, Android P/Q and newer downstream forks). Record the exact branch/commit and firmware generation for every fact imported. The currently running Android 15 kernel in the audit is 4.19.325-NYXION-SUSFS; that does not make an old Android P/Q DTS or vendor tree identical to the current device's live FDT.

### Additional device-audit details worth retaining

From the 2026-11-12 uploaded audit files:
- The final sanitized kernel command line identifies the Tianma panel as qcom,mdss_dsi_nt36672a_tianma_fhdplus_video.
- Touch input is exposed as NVTCapacitiveTouchScreen; the audit reports 10 multitouch slots, X 0–1079, Y 0–2339 and pressure 0–1000.
- The live device tree identifies the model as Qualcomm SDM 660 PM660 + PM660L MTP F7A overlay and compatibles qcom,sdm660-mtp, qcom,sdm660, qcom,mtp.
- The boot controller is c0c4000.sdhci; do not infer a UFS storage controller from Snapdragon family-level capabilities.
- Linux exposes 8 logical CPUs / two clusters, GICv3-related interrupts, KGSL/Adreno, MDSS, SMMU and Qualcomm peripheral interrupts. The audit is Linux runtime evidence; it is not proof that XNU has drivers for any of them.
- Raw audit files remain conversation attachments. Do not commit them; they contain device-specific runtime and identifier material. Keep only sanitized technical facts in Git.

### Latest CI run — exact verified state

Run: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602  
Job: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43198618602/job/131115903232  
Branch: ci/lavender-machine-profile  
Run head: 1842f223f3e3926710b9f53c86b586dfd9b62a99  
Workflow file blob at inspection: 8bf0c956cfc61d904fd2a7e400c40a0dd34197c9  
Result: **job conclusion = failure**.

The job steps show:
- checkout, Xcode selection, pinned build-tool fetch, profile validation and dependency install succeeded;
- profile preflight succeeded;
- the XNU 8796 build failed;
- packaging and artifact upload steps ran because they use if: always().

The workflow is configured for macos-15-intel, explicitly selects Xcode 16.0, fetches blacktop/darwin-xnu-build at 663906528d1cd5ea8fde7de6f16de73db9511549, and defaults to DEVELOPMENT / ARM64 / LAVENDER. keep_going defaults to true (make -k) so the build can surface more than one independent compile failure. Its concurrency group includes the run ID and cancel-in-progress: false, so a later run should not cancel an earlier run merely because it targets the same ref.

Confirmed fatal diagnostic:
```
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```
This is the first concrete blocker to fix. It is in the IOKit personality table compilation path and correlates with the Lavender personality injection in patch 0087-lavender-linker-and-platform-expert.patch. Inspect the exact post-patch/generated file and the XNU table-generation mechanism before changing it. Do not blindly remove the platform expert or replace it with a fake registration.

Other observations in the same run:
- the patch driver skipped iobuffermemd_monterey.patch and skywalk_sonoma.patch because hunks did not apply to this XNU source; TrustCache.patch was skipped because its files already existed. These are patch-series hygiene issues to review, but they are not the confirmed line-93 fatal error.
- the Xcode SDK copy of Kernel.framework/.../PrivateHeaders/platform failed because the path was absent. This is a separate build-tree packaging issue.
- the build exited with code 2.
- xnu-buildtree artifact ID 13936585997, 22,350,351 bytes, SHA-256 dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf.
- xnu-kernel artifact ID 13935079884, 178,298 bytes, SHA-256 58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4. The workflow's out/ directory contained only kdk-libs/ inspection metadata; there was no kernel.* output, and shasum reported no matching kernel file. **This artifact is not a kernel.**
- The workflow should be hardened so the kernel artifact step fails if the expected kernel file is missing or invalid. Do not let if: always() turn an empty output directory into a seemingly successful kernel artifact.

### Concrete next actions for the next engineering session

1. Re-read this file, inspect current branch HEAD and latest workflow run. The recorded head above is historical, not guaranteed to be today's branch tip.
2. Download/read the xnu-buildtree artifact if still available and inspect the generated iokit/KernelConfigTables.cpp around line 93. Compare the upstream XNU 8796 source, post-patch source and any generator input.
3. Fetch the current 0087 patch and identify exactly how the personality block enters KernelConfigTables.cpp. Correct the source-of-truth mechanism, not a generated build output.
4. Re-run a focused syntax/compile check for the affected IOKit table before a full Actions build if the build environment permits.
5. Review patch application skips and the Xcode 16 / XNU 8796 compatibility path, but keep the primary fatal error isolated.
6. Run one full CI build after a real change. Keep the log and compare the first fatal diagnostic against this run.
7. If compilation succeeds, require a real ARM64 Mach-O kernel, inspect file type/architecture/load commands/entry/symbols/relocations and unresolved dependencies, then record SHA-256.
8. Only after static verification prepare a RAM-only boot test. Never flash boot/recovery/vbmeta or repartition as part of this kernel milestone.

---

## 13. Repository discoverability and branch map — 2026-11-12

### Canonical entry point

The handoff is already present at the repository root as `PROJECT_HANDOFF.md`. The default-branch `README.md` begins with a Lavender-port notice linking to both `AGENTS.md` and this file. `AGENTS.md` explicitly tells every agent to read this handoff first. The `ci/lavender-machine-profile` README also points readers to it.

At the time of this inspection, the `PROJECT_HANDOFF.md` blob was identical on `main` and `ci/lavender-machine-profile` (blob SHA `c00bb3fc9c675aac92c317fcc9986a21429e5ab0`). Keep both copies synchronized when updating the handoff. The intended new-session discovery path is:

1. Open repository: https://github.com/NAANG360/hackios-lavender-xnu
2. Read `AGENTS.md`.
3. Read `PROJECT_HANDOFF.md` in full.
4. Inspect current branch HEAD and latest Actions run before relying on this document's snapshot.

### Branches visible in GitHub branch search

The repository branch search returned these names on 2026-11-12:

- `main`
- `ci/lavender-machine-profile`
- `ci/xnu-reference-no-cancel`
- `port/lavender-platform-baseline`
- `fix/lavender-config-table-placement`
- `fix/lavender-kernel-config-table`
- `fix/lavender-kernelconfig-compile`
- `fix/lavender-kernelconfig-table-placement`
- `fix/lavender-kernelconfigtables-build`
- `fix/lavender-remove-invalid-kernel-config-table`

**Important:** this is only a branch-name inventory. The contents, HEAD commits, ancestry, CI status and relative quality of all those fix branches were not established in this inspection. Do not assume the branch with the most promising name contains a correct fix. Before choosing a working branch, compare each candidate's commit history and diffs against `ci/lavender-machine-profile`, inspect its latest workflow and verify whether it removes, relocates or correctly regenerates the Lavender IOKit personality. Avoid losing any useful work by force-resetting or deleting branches.

### Main branch versus the Lavender working branch

- `main` still has the inherited SWEET workflow: default `MACHINE_CONFIG=SWEET`, `macos-14`, and `cancel-in-progress: true`. Do **not** dispatch the default-branch workflow expecting it to build Lavender.
- `ci/lavender-machine-profile` has the Lavender workflow observed in this session: default `MACHINE_CONFIG=LAVENDER`, runner `macos-15-intel`, explicit Xcode 16.0 selection, Lavender profile preflight, and a concurrency group containing the run ID with `cancel-in-progress: false`.
- Treat `ci/lavender-machine-profile` as the last confirmed Lavender build branch in this record, but inspect the fix branches before continuing because they may contain later experiments.
- The `main` README banner and `AGENTS.md` make the handoff discoverable even when a new agent lands on the default branch. Preserve that entry-point link.

---

## 14. Additional project intent and user constraints from prior sessions

These are project-specific requirements gathered across the user's earlier porting conversations. They are not hardware verification results.

- The user wants an iOS/Darwin port on the actual Redmi Note 7, sometimes casually calling the project **LIOS** (“Lucid iOS”). The immediate goal is a working Lavender XNU kernel, not a polished UI.
- The user repeatedly asked for a flashable/bootable image quickly, but also clarified that the first bring-up should be temporary/untethered so they can verify it before installation. Resolve this as: build and statically verify the kernel first; prepare a reversible RAM-only boot path next; do not flash or overwrite partitions without explicit permission.
- The user asked whether an OrangeFox recovery-terminal script could eventually boot the system. Treat that as a later convenience wrapper around a verified loader/payload; it does not replace the need to establish the correct boot ABI and memory map.
- The user explicitly said not to use an emulator as the target system or as evidence that the real phone works. A QEMU test is permissible only for isolated kernel logic and must be labelled as such.
- The user wants actual source edits, CI runs, and artifacts rather than repeated promises to inspect or generic status reports. When an action cannot be completed, state exactly which tool/access/build limitation blocked it and preserve the next actionable step in this file.
- The user wants the work to continue as an iterative diagnose → smallest justified patch → validation → CI build → artifact verification loop until a real XNU kernel is produced. No assistant can guarantee a CI process runs indefinitely after a session ends; leave durable commits, build logs, artifacts and updated handoff notes after each session.
- The phone's power button is broken. Avoid recovery/boot instructions that require it; the user previously asked about volume-up + volume-down alternatives, but do not claim a key sequence is verified unless the current bootloader/recovery evidence confirms it.
- The user previously suggested mining their separate Lucid-OS Android/Linux project for SDM660 driver knowledge. If that repository is explicitly in scope, use it as a **supplemental Linux hardware reference** for register layouts, device-tree nodes, regulators, clocks, GPIOs, panel/touch configuration and sequencing. Linux drivers are not XNU drivers; translate the hardware facts and implement the relevant XNU/IOKit interface rather than transplanting Linux code or assuming its APIs work.
- Do not ask the user to repeat hardware information already present in the two uploaded audits or repository artifacts. Do not put raw audit dumps, device serials, calibration data, private firmware or proprietary iOS root files into Git.

---

## 15. Additional confirmed CI log observations

The failed run's patch application log also reported these skips/failures before compilation:

- `0031-sweet-read-write-root.patch` was skipped because `bsd/sweet_hfs/core/hfs_vfsutils.c` was absent.
- `0041-sweet-hfs-link-notes.patch` was skipped because `bsd/sweet_hfs/core/hfs_catalog.c` was absent.
- `iobuffermemd_monterey.patch` failed to apply at `iokit/Kernel/IOBufferMemoryDescriptor.cpp:332`.
- `skywalk_sonoma.patch` failed to apply at `bsd/skywalk/packet/os_packet.h:883`.
- `syntax_checker_sonoma.patch` failed to apply at `tools/lldbmacros/core/syntax_checker.py:1`.
- `TrustCache.patch` was skipped because its `EXTERNAL_HEADERS/TrustCache/*.h` files already existed.
- The workflow proceeded to apply alternative/other patches after those warnings and reached the XNU build.

These are patch-series compatibility/hygiene findings, not the confirmed `KernelConfigTables.cpp:93` compiler failure. For each skipped patch, inspect the pinned XNU source and determine whether the intended change is already upstream, unnecessary, or genuinely missing. Do not blindly force-apply old Sweet/macOS patches to XNU 8796.

The same job emitted warnings/notes involving `kalloc.h`, startup macro expansion and atomic headers. The extracted fatal compiler error in this run is the IOKit table error recorded in Section 5. Only promote another diagnostic to a blocker if a fresh log shows an actual `error:` that stops a compile/link target.

---

## 16. Session snapshot and immediate continuation — 2026-11-12

### What was verified in this session

- Confirmed that `PROJECT_HANDOFF.md` exists at the repository root and is linked from the default-branch README and `AGENTS.md`.
- Confirmed the handoff file is also present on `ci/lavender-machine-profile`.
- Confirmed the main branch's XNU workflow is still SWEET-default and should not be mistaken for the Lavender workflow.
- Confirmed the Lavender working branch's workflow selects `LAVENDER`, runs on `macos-15-intel`, explicitly selects Xcode 16.0 and disables run cancellation through a run-specific concurrency key.
- Confirmed the branch-name inventory in Section 13. The fix branches need comparison; their contents and HEAD SHAs remain unverified.
- Re-fetched run `43198618602` / job `131115903232` logs. The job failed at the XNU build step; profile validation passed. The fatal diagnostic remains `KernelConfigTables.cpp:93:5: expected unqualified-id`.
- Confirmed the failed run's nominal `xnu-kernel` artifact contains no kernel file. It is only a KDK metadata bundle and must not be used as a bootable image.
- No source-code fix, successful kernel build, verified Mach-O kernel, or physical-device boot was produced by this session.

### Immediate next actions (do these, do not restart from scratch)

1. Inspect the branch candidates in Section 13. Determine their HEAD commits and compare their `0087`/KernelConfigTables changes. Select the best evidence-backed candidate without discarding existing work.
2. Inspect the actual `xnu-buildtree` artifact (ID `13936585997`) while it remains available. Extract the post-patch `iokit/KernelConfigTables.cpp` around line 93 and the exact patch application result.
3. Trace the XNU 8796 source-generation path for KernelConfigTables. Establish whether the table is generated from personality inputs and whether patch 0087 modifies a generated output or splits an invalid C++ initializer.
4. Fix the source-of-truth personality registration mechanism. Preserve a valid Lavender platform-expert registration; do not simply delete the service or silence the compiler.
5. Add a real compile/syntax validation for the affected translation unit and hard-fail artifact collection if the expected kernel output is absent.
6. Dispatch the Lavender workflow only after a meaningful fix. Compare the next run's first fatal diagnostic with run 43198618602.
7. Once the build succeeds, prove that the artifact is an ARM64 Mach-O kernel, inspect its load commands/entry/symbols/relocations and dependency closure, compute SHA-256, and update this file.
8. Only then move to the RAM-only device boot chain. No flashing or destructive partition work.

### Truthful status at this snapshot

- **Working Lavender XNU kernel:** not yet produced.
- **Static Lavender profile preflight:** passed in the recorded failed run.
- **XNU compilation:** failed at the IOKit KernelConfigTables translation unit.
- **Kernel artifact:** absent; the uploaded 178 KB artifact is not a kernel.
- **Physical Lavender boot:** not attempted/verified in the project evidence recorded here.

This status is deliberately evidence-based. Do not substitute an optimistic percentage for the concrete state above.


---

## 12. Session consolidation — 2026-11-12 22:06 IST

This section records the latest cross-session review. It supplements the preceding handoff; it does not claim that a build fix or kernel was produced.

### What was re-verified in this session

- The root README already points new contributors to this file as the authoritative handoff. Keep that pointer at the top of README.
- The known branch is `ci/lavender-machine-profile`; the recorded failing run's head remains `1842f223f3e3926710b9f53c86b586dfd9b62a99`. Re-fetch the branch tip before any subsequent write because the branch may have advanced.
- `PROJECT_HANDOFF.md` already existed at repository root. This file is the one canonical handoff; do not create a competing handoff with a different name.
- The workflow source was re-read. It selects `macos-15-intel`, explicitly selects Xcode 16.0, fetches `blacktop/darwin-xnu-build` at `663906528d1cd5ea8fde7de6f16de73db9511549`, validates the LAVENDER profile, then builds XNU 8796 in DEVELOPMENT / ARM64 / LAVENDER with `MAKEFLAGS=-k` by default.
- The profile validator is explicit that it only checks a compile profile and patch syntax; it prints “compile profile only; not boot-ready.” It is not a build or boot test.

### Full-log clarification: fatal errors versus macro expansion notes

The complete recorded job log was queried again and filtered for compiler diagnostics.

The confirmed fatal C++ diagnostic is still:

```
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```

It occurs while compiling `iokit/KernelConfigTables.cpp` and correlates with the conditional Lavender personality block added by `0087-lavender-linker-and-platform-expert.patch`. The exact cause is not yet proven. Inspect the source file's surrounding C++ syntax, how the XNU 8796 table is generated, and the post-patch result before changing it.

The log also contains large numbers of notes and caret expansions through `osfmk/kern/kalloc.h`, `libkern/os/atomic_private.h`, and `atomic_private_impl.h`. In the extracted diagnostic output, these were macro-expansion context, not independently established fatal errors. Do not list them as separate confirmed blockers unless a future log contains a primary compiler `error:` diagnostic pointing to them.

### Patch-series warnings observed in that run

The patch driver logged skips/failures for these reference-era patches:

- `0031-sweet-read-write-root.patch`: target `bsd/sweet_hfs/core/hfs_vfsutils.c` was absent.
- `0041-sweet-hfs-link-notes.patch`: target `bsd/sweet_hfs/core/hfs_catalog.c` was absent.
- `iobuffermemd_monterey.patch`: hunk failed at `iokit/Kernel/IOBufferMemoryDescriptor.cpp:332`.
- `skywalk_sonoma.patch`: hunk failed at `bsd/skywalk/packet/os_packet.h:883`.
- `syntax_checker_sonoma.patch`: hunk failed at `tools/lldbmacros/core/syntax_checker.py:1`.
- `TrustCache.patch`: skipped because the TrustCache headers already existed in the source tree.

These are patch compatibility/hygiene warnings. The log continued beyond them; they are not the confirmed `KernelConfigTables.cpp` fatal diagnostic. Audit whether each skipped patch is actually required for the Lavender target before changing the patch driver. Do not force-apply SWEET-only changes merely to make the log quiet.

### Artifact and packaging confirmation

- The `xnu-buildtree` artifact is a 22,350,351-byte diagnostic bundle (artifact ID `13936585997`, SHA-256 `dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf`). It can help inspect the build tree but excludes object files and the final kernel.
- The `xnu-kernel` artifact is 178,298 bytes (artifact ID `13935079884`, SHA-256 `58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4`). The uploaded directory held only `kdk-libs/` metadata; there was no `kernel.*` output. It is not a kernel.
- The workflow's attempt to copy Xcode's private `Kernel.framework/.../PrivateHeaders/platform` directory failed because the expected path did not exist under the selected Xcode 16.0 / macOS 15 SDK. This is a separate packaging/build environment defect.
- The final artifact step currently uses `if: always()`; therefore artifact existence alone is not a success signal. Harden the workflow so it fails if no expected kernel is present, and verify the Mach-O before naming/uploading an artifact as a kernel.

### Reference-source discipline

The user's explicit reference set is:
1. **hackIOS / HakiOS** — use for XNU-on-Android architecture, loader/boot flow, device separation, RAM-only boot design and porting process. Treat other-device support as architectural reference, not SDM660 proof.
2. **IOS-Sweet** — use for the XNU 8796/iOS 16-era workflow, shim, patch organization, diagnostics and staged bring-up. It targets Redmi Note 10 Pro / SWEET / SM7150, not Lavender.
3. **Public Lavender and SDM660 documentation/source** — use for the actual board tree, device tree, kernel drivers, SoC registers, clocks, regulators, GPIOs, storage and panel/touch wiring. Prefer the matching device tree and kernel branch for the user's running firmware generation.
4. **The two uploaded Lavender audits** — use as runtime evidence for this exact phone. They are not substitutes for the source DTS or bootloader FDT, and raw dumps must remain out of Git.

### Explicit non-results

As of this consolidation:
- No verified XNU 8796 ARM64 Mach-O kernel for Lavender has been produced.
- No verified XNU boot on the physical Redmi Note 7 has occurred.
- No display, touch, storage or Darwin userspace subsystem has been demonstrated under XNU on this device.
- No new source fix or successful build is recorded by this session.
- No partition has been flashed by this work.

### Immediate next action — do this before another full CI run

1. Re-fetch current branch HEAD.
2. Obtain and inspect artifact `13936585997` while it is still retained (recorded expiry: 2026-11-19 10:45:59 UTC). Find the actual patched/generated `iokit/KernelConfigTables.cpp` and the relevant generator inputs.
3. Compare the table source before/after patch 0087. Determine whether the inserted `#if` is outside a valid adjacent-string-literal sequence, inside a raw/string literal, or applied to a generated output at the wrong stage.
4. Fix the authoritative source/generator/personality registration mechanism. Add a targeted syntax check so the same defect is caught cheaply.
5. Re-run the build. Do not proceed to hardware claims until a non-empty ARM64 Mach-O kernel exists and passes binary inspection.



---

## 17. Consolidation update — 2026-11-12 22:08 IST

This section records a fresh read of the repository, the failed CI job, the branch inventory, the uploaded audits, and the external references during the handoff-consolidation session. It supplements Sections 1–16; it does not supersede the evidence or cautions above.

### Canonical handoff location and discovery

- Canonical document: PROJECT_HANDOFF.md at the repository root.
- The root README.md on both main and ci/lavender-machine-profile links to this document and directs readers to start here.
- AGENTS.md also points new contributors/agents at the handoff.
- New sessions should open the repository, read AGENTS.md and then this file before inspecting or editing code. Do not create a second competing handoff document.
- Keep this file current after each meaningful repository edit, CI run, artifact inspection, or physical test. The handoff is intended to be the durable memory of the project; the chat transcript is not the source of truth.

### Fresh confirmation of the recorded failed run

Run 43198618602, job 131115903232, was re-fetched from GitHub Actions during this session.

- Job conclusion: failure.
- Completed steps 1–6, including checkout, runner/Xcode selection, pinned build-tool fetch, Lavender profile validation, and dependency installation.
- Step 7, Build XNU 8796, failed.
- Later steps uploaded the diagnostic build tree and an output bundle despite the failure.
- The only fatal compiler diagnostic in the extracted job log is:
  iokit/KernelConfigTables.cpp:93:5: error: expected unqualified-id
- The compiler then reported 1 error generated; the make target failed. The kalloc.h expansion and atomic-header material seen in the same large log is not itself evidence of a second fatal error in this run.
- Patch-application warnings were separately observed for the SWEET-derived patches listed in Section 15. These warnings deserve cleanup, but they are not the recorded fatal C++ diagnostic.

The current Lavender workflow file on ci/lavender-machine-profile selects:
- XNU generation 8796 (macOS version input 13.5);
- DEVELOPMENT, ARM64, LAVENDER;
- macos-15-intel with Xcode 16.0 selected explicitly;
- pinned blacktop/darwin-xnu-build commit 663906528d1cd5ea8fde7de6f16de73db9511549;
- MAKEFLAGS=-k by default, to gather more than one compile diagnostic;
- a unique workflow concurrency group with cancellation disabled.

The packaging step attempted to copy Xcode's private Kernel.framework platform headers from the selected macOS SDK, but that path was absent. Treat this as a separate build-environment warning to investigate after the compile blocker; do not assume it caused the KernelConfigTables.cpp syntax failure.

### Branch inventory discovered during this session

GitHub branch search returned these Lavender-related branches:

- ci/lavender-machine-profile — current known integration/build branch.
- fix/lavender-config-table-placement
- fix/lavender-kernel-config-table
- fix/lavender-kernelconfig-compile
- fix/lavender-kernelconfig-table-placement
- fix/lavender-kernelconfigtables-build
- fix/lavender-remove-invalid-kernel-config-table
- port/lavender-platform-baseline

The 0087-lavender-linker-and-platform-expert.patch file was fetched from the first seven branches (the final branch did not expose that path, so its state is unknown). The inspection produced this useful triage:

| Branch | Observed state of patch 0087 | Interpretation |
|---|---|---|
| ci/lavender-machine-profile | Adds a Lavender personality block to iokit/KernelConfigTables.cpp, matching qcom,sdm660-mtp | This is the insertion associated with the confirmed syntax error. |
| fix/lavender-config-table-placement | Still adds a conditional block to KernelConfigTables.cpp, at a different hunk location, and matches xiaomi,lavender | Not proven fixed; inspect the full hunk and build evidence. |
| fix/lavender-kernel-config-table | Still adds a conditional block to KernelConfigTables.cpp, matching xiaomi,lavender | Not proven fixed; inspect the full hunk and build evidence. |
| fix/lavender-kernelconfig-compile | The fetched 0087 blob has no KernelConfigTables.cpp hunk | Candidate to inspect. Removing the table registration may also leave the platform expert uninstantiated; verify service discovery/registration before treating as correct. |
| fix/lavender-kernelconfigtables-build | Same fetched 0087 blob SHA as the compile candidate; no table hunk | Candidate appears to share the same patch revision; not build-verified here. |
| fix/lavender-remove-invalid-kernel-config-table | Same fetched 0087 blob SHA as the compile candidate; no table hunk | Candidate appears to share the same patch revision; not build-verified here. |
| fix/lavender-kernelconfig-table-placement | Still adds a conditional block to KernelConfigTables.cpp, matching xiaomi,lavender | Not proven fixed; inspect the full hunk and build evidence. |
| port/lavender-platform-baseline | The requested 0087 path returned 404 | This only means that exact path was not found on that branch; it does not establish whether the branch is useful. |

Important: this was a file-content comparison, not a branch build or a complete commit-history comparison. Branch HEAD SHAs, workflow runs, other patch differences, and successful kernel artifacts for these branches were not verified in this consolidation session. Do not cherry-pick or merge one solely because its patch omits the invalid table hunk.

### More precise reference-source crosswalk

The user explicitly requires the project to use HackIOS/HakiOS, IOS-Sweet, and public Lavender/SDM660 documentation as references. Use them in complementary roles:

1. **HackIOS / HakiOS — architecture and porting method**
   - Official current repository: https://github.com/oosexgang/hakios-xnu-kernel
   - Porting guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/porting/index.md
   - Documentation: https://hackios-web.vercel.app/en/
   - Phone boot guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/getting-started/phone.md
   - The project is now branded HakiOS; older references may call it HackIOS. The official main repository is oosexgang/hakios-xnu-kernel.
   - It uses XNU 8796.121.2, its own loader and driver/configuration architecture. Its currently documented phone is POCO X5 Pro 5G (redwood, Snapdragon 778G / SC7280 family), not Lavender / SDM660.
   - Its strongest transferable lessons are: start from the phone's own FDT; create a device-specific configuration; get a minimal boot before enabling drivers; isolate common, SoC and phone-specific code; gate each driver in the kernel configuration; add drivers one at a time; and make RAM boot the default.
   - HakiOS's RAM boot model is a design reference, not a ready-made Lavender loader. Its current device's MMIO, interrupt, firmware, UEFI and GPU details must not be copied to SDM660.

2. **IOS-Sweet — XNU 8796 / iOS 16-era bring-up techniques**
   - Repository: https://github.com/dipa-studio/ios-sweet
   - Engineering notes: https://github.com/dipa-studio/ios-sweet/blob/main/NOTES.md
   - This is a Redmi Note 10 Pro (sweet, SM7150 / Snapdragon 732G) port. It has extensive experience with XNU 8796, IOKit, Apple Device Tree construction, CPU startup, GICv3, timer setup, console output, HID, framebuffer presentation and staged Darwin userspace bring-up.
   - Its README reports an iOS 16.7.16 iPhone X userspace reaching SpringBoard on SWEET as of 2026-10-18. Its NOTES file contains the detailed sequence and diagnostic history.
   - Transfer generic XNU/IOKit patterns and engineering methodology. Do not transfer SWEET's CPU cluster layout, memory bank assumptions, GIC addresses/routing, timer values, UFS, Goodix controller, panel, PMIC, regulators, touch protocol, or boot arguments without independently proving they apply to Lavender.

3. **Public Lavender and SDM660 sources — hardware truth**
   - SDM660-Project Lavender device tree: https://github.com/SDM660-Project/device_xiaomi_lavender
   - Lavender device-tree/source organization: https://github.com/Device-Tree-for-Redmi-Note-7-lavender
   - Lavender-Gang 4.19 kernel source: https://github.com/Lavender-Gang/kernel_xiaomi_lavender
   - Qualcomm SDM660 product page: https://www.qualcomm.com/processors/application-processors/products/sdm660
   - Also inspect the matching public kernel/device-tree branches recorded in Section 7 and, above all, the exact live DTB/DTBO and the uploaded audits.
   - Use source code and DTS bindings to establish addresses, IRQs, clock/reset names, regulators, GPIOs, reserved-memory regions, panel/touch wiring and sequencing. Treat marketing specifications as broad SoC context only.
   - Prefer evidence from the exact Lavender board overlay and current device tree over generic qcom,sdm660-mtp assumptions. The audit's MTP-compatible string is a reported root compatible, not proof every MTP wiring detail matches the retail phone.

### Hardware audit facts worth retaining in every implementation discussion

The two uploaded audit files are:
- lavender-final-audit.txt, captured 2026-11-12 11:23 IST;
- lavender-hardware-report.txt, captured 2026-11-12 10:29 IST.

Their relevant, sanitized facts are already summarized in Section 3. Additional reminders:
- The live Android system is Android 15 / SDK 35 on Linux 4.19.325-NYXION-SUSFS, AArch64, built with Android clang 19 / LLD 19. This is not the XNU build toolchain.
- The observed root compatible list includes qcom,sdm660-mtp, qcom,sdm660, and qcom,mtp; board/PMIC/overlay details must still be resolved.
- The audit's input subsystem names the touchscreen NVTCapacitiveTouchScreen, with 10 slots and 1080×2340 coordinate bounds. Do not substitute SWEET's Goodix GT9896 driver.
- The active boot device is c0c4000.sdhci; public Lavender specifications describe eMMC 5.1. Do not import SWEET's UFS-root assumptions.
- Usable Linux memory is about 2.8 GiB on the 3 GB model, with substantial CMA and reserved regions. This is not an XNU-safe contiguous RAM range. Parse the live FDT and preserve every reservation.
- The Tianma nt36672a FHD+ DSI panel string and the 1080×2340 mode are observations. Display timing, lanes, regulators, reset GPIO and init sequence still need source-backed verification.

### Immediate continuation priority

The next agent should not start by rewriting the machine profile or producing another percentage. Continue from the evidence:

1. Fetch the full patch 0087 and the associated workflow/config files from all candidate fix branches. Fetch branch HEAD commits and compare the actual commits, not just one blob.
2. Inspect the promising branches whose 0087 no longer edits KernelConfigTables.cpp; determine how LavenderPlatformExpert is registered/started without that table entry. Check whether another file or mechanism now provides the personality.
3. Inspect the failed run's xnu-buildtree artifact (ID 13936585997) and recover the exact post-patch iokit/KernelConfigTables.cpp around line 93. Trace the generation mechanism back to the true source input.
4. Make the smallest correct fix on a dedicated branch or the selected existing fix branch. Do not delete the platform expert merely to make compilation pass.
5. Add a syntax/compile check for the generated IOKit table and a hard artifact gate: a failed build or missing kernel must not upload a misleading xnu-kernel artifact as if it were usable.
6. Rebuild DEVELOPMENT / ARM64 / LAVENDER. Continue only from new evidence; distinguish compile, link, binary verification and physical boot as separate gates.
7. Verify the resulting Mach-O architecture/type, load commands, entry point, sections, symbols, relocations and unresolved dependencies; record SHA-256 and artifact ID.
8. Only after static kernel verification, design a temporary RAM-only loader/boot path from Lavender's own bootloader/FDT data. No destructive flash.

### What this consolidation session did and did not do

- Re-read the existing root-level handoff, the Lavender workflow, the profile validator, the SWEET-derived README/NOTES context, the failed run's job state and logs.
- Confirmed that the README links to this handoff on both main and ci/lavender-machine-profile.
- Searched the repository's Lavender-related branches and compared the 0087 patch across the available branches.
- Rechecked the uploaded hardware audits through Files search.
- Rechecked public HakiOS, IOS-Sweet and Lavender/SDM660 reference sources.
- **No source-code fix was committed, no workflow was dispatched, no new kernel artifact was built, and no physical-device boot was performed in this consolidation session.** The purpose of this update is to leave a complete, accurate handoff and a concrete continuation path, not to imply that the kernel is fixed.


---

## 18. Session handoff update — 2026-11-12 22:10 IST

This section records the latest user instruction and the most recent direct repository/CI inspection. It supplements the earlier sections; it does not claim a kernel fix.

### User's latest explicit direction

The user has reiterated that the work must target a **working XNU kernel for SDM660 / Lavender** and explicitly instructed that the engineering references be:

- HackIOS / HakiOS;
- IOS-Sweet;
- public Redmi Note 7 Lavender and Qualcomm SDM660 documentation.

The user then requested that all project knowledge be consolidated into one Markdown file in a location a new ChatGPT session can discover and continue from. This file is that canonical handoff. A new session should read it before asking the user to repeat context or starting a new audit.

### Canonical handoff discoverability — verified

- Canonical file: `PROJECT_HANDOFF.md` at the repository root.
- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Working branch inspected: `ci/lavender-machine-profile`.
- The branch's `README.md` already links to `PROJECT_HANDOFF.md` twice near the top and explicitly says that the remainder of README is inherited SWEET documentation, not Lavender device status.
- Therefore, do not create a second competing handoff. Update this file after every meaningful source inspection, commit, build, artifact, or device test.
- Current inspected handoff blob SHA before this update: `a14517c0bed5003b5d8f0c7459ad197ea2114641`.

### Freshly rechecked CI facts

Run `43198618602`, job `131115903232`, branch `ci/lavender-machine-profile`, head `1842f223f3e3926710b9f53c86b586dfd9b62a99` was rechecked against GitHub Actions.

**Build outcome: FAILED. No kernel was produced.**

The exact compiler fatal diagnostic in the log is:

```text
.../xnu/iokit/KernelConfigTables.cpp
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```

The build was configured as `DEVELOPMENT / ARM64 / LAVENDER`, using the pinned `blacktop/darwin-xnu-build` commit `663906528d1cd5ea8fde7de6f16de73db9511549` and Xcode 16.0 on `macos-15-intel`.

The 0087 patch currently injects this conditional personality block into `iokit/KernelConfigTables.cpp`:

```cpp
#if defined(ARM64_BOARD_CONFIG_LAVENDER)
    "   {"
    "     'IOClass'         = LavenderPlatformExpert;"
    "     'IOProviderClass' = IOPlatformExpertDevice;"
    "     'IONameMatch'     = 'qcom,sdm660-mtp';"
    "     'IOProbeScore'    = 10000:32;"
    "   },"
#endif
```

This insertion is the leading source-level suspect because the fatal error is in the same table and at the injected string-fragment location. It is not yet proven whether the root cause is preprocessor placement, the table's string-literal construction, generation order, or malformed personality grammar. Inspect the actual post-patch/preprocessed source and its generator before editing. Do not merely remove the platform expert and call the build fixed.

### Patch-application warnings from the same run

The build log also reported patch drift / already-present files for these inherited patches:

- `0031-sweet-read-write-root.patch`: referenced `bsd/sweet_hfs/core/hfs_vfsutils.c`, absent.
- `0041-sweet-hfs-link-notes.patch`: referenced `bsd/sweet_hfs/core/hfs_catalog.c`, absent.
- `iobuffermemd_monterey.patch`: did not apply to `iokit/Kernel/IOBufferMemoryDescriptor.cpp`.
- `skywalk_sonoma.patch`: did not apply to `bsd/skywalk/packet/os_packet.h`.
- `syntax_checker_sonoma.patch`: did not apply to `tools/lldbmacros/core/syntax_checker.py`.
- `TrustCache.patch`: target headers already existed under `EXTERNAL_HEADERS/TrustCache/`.

These were warnings from the patch-staging logic and were not the primary fatal compiler error in this run. Review whether each patch is required for Lavender, already upstream, SWEET-only, or stale. Do not indiscriminately force-apply old SWEET patches to XNU 8796.

The log contains many notes and macro-expansion locations involving `kern/kalloc.h` and `atomic_private*.h`. In the extracted log, these are not established as independent fatal errors. Do not promote them to confirmed blockers without seeing a new direct compiler `error:` diagnostic.

### Artifact confirmation — no usable kernel

Run `43198618602` uploaded:

| Artifact | ID | Size | SHA-256 | Meaning |
|---|---:|---:|---|---|
| `xnu-buildtree` | `13936585997` | 22,350,351 bytes | `dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf` | Useful diagnostic build tree; excludes many compiled objects. |
| `xnu-kernel` | `13935079884` | 178,298 bytes | `58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4` | **Not a kernel.** Contains only KDK library inspection metadata. |

The log shows `out/` contained `kdk-libs/` only; the copy step found no `kernel.*` files and printed:

```text
shasum: kernel.*: No such file or directory
```

The build-tree artifact was uploaded successfully and was reported to expire on 2026-11-19 10:45:59 UTC. It may be useful for diagnosis if still retained, but do not rely on it remaining available.

The packaging step also tried to copy Xcode's private Kernel.framework platform headers from the selected SDK and got `No such file or directory` for `MacOSX15.0.sdk/.../PrivateHeaders/platform`. Treat this as a separate packaging/toolchain issue, not the cause of the C++ syntax error unless new evidence says otherwise.

### Current patch-0087 branch leads

A previous cross-branch comparison recorded these Lavender-related branches:

- `ci/lavender-machine-profile` — active known integration/build branch; 0087 currently injects the table block described above.
- `fix/lavender-config-table-placement`
- `fix/lavender-kernel-config-table`
- `fix/lavender-kernelconfig-compile`
- `fix/lavender-kernelconfig-table-placement`
- `fix/lavender-kernelconfigtables-build`
- `fix/lavender-remove-invalid-kernel-config-table`
- `port/lavender-platform-baseline`

Some of the `fix/*` branches have an 0087 blob that no longer edits `KernelConfigTables.cpp`; others still insert a conditional table block. These are **candidate source states only**, not proof of a successful compile. The exact branch HEAD SHAs, complete diffs, and successful build artifacts were not established in this session. Before selecting one, compare its full patch series and determine how the platform expert is registered without the removed table entry.

### Immediate next-session bootstrap

A new session should execute this sequence rather than restarting the conversation from scratch:

1. Read this entire `PROJECT_HANDOFF.md` first. Treat it as the project continuity source.
2. Fetch the current branch tip and verify it has not moved. Do not assume the recorded run SHA is still HEAD.
3. Inspect the full `0087` patch and the exact upstream XNU 8796 `KernelConfigTables.cpp` context. Inspect the build-tree artifact if still available; identify whether the C++ source is generated, patched before/after generation, or concatenated through a macro.
4. Compare the candidate `fix/*` branches by HEAD SHA and complete diff. In particular, determine how `LavenderPlatformExpert` gets instantiated if the table personality is removed.
5. Fix the actual source-of-truth mechanism on a dedicated branch. Add a regression check that compiles or validates the resulting table source.
6. Improve the workflow so it fails the job if the kernel output directory has no real `kernel.*` artifact. Do not upload KDK metadata as an artifact named `xnu-kernel`.
7. Run a clean `DEVELOPMENT / ARM64 / LAVENDER` build. If it fails, extract direct compiler diagnostics, not macro-expansion notes, and iterate on the smallest root cause.
8. After a successful link, inspect the actual Mach-O kernel, architecture, file type, entry/load information, sections, symbols, relocations and unresolved dependencies. Record its SHA-256.
9. Only then prepare a temporary RAM-only boot payload/loader for the physical Lavender. Do not flash or overwrite boot-critical partitions.

### Work completed in this handoff update

- Reconfirmed the canonical root-level handoff and its README discoverability.
- Re-read the latest stored handoff and the Lavender workflow.
- Re-fetched the failed run's job log, job metadata and artifact metadata.
- Reconfirmed the 0087 table insertion and the GIC/config patch contents.
- Consolidated the direct compiler failure, patch-drift warnings and false-positive kernel artifact into this handoff.
- **No source fix, successful build, kernel artifact, or physical boot was produced by this documentation update.** The next session must continue with the source-level fix and build, not mistake this handoff commit for engineering completion.


---

## 19. Canonical handoff synchronization — 2026-11-12 22:12 IST

### Repository entry points

The repository now has two intentionally complementary entry points:

1. `AGENTS.md` — short operational instructions for coding agents. It says to read this handoff first, inspect current branch/CI state, use HakiOS + IOS-Sweet + public Lavender/SDM660 references, avoid cross-device hardware assumptions, preserve the Android recovery path, and update this file after meaningful work.
2. `PROJECT_HANDOFF.md` — the detailed project memory: mission, hardware audit, user constraints, branch history, patch series, CI failures, artifact truth, references, and continuation steps.

The root `README.md` on `main` points readers to both `AGENTS.md` and `PROJECT_HANDOFF.md`. The README on the working branch also points to the handoff. A new session should open the repository and read `AGENTS.md` followed by this complete file before acting.

### Verified documentation commits

- Working branch `ci/lavender-machine-profile`: `68859476d83718c689b752b0aac4d709cd795747`
  - Message: `docs: consolidate Lavender XNU project handoff and latest CI evidence`
- Default branch `main`: `6f3cf9eab8ccb6cb7b74553cdf33840743442fad`
  - Message: `docs: sync canonical Lavender XNU handoff to main`
- After synchronization, the fetched `PROJECT_HANDOFF.md` blob SHA is identical on both branches:
  `86684dc1c03488080ee8d4077bf472b3e4ba3eb2`.

These are documentation-only commits. They do not contain an XNU source fix and do not change the failed kernel-build status. Keep the working branch and default branch histories distinct; this synchronization did not merge the Lavender source branch into main.


---

## 18. Canonical handoff verification and source snapshot — 2026-11-12 22:09 IST

This update was made after the user explicitly requested that all project knowledge be consolidated into one Markdown file that a new chat/agent can discover without relying on the prior conversation.

### One canonical file — no competing summaries

- **Canonical path:** repository root, PROJECT_HANDOFF.md.
- **Working branch:** ci/lavender-machine-profile.
- The working-branch copy fetched immediately before this update was 911 lines / 67,911 characters (before this appended section), blob SHA a14517c0bed5003b5d8f0c7459ad197ea2114641.
- The default-branch copy was older: 832 lines, blob SHA acbfccf85319ac8f833184d818e628c42250d3bf.
- The default-branch README points to both AGENTS.md and PROJECT_HANDOFF.md. The working-branch README points directly to PROJECT_HANDOFF.md. AGENTS.md exists on both branches and explicitly instructs agents to read this handoff first.
- The working-branch AGENTS.md states that the SWEET-derived README/NOTES must not be treated as Lavender hardware facts, requires current branch/run inspection at session start, prohibits unsupported build/boot claims, and requires this handoff to be updated after meaningful milestones.
- This update synchronizes the newer working-branch handoff content to main as well, so a session that opens the repository's default branch does not begin from the older 832-line copy. Keep both copies identical until the project deliberately changes its default-branch strategy.
- Do not create another "master summary", "context", or "handoff" Markdown file. Update this one.

### Exact source-file snapshot fetched from the working branch

The following are Git blob SHAs returned by GitHub when the files were fetched during this session. They identify file contents, not commit SHAs:

| File | Blob SHA |
|---|---|
| .github/workflows/xnu.yml | 8bf0c956cfc61d904fd2a7e400c40a0dd34197c9 |
| tools/validate-lavender-profile.sh | 785439a44ddffd23c6e93be6d4e8e40cf2478698 |
| xnu/patches/0082-lavender-machine-profile.patch | 5e9c62a589da6ed06f9a9e158eaa8317afe6cb0e |
| xnu/patches/0083-lavender-cache-config.patch | ac2d2e0a799276439fb5e24b68102e0b16b33849 |
| xnu/patches/0085-lavender-gic-register-definitions.patch | 203873e015c1d133369d25c702fe5514c4e666e4 |
| xnu/patches/0087-lavender-linker-and-platform-expert.patch | 12643c858539069eaad4dc1a65efb1242f643634 |
| xnu/patches/0088-lavender-config-groups.patch | 8157db427bff9e698de3a837c1e4f6f7907ca5a1 |
| xnu/patches/0089-lavender-exclude-sweet-sampler.patch | 155aec566d2e65a1619d1a65a1d4af426694c71b |
| xnu/patches/0090-lavender-no-apple-monitor.patch | c5ffdb9cd32de30c55a31f1772eb240c6e1ef267 |

Before editing any of these, fetch the latest file and blob SHA again. The table is a snapshot, not a license to overwrite later changes.

### Confirmed references and their boundaries

The reference identities were rechecked through GitHub during this session:

- HakiOS / older HackIOS name: https://github.com/oosexgang/hakios-xnu-kernel. Its README describes XNU 8796.121.2 on Android phones, a separate loader, device-specific drivers/configuration, and RAM-resident boot that returns to Android after reboot. The README's documented primary phone is POCO X5 Pro / redwood / Snapdragon 778G-class hardware, not SDM660. Use its architecture and porting methodology, not its hardware constants.
- IOS-Sweet: https://github.com/dipa-studio/ios-sweet. Its README describes Redmi Note 10 Pro / sweet / SM7150 / Snapdragon 732G and an iOS 16.7.16 userspace reaching SpringBoard. Use its XNU 8796, shim, IOKit, diagnostics, and staged bring-up lessons; never assume its UFS, Goodix, CPU, panel, memory, or PMIC details match Lavender.
- Lavender public device sources and Qualcomm SDM660 product documentation are listed in Section 7. Prefer the exact Lavender device tree and matching kernel source over generic SoC marketing pages or the qcom,sdm660-mtp compatible alone.

### What has and has not changed

- This update consolidates and synchronizes documentation only.
- It does **not** fix the KernelConfigTables.cpp:93:5 compiler error.
- It does **not** make the failed 178 KB xnu-kernel artifact usable; that artifact contains no kernel image.
- It does **not** produce a verified ARM64 Mach-O kernel, dispatch a new build, or boot the physical phone.
- The immediate engineering blocker and next actions remain those in Sections 5, 13, 15, and 17. A new session should continue from the artifact/patch-0087 investigation rather than repeat the hardware audit or restart the project.


---

## 20. Current-session consolidation — 2026-11-12 22:14 IST

This section supersedes older branch-tip and "latest run" snapshots above where they conflict. It records a fresh repository/Actions inspection and additional reference-source checks. It is a documentation update, not a kernel fix.

### 20.1 Live repository pointers

Repository: https://github.com/NAANG360/hackios-lavender-xnu

At inspection time:
- Main ref pointed to commit **425cdc12d0f8514c5df7b76add26f7db9ec1c61f**.
- Integration branch **ci/lavender-machine-profile** pointed to commit **d595ae32a3013ae003b2d5ff5fafd83edabea7cd**.
- The two branches had the same PROJECT_HANDOFF.md blob SHA before this update: **ac593c04b2f325dae1a69a1cf6dc05a7029c80f1**.
- The canonical handoff remains repository-root PROJECT_HANDOFF.md. Both branch READMEs point to it; both AGENTS.md files tell agents to read it first. Do not create a second summary file.
- The current integration branch tip's commit message is "docs: consolidate Lavender XNU project handoff"; it is a documentation-only commit. The main-branch sync is also documentation-only. These refs are not proof of a source-code fix.

### 20.2 Newest CI queue — do not mistake queued for passed

The newest Actions query for ci/lavender-machine-profile returned runs #75–#84, all with status queued and no conclusion at the time of inspection. No logs or build results were available for these queued jobs. The newest was:

- Run #84 / ID **43259106099**
- Job ID **131325274032**
- Head SHA **9a76786a5934f38e0a2a3a1935bfac61f2c00dde**
- Commit: "ci(lavender): validate config table insertion in platform patch"
- URL: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43259106099

Other queued runs returned in the same query:
| Run # | Run ID | Head SHA | Commit title |
|---:|---:|---|---|
| 83 | **43259014570** | **e0b3fca6f3edd79f2f3155ef362cab7b8a27d492** | fix: keep Lavender IOKit personality inside config table |
| 82 | **43258847754** | **ff6e01ae42d65a3a24534d03972de6f128b39264** | remove(lavender): discard invalid config-table relocation patch |
| 81 | **43258837476** | **32f34d36868ce84989cef1185cd1d26c15af7dea** | fix(lavender): match SWEET config-table insertion pattern |
| 80 | **43258728617** | **51aade2edd430decb5aea27775d366a814d00660** | fix(lavender): defer unsafe config table registration patch |
| 79 | **43258717983** | **786c7d77e86baaffdbbf8e742019c7809df18806** | fix(lavender): decouple platform expert from config table build |
| 78 | **43258700891** | **4a95f670403c834414867db21bac791da599cd0b** | fix: correct Lavender config-table patch offsets |
| 77 | **43258653560** | **6e271ad46bebe526a9b7e8be538f6ab7e0eceb8a** | ci: document Lavender configuration string boundary |
| 76 | **43258626882** | **b18e42093b2728b5766808776d6e4975e8bf97ef** | ci: validate Lavender IOKit configuration table placement |
| 75 | **43255605446** | **f8e84b8571a537d6ec983d97c92f18e5d22c626e** | Match Lavender platform expert to audited SDM660 device tree |

**Do not cancel these queued/running attempts just to tidy the Actions page.** Preserve their results and inspect the newest relevant completed build before dispatching another duplicate. The workflow uses unique run-ID concurrency groups with cancel-in-progress false, so commits do not automatically cancel earlier builds. A queued run is not a successful compile.

### 20.3 What run #84 actually changed — important distinction

The run #84 head commit changes tools/validate-lavender-profile.sh: it removes the separate 0091-lavender-config-table-placement.patch from the validator's required-patch list.

However, the 0087-lavender-linker-and-platform-expert.patch blob at that same run head is still **12643c858539069eaad4dc1a65efb1242f643634**, and it still modifies iokit/KernelConfigTables.cpp to inject the conditional Lavender personality. The relevant entries remain:
- IOClass = LavenderPlatformExpert;
- IOProviderClass = IOPlatformExpertDevice;
- IONameMatch = qcom,sdm660-mtp;
- IOProbeScore = 10000:32.

Therefore, run #84's validator change by itself does not demonstrate that the original KernelConfigTables.cpp:93:5 syntax problem is fixed. The same source-level suspect remains in the patch. Wait for the queued build's actual compiler result; if it reaches the same diagnostic, inspect the exact post-patch C++ string construction and fix the authoritative registration/generation mechanism. Do not claim this change solved the error.

The current workflow blob on the integration branch is **8bf0c956cfc61d904fd2a7e400c40a0dd34197c9**. It still selects macos-15-intel and Xcode 16.0, runs the static profile validator, builds XNU 8796 for DEVELOPMENT / ARM64 / LAVENDER, and uploads artifacts with if: always(). It still needs a hard gate that fails if no real kernel image exists before uploading xnu-kernel.

### 20.4 Additional verified reference-source findings

**HakiOS (formerly commonly called hackIOS)**
- Repository: https://github.com/oosexgang/hakios-xnu-kernel
- Porting guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/porting/index.md
- Docs index: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/index.md
- Phone boot guide: https://github.com/oosexgang/hakios-xnu-kernel/blob/main/docs/getting-started/phone.md
- Current README identifies XNU 8796.121.2, its own loader, device/SoC configuration layers, drivers, and RAM-first operation. Its documented primary phone is POCO X5 Pro / redwood / Snapdragon 778G-class (SC7280 family), not Lavender / SDM660.
- The porting guide's transferable sequence is: obtain the phone's own FDT; create a per-device config and SoC layer; start with phone-specific drivers disabled; build the minimal kernel and loader; perform a first boot; then re-enable/port drivers one at a time. The guide explicitly warns that another phone's driver addresses, pins and rails are device-specific.
- Do not copy redwood MMIO, UEFI, firmware, interrupt, panel, GPU or power details into Lavender.

**IOS-Sweet**
- Repository: https://github.com/dipa-studio/ios-sweet
- README: https://github.com/dipa-studio/ios-sweet/blob/main/README.md
- Notes: https://github.com/dipa-studio/ios-sweet/blob/main/NOTES.md
- Workflow: https://github.com/dipa-studio/ios-sweet/blob/main/.github/workflows/xnu.yml
- Sweet is Redmi Note 10 Pro / sweet / SM7150 / Snapdragon 732G, not SDM660. Its README reports iOS 16.7.16 userland reaching SpringBoard on that different phone. Its workflow is a useful XNU 8796 build reference.
- The public Sweet workflow comments that Xcode 16's iig emits an IORPC.kernelContent ABI that this XNU 8796 generation lacks, and historically used a macOS 14 / Xcode 15 runner for compatibility. Lavender's current workflow selects macOS 15 Intel / Xcode 16 because the old runner was retired. This is a potential later toolchain compatibility issue, separate from the currently confirmed KernelConfigTables.cpp:93:5 compiler error. Reassess it from fresh logs after the current queue reaches that stage.
- Transfer XNU build/patch methodology, shim/diagnostic patterns, IOKit and staged bring-up lessons; do not transfer Sweet's UFS, Goodix, CPU topology, MMIO, GIC routing, timer IDs, panel, PMIC or memory layout.

**Public Lavender / SDM660**
- SDM660-Project device tree: https://github.com/SDM660-Project/device_xiaomi_lavender
- Device-tree repository index: https://github.com/Device-Tree-for-Redmi-Note-7-lavender
- Xiaomi SDM660 common platform tree: https://github.com/xiaomi-sdm660/android_device_xiaomi_sdm660-common
- Xiaomi Lavender device tree: https://github.com/xiaomi-sdm660/android_device_xiaomi_lavender
- Lavender-Gang kernel source: https://github.com/Lavender-Gang/kernel_xiaomi_lavender
- Xiaomi kernel source collection: https://github.com/MiCode/Xiaomi_Kernel_OpenSource
- Qualcomm SDM660 product page: https://www.qualcomm.com/processors/application-processors/products/sdm660
- Public Linux panel patch discussion for Novatek NT36672A panels used on Xiaomi SDM636/660 devices, including Lavender variants: https://lists.openwall.net/linux-kernel/2026/09/17/2280
- The panel patch identifies Lavender variants using the Novatek NT36672A controller, including Shenchao and Tianma panel variants. The user's live audit independently identifies the Tianma NT36672A FHD+ panel string. The exact panel variant, timings, lanes, reset sequence and regulators must still be matched to the user's live DTB/DTBO and the correct kernel branch.
- Use exact Lavender DTS and kernel code to resolve the live FDT, clocks, resets, regulators, GPIOs, I²C addresses, IRQs, SDHCI/eMMC details, panel and touch. Do not treat a product page or generic MTP compatible as a complete board description.

### 20.5 Current truthful status and next actions

As of this update:
- Hardware reconnaissance remains complete enough to identify the target and guide source research; the live FDT and complete reserved-memory map still need source-backed extraction/interpretation.
- Lavender machine-profile patches and static validation exist in the repository.
- The last completed diagnostic run with fully inspected logs remains run **43198618602**, which failed at iokit/KernelConfigTables.cpp:93:5.
- The newest run observed is #84 / **43259106099**, still queued. Runs #75–#84 were queued with no conclusion at inspection time.
- No verified ARM64 Mach-O XNU kernel for Lavender has been produced in the evidence inspected here.
- No XNU boot, display, touch, storage or Darwin userspace has been demonstrated on the physical Lavender phone.
- No boot-critical partition has been flashed by this work.

Next session must:
1. Read this handoff and AGENTS.md; inspect current branch HEAD and newest Actions status because the queue may have advanced.
2. Do not cancel the queued runs; capture their results and identify the newest build whose source head contains the intended fix.
3. Inspect run #84 and the later queued runs' actual compile logs. Confirm whether the line-93 error persists.
4. If it persists, fix the source-of-truth IOKit personality registration in 0087; do not only remove a validator check or relocate a second patch.
5. Harden the workflow: fail if the expected kernel.development.* is absent, non-empty check, and Mach-O ARM64 validation before publishing an artifact named xnu-kernel.
6. Rebuild until a real XNU ARM64 Mach-O kernel is produced and statically verified. Record exact source SHA, run ID, artifact ID, SHA-256, Mach-O details and unresolved imports.
7. Only after static verification, plan a temporary RAM-only Lavender boot path. Never flash/erase/repartition boot-critical storage without explicit user approval.

### 20.6 What this session did and did not do

- Verified both repository branch refs and confirmed that the handoff file was synchronized across main and ci/lavender-machine-profile before this update.
- Verified README and AGENTS discoverability on both branches.
- Queried the newest ten Actions runs, inspected run #84's job and commit diff, and re-fetched its 0087 patch.
- Rechecked the HakiOS, IOS-Sweet, Lavender and SDM660 public references listed above.
- Updated this single canonical handoff on both branches. Identical documentation commits: main `4422d6b9e0aee32c0571ff2c77ec2fa809de7e06`; ci/lavender-machine-profile `07cb2d9c83eed9c137969213df2cd8451ea38740`; resulting blob SHA `513ee08ea4a398598326c573d2140d7117e04e6f`.
- **Did not** patch XNU source, dispatch a new workflow, cancel any queued run, produce a kernel, or boot the physical phone.


---

## 20. User-requested all-context handoff refresh — 2026-11-12 22:12 IST

The user explicitly requested that **all project knowledge be placed in one Markdown file that a new ChatGPT session can discover and continue from**. This section records the current-session confirmation and continuation contract.

### Canonical handoff location and discovery path

- The single canonical project memory is this file: `PROJECT_HANDOFF.md` at the repository root.
- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- New-session entry path: open the repository root, read `AGENTS.md`, then read this entire `PROJECT_HANDOFF.md` before making changes.
- The root `README.md` already points to this handoff; `AGENTS.md` explicitly says to read it first. Do not create a second competing project-summary Markdown file.
- During this session, GitHub returned the same handoff blob SHA on `ci/lavender-machine-profile` and `main`: `ac593c04b2f325dae1a69a1cf6dc05a7029c80f1`. The fetched copies were 83,003 characters / 1,120 lines before this addendum.
- Both copies are being updated so a new session that starts from either the default branch or the active work branch finds the same project context. This is documentation synchronization only; it does not merge source branches.

### User's current explicit direction

- Keep pursuing a **real ARM64 XNU kernel for Redmi Note 7 / Lavender / SDM660** until the immediate kernel milestone is achieved.
- Use all three reference groups as requested: **hackIOS/HakiOS**, **IOS-Sweet**, and **public Lavender + Qualcomm SDM660 documentation**.
- Do not substitute emulator success for the real device target. QEMU may be used only for isolated logic tests and must be labelled as such.
- Prioritize a real compiled kernel over userspace, SpringBoard, GPU acceleration, polish, or speculative percentage estimates.
- Make actual repository changes and CI attempts where supported; report exact commits, run IDs, logs and artifacts. Do not claim work happened when no verifiable edit/build happened.
- Preserve the phone's Android/recovery path. No flashing, formatting, repartitioning, or overwriting boot-critical partitions without explicit user authorization. RAM-only/temporary boot is the intended first device test.
- The user has a broken power button. Do not make any test or recovery plan depend on pressing it; do not claim a volume-key sequence works unless verified for the actual bootloader/recovery.

A CI run cannot be kept alive indefinitely: the current workflow itself has a 120-minute timeout and a chat session cannot guarantee background execution after it ends. Interpret the user's instruction as the project-level requirement to keep iterating across available sessions: diagnose, make the smallest justified patch, validate, build, inspect the artifact, and update this handoff. Leave durable repository state so the next session can continue without restarting.

### Current-session repository and branch inventory

The GitHub branch search returned these branches:

- `ci/lavender-machine-profile` — active known integration/build branch.
- `ci/xnu-reference-no-cancel`
- `fix/lavender-config-table-placement`
- `fix/lavender-kernel-config-table`
- `fix/lavender-kernelconfig-compile`
- `fix/lavender-kernelconfig-table-placement`
- `fix/lavender-kernelconfigtables-build`
- `fix/lavender-remove-invalid-kernel-config-table`
- `main`
- `port/lavender-platform-baseline`

The branch-name inventory is verified, but this session did not resolve the current tip SHA of every branch. Do not infer that a branch is a successful fix from its name. Before selecting a candidate, fetch its HEAD/commit and compare the full patch series and CI evidence. The last positively recorded active-branch documentation commit in the previous handoff is `68859476d83718c689b752b0aac4d709cd795747`; the last positively recorded main synchronization commit is `6f3cf9eab8ccb6cb7b74553cdf33840743442fad`. These are historical known commits, not a claim that either remains the current branch tip.

### Fresh CI evidence rechecked in this session

The latest fully inspected failing build still recorded in the project evidence is:

- Workflow run: `43198618602`
- Job: `131115903232` (`build`)
- Run head: `1842f223f3e3926710b9f53c86b586dfd9b62a99`
- Branch: `ci/lavender-machine-profile`
- Target: XNU 8796 / DEVELOPMENT / ARM64 / LAVENDER
- Outcome: **failed; no kernel produced**

The direct fatal compiler diagnostic remains:

```text
.../xnu/iokit/KernelConfigTables.cpp
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```

The leading source-level suspect remains patch `0087-lavender-linker-and-platform-expert.patch`, which injects a conditional Lavender IOKit personality string block into `iokit/KernelConfigTables.cpp`. The exact failure mechanism is not yet proven. Inspect the generated/post-patch source and the table's source-of-truth/generator before editing. Do not remove the platform expert merely to make the compile error disappear.

Other patch-staging warnings observed in the same run:
- `0031-sweet-read-write-root.patch`: target `bsd/sweet_hfs/core/hfs_vfsutils.c` absent.
- `0041-sweet-hfs-link-notes.patch`: target `bsd/sweet_hfs/core/hfs_catalog.c` absent.
- `iobuffermemd_monterey.patch`: failed against `iokit/Kernel/IOBufferMemoryDescriptor.cpp`.
- `skywalk_sonoma.patch`: failed against `bsd/skywalk/packet/os_packet.h`.
- `syntax_checker_sonoma.patch`: failed against `tools/lldbmacros/core/syntax_checker.py`.
- `TrustCache.patch`: TrustCache headers already existed.

These are patch-series drift/skip observations, not the confirmed primary compiler fatal. The log also contains macro-expansion notes involving allocator/atomic headers; do not label those independent fatal errors without a direct compiler error diagnostic.

The workflow's `xnu-kernel` artifact is a false positive:
- Artifact ID `13935079884`, 178,298 bytes, SHA-256 `58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4`.
- The `out/` directory had only KDK library metadata; no `kernel.*` file existed. The log said `shasum: kernel.*: No such file or directory`.
- Do not download, package, boot, or describe this artifact as a kernel.

The diagnostic build tree artifact:
- Artifact ID `13936585997`, 22,350,351 bytes, SHA-256 `dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf`.
- It may contain useful post-patch source/build metadata, but it excludes many object files and was reported with a limited retention period. Check whether it still exists before relying on it.
- Workflow also failed to find Xcode's private `Kernel.framework/.../PrivateHeaders/platform` directory in the selected macOS 15 SDK. Treat that as a separate build-tree/packaging concern, not the cause of the `KernelConfigTables.cpp:93` syntax error absent new evidence.

### Verified target facts to carry forward

Use the sanitized audit findings already recorded in Section 3. Important reminders:
- Actual device identity is `lavender`, Redmi Note 7, Qualcomm SDM660; Android 15 / SDK 35 currently boots.
- Linux reports 8 CPUs / two clusters and approximately 2.8 GiB usable RAM on the 3 GB configuration.
- Device-tree compatible strings include `qcom,sdm660-mtp`, `qcom,sdm660`, and `qcom,mtp`; that does not prove all MTP wiring equals the retail Lavender board.
- Boot storage controller is `c0c4000.sdhci`; treat this as the observed eMMC/SDHCI path, not SWEET's UFS path.
- Panel boot argument identifies `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`; display is 1080×2340.
- Touch input identifies `NVTCapacitiveTouchScreen`; exact controller address, interrupt, reset GPIO and regulator sequence still require the real DT and matching Lavender Linux driver.
- The memory map has multiple RAM banks and reserved carve-outs. It is not safe to map a guessed contiguous range. Derive XNU's map from the live bootloader FDT and preserve modem, DSP, secure, CMA, ramoops and other reserved regions.
- Never copy SWEET's RAM addresses, CPU layout, GIC/timer assumptions, storage, display, touch or PMIC details into Lavender.

### Reference discipline

The canonical references and their intended use are in Section 7:
- HakiOS/hackIOS: https://github.com/oosexgang/hakios-xnu-kernel — use its modern XNU-on-Android architecture, loader/boot approach, per-device separation and RAM-only boot methodology. Its documented devices are not SDM660 Lavender; do not copy their hardware constants.
- IOS-Sweet: https://github.com/dipa-studio/ios-sweet — use XNU 8796 build/patch techniques, shim, ADT generation, IOKit bring-up, framebuffer diagnostics, persistent logging and staged path to iOS 16 userspace. It targets `sweet` / SM7150 / Snapdragon 732G, so its hardware implementation is not Lavender's.
- Public Lavender device tree and matching kernel sources, plus Qualcomm SDM660 documentation, are listed in Section 7. Prefer the exact Lavender DTB/DTBO and matching Linux driver source over generic MTP descriptions and marketing specs.
- The user's separate SDM660/Lucid/Snapdragon-Lab research may be used as supplemental Linux/DT hardware evidence where explicitly available; translate register and sequencing knowledge into XNU/IOKit rather than transplanting Linux driver code or assuming QEMU equals hardware.

### Next-session checklist — start here

1. Read `AGENTS.md`, then this file in full.
2. Re-fetch the actual HEAD SHA of `ci/lavender-machine-profile` and inspect the newest Actions run. Also inspect the candidate `fix/*` branches and their commits; the branch list alone is not evidence of a fix.
3. Download/inspect artifact `13936585997` if still retained. Extract the post-patch `KernelConfigTables.cpp` around line 93 and identify the upstream generator/source file.
4. Compare patch 0087 across the candidate branches. Repair the source-of-truth registration mechanism while keeping `LavenderPlatformExpert` available through a valid XNU mechanism.
5. Audit the patch application loop: classify each skipped SWEET/macOS patch as already upstream, not applicable, or required. Make skip behavior explicit rather than silently accepting malformed relevant patches.
6. Improve artifact collection so the workflow fails if no actual kernel file exists; never upload a KDK metadata directory as `xnu-kernel`.
7. Re-run a clean `DEVELOPMENT / ARM64 / LAVENDER` build. Iterate from direct compiler/linker diagnostics until it produces a real kernel.
8. Verify the linked image as ARM64 Mach-O with file type, load/entry details, symbols, relocations and unresolved-symbol/dependency analysis; record SHA-256.
9. Only after static verification prepare a reversible RAM-only loader/boot payload for the actual phone. Do not flash or repartition.

### Work performed in this addendum

- Reconfirmed the canonical handoff exists at the repository root on both `main` and `ci/lavender-machine-profile`.
- Reconfirmed that README and AGENTS already make the file discoverable to a new session.
- Re-read the working-branch workflow, Lavender validator, current machine-profile/config/GIC/platform-expert patches, branch inventory and failed job log.
- Reconfirmed the fatal table compile error and that the purported kernel artifact contains no kernel.
- This is a documentation/handoff update only. **No source fix, new successful build, verified kernel, or physical-device boot was produced in this session.**


---

## 17. Latest user instruction and session snapshot — 2026-11-12 22:15 IST

### User's explicit continuation requirement

The user has explicitly instructed the engineering agent to keep iterating until a working XNU kernel for the SDM660 target is obtained, and to use **hackIOS/HakiOS, IOS-Sweet, and public Lavender/SDM660 documentation** as the reference set. This is an engineering objective, not permission to claim that an unattended process can run forever.

Operational interpretation:
- Continue with durable diagnose → source-level fix → targeted validation → full CI build → artifact inspection cycles.
- A CI run is bounded by the workflow's `timeout-minutes: 120`; it cannot be made literally infinite. The workflow's run-specific concurrency key and `cancel-in-progress: false` prevent a later run from cancelling an earlier one, but they do not remove the per-job timeout.
- Do not repeatedly rerun unchanged code. Each new run should test a specific source-backed hypothesis or a committed fix.
- If a session ends, leave the repository, CI logs/artifacts, and this file in a state that another session can resume without asking the user to repeat known facts.
- The user's requested end state remains a real, statically verified ARM64 XNU kernel for Lavender/SDM660. Do not substitute a generic arm64 kernel, emulator result, profile preflight, or artifact directory.

### Latest run observed during this session

- Workflow run: `43259106099` (run #84 per repository handoff metadata).
- Job ID: `131325274032`.
- At the time of this inspection, the job status returned by GitHub was **queued**, with no conclusion and no job steps/logs yet.
- The handoff metadata associates this queued attempt with commit `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`. Treat that as the queued run's source commit, not proof of the current branch HEAD.
- No artifacts were listed for this queued run at inspection time.
- The last fully inspected failed run remains `43198618602`, job `131115903232`, source commit `1842f223f3e3926710b9f53c86b586dfd9b62a99`, with the confirmed `KernelConfigTables.cpp:93:5 expected unqualified-id` compile failure and no kernel output.
- Re-check run #84's status, job logs, and artifacts before relying on this snapshot. If it fails, capture its *new* first fatal diagnostic; if it succeeds, verify the artifact itself before calling it a kernel.

### Additional public Lavender source discovered

- [SDM660-Project/device_xiaomi_lavender](https://github.com/SDM660-Project/device_xiaomi_lavender) is another public Lavender device-tree source describing a 4.19-kernel device configuration. Its repository metadata identifies Lavender, SDM660 / Snapdragon 660, Adreno 512, 3/4/6 GB LPDDR4X variants, eMMC 5.1 and 2340×1080 display.
- Use this as an additional device-tree cross-reference, alongside OrangeFox's Lavender tree and the 4.19 kernel source already listed above. It is not proof that its DT files exactly match the user's current Android 15 / 4.19.325 runtime tree. Record the exact branch/commit and compare against the uploaded live audit before importing hardware facts.
- Qualcomm's [SDM660 product specification](https://www.qualcomm.com/processors/application-processors/products/sdm660) describes Kryo 260, two clusters, Adreno 512, LPDDR4/4x, and platform-level support for both eMMC 5.1 and UFS 2.1. That is SoC capability information only. The user's audit specifically reports the Lavender boot storage controller as `c0c4000.sdhci`; do not infer that this phone uses UFS.

### Current honest status

- No successful Lavender XNU kernel has been produced or verified in the evidence inspected here.
- The root-level handoff and agent discovery instructions already exist on both `main` and the Lavender working branch. Keep this file as the single canonical project handoff; do not create a second similarly named status file.
- This session's work is documentation consolidation and state capture. It is not a code fix, build success, Mach-O verification, or physical-device boot.


---

## 21. Live continuity update — 2026-11-12 22:13 IST

This is a factual delta after the previous consolidation. Keep this canonical file as the sole project handoff; do not create a second summary.

### Canonical file / discoverability rechecked

- `PROJECT_HANDOFF.md` exists at the repository root on both `main` and `ci/lavender-machine-profile`.
- Both copies were fetched and were byte-for-byte identical at blob SHA `244021d7f0ddcd6d6e391b2cee96761bcaf11970` immediately before this update.
- The root README already points to `PROJECT_HANDOFF.md`; `AGENTS.md` also instructs new agents to read the handoff first. Therefore a new chat/agent opening the repository has a clear entry point.
- Update this file in place after future work. Do not create a competing `HANDOFF.md`, `CONTEXT.md`, or similarly named summary.

### Latest source/build state rechecked

- The active engineering branch remains `ci/lavender-machine-profile`.
- The newest source-changing build trigger currently visible is commit `9a76786a5934f38e0a2a3a1935bfac61f2c00dde` (`ci(lavender): validate config table insertion in platform patch`).
- Workflow run `43259106099` (#84), job `131325274032`, was still **queued** at the time of this check. No logs or artifacts were available yet. A queued run is not a build result.
- Run #84 is expected to exercise the current Lavender patch series, including `xnu/patches/0091-lavender-config-table-placement.patch`, which is present on the working branch.
- Patch 0091 moves the Lavender IOKit personality block so that the final `IOPanicPlatform` personality and closing configuration-string tokens remain after the conditional block. This is a plausible correction for the previous `KernelConfigTables.cpp:93:5` syntax error, but it is **not verified until the queued run finishes and the actual compiler log is inspected**.
- The current `tools/validate-lavender-profile.sh` no longer asserts that patch 0091 exists or is syntactically valid. The workflow copies `xnu/patches/*.patch` into the pinned build tooling, so the present file should still be staged; nevertheless, restore an explicit validator check for 0091 after the build experiment, so a future missing patch cannot pass preflight silently.
- Patch 0087 itself still contains the original conditional Lavender personality insertion in `iokit/KernelConfigTables.cpp`; 0091 is a later placement/reordering patch layered on top. Confirm both patches apply in lexical order and inspect the resulting post-patch source in the build tree.

### Latest known failure remains the baseline until run #84 completes

- Run `43198618602`, job `131115903232`, failed during XNU 8796 `DEVELOPMENT / ARM64 / LAVENDER` compilation.
- Confirmed fatal diagnostic: `KernelConfigTables.cpp:93:5: error: expected unqualified-id`, at the string literal `"   {"`; `make[7]` failed target `KernelConfigTables.cpo`.
- The 178,298-byte artifact named `xnu-kernel` from that run is **not a kernel**; it contains KDK library metadata only and no `kernel.*` output. The 22,350,351-byte `xnu-buildtree` artifact is the useful diagnostic artifact, if it remains retained.
- No verified ARM64 Mach-O kernel has been produced in the evidence inspected so far. Do not claim otherwise unless run #84 or a later run proves it.

### Current continuation sequence

1. Poll run `43259106099` until it reaches a terminal state; do not start an overlapping full build while this run is queued/running.
2. If it fails, retrieve the full job log and extract every direct compiler/linker `error:` with surrounding lines. Check whether patch 0091 applied or was skipped; inspect the post-patch `KernelConfigTables.cpp` around the reported line.
3. If the table error is gone, continue through all subsequent compile/link failures. Do not stop at the first successful object or at a successful preflight.
4. If the build succeeds, inspect the artifact contents. Require an actual kernel image; verify it is ARM64 Mach-O and record file type, load/entry information, sections, symbols, relocations, unresolved dependencies and SHA-256.
5. Update this handoff with the exact source commit, workflow/run/job IDs, artifact ID/name/size/digest, verification commands/results, and remaining blockers.
6. Only after static kernel verification should the team prepare a reversible RAM-only boot path for the actual Lavender device. No flashing or repartitioning.

### User's current instruction and what this update accomplished

The user explicitly asked for all project knowledge to be consolidated into one Markdown file that a new chat session can discover and continue from. The intended canonical file is this repository-root `PROJECT_HANDOFF.md`, not a separate generated attachment. The user also reiterated that HakiOS/hackIOS, IOS-Sweet and public Lavender/SDM660 documentation must be used as references, with hardware-specific facts transferred only when verified for Lavender.

This update synchronizes the top-level timestamp and records the newly observed patch 0091 / queued run #84 state on both branches. It is a documentation update only: **it does not fix XNU, complete run #84, produce a verified kernel, or boot the phone.**


---

## 22. Canonical project memory — revalidated 2026-11-12 22:15 IST

### One-file rule and fresh-session discovery

This repository-root `PROJECT_HANDOFF.md` is the one canonical memory for the entire Lavender XNU port. It intentionally consolidates the project's goal, device constraints, uploaded hardware-audit findings, source/reference map, XNU patch design, build workflow, exact CI failures, artifacts, boot strategy, user requirements, and the next-session checklist.

A new session must start here:

1. Open https://github.com/NAANG360/hackios-lavender-xnu
2. Read `AGENTS.md`.
3. Read this entire `PROJECT_HANDOFF.md`.
4. Inspect the live tip of `ci/lavender-machine-profile`, all relevant `fix/*` candidates, and the newest GitHub Actions run before changing source.
5. Continue the next action in the latest dated section; do not restart the audit or assume a queued run passed.

The root `README.md` links to this file. Both branches' `AGENTS.md` instructs agents to read it first. Do not create another competing `HANDOFF.md`, `CONTEXT.md`, or project-summary file. Keep this file updated after each meaningful source edit, CI attempt, artifact inspection, or device test.

### Evidence and status reaffirmed in this session

- Re-read the two uploaded Lavender audits. Keep the raw dumps out of Git; use the sanitized hardware facts already captured in Section 3 and the dated addenda.
- Re-fetched the source workflow, Lavender profile validator, machine/config/platform-expert patches, failed workflow job log, job artifacts, README and agent instructions.
- The confirmed failed baseline remains run `43198618602`, job `131115903232`, source head `1842f223f3e3926710b9f53c86b586dfd9b62a99`. Its direct fatal diagnostic is `KernelConfigTables.cpp:93:5: error: expected unqualified-id` at the injected `"   {"` string literal.
- The failed run's `xnu-kernel` artifact `13935079884` is not a kernel: the workflow output contained KDK metadata only and no `kernel.*` image. The `xnu-buildtree` artifact `13936585997` is the useful diagnostic artifact if still retained.
- The newer run `43259106099` (#84), job `131325274032`, and patch `0091-lavender-config-table-placement.patch` are documented in Section 21. Its queued state at the last inspection was not a success result; re-poll it before deciding the next fix.
- The current user instruction is to keep iterating toward a real, statically verified ARM64 XNU kernel for SDM660 Lavender and to use HakiOS/HackIOS, IOS-Sweet, and public Lavender/SDM660 documentation. The project remains a real-device target; emulator results are not acceptance evidence.
- No new source fix, successful kernel build, ARM64 Mach-O verification, or physical-device boot was performed by this documentation consolidation.

### Important continuity guardrails

- Treat every branch SHA, queued run, artifact ID and build result as a timestamped snapshot. Refresh before acting.
- Distinguish observed facts, source-backed facts, hypotheses, and unverified assumptions.
- Never copy SWEET/SM7150 hardware constants into Lavender. Transfer build/boot methods, not another phone's MMIO, IRQ, memory, timer, storage, panel, touch or regulator details.
- A build artifact is not a kernel until its contents are inspected and a real ARM64 Mach-O image is verified.
- Preserve Android/recovery. The first hardware experiment is reversible and RAM-only; do not flash, erase or repartition boot-critical storage without explicit approval.

---

## 22. Live continuity update — 2026-11-12 22:13 IST

This is a factual delta after the previous consolidation. Keep this canonical file as the sole project handoff; do not create a second summary.

### Canonical file / discoverability rechecked

- `PROJECT_HANDOFF.md` exists at the repository root on both `main` and `ci/lavender-machine-profile`.
- Both copies were fetched and were byte-for-byte identical at blob SHA `244021d7f0ddcd6d6e391b2cee96761bcaf11970` immediately before this update.
- The root README already points to `PROJECT_HANDOFF.md`; `AGENTS.md` also instructs new agents to read the handoff first. Therefore a new chat/agent opening the repository has a clear entry point.
- Update this file in place after future work. Do not create a competing `HANDOFF.md`, `CONTEXT.md`, or similarly named summary.

### Latest source/build state rechecked

- The active engineering branch remains `ci/lavender-machine-profile`.
- The newest source-changing build trigger currently visible is commit `9a76786a5934f38e0a2a3a1935bfac61f2c00dde` (`ci(lavender): validate config table insertion in platform patch`).
- Workflow run `43259106099` (#84), job `131325274032`, was still **queued** at the time of this check. No logs or artifacts were available yet. A queued run is not a build result.
- Run #84 is expected to exercise the current Lavender patch series, including `xnu/patches/0091-lavender-config-table-placement.patch`, which is present on the working branch.
- Patch 0091 moves the Lavender IOKit personality block so that the final `IOPanicPlatform` personality and closing configuration-string tokens remain after the conditional block. This is a plausible correction for the previous `KernelConfigTables.cpp:93:5` syntax error, but it is **not verified until the queued run finishes and the actual compiler log is inspected**.
- The current `tools/validate-lavender-profile.sh` no longer asserts that patch 0091 exists or is syntactically valid. The workflow copies `xnu/patches/*.patch` into the pinned build tooling, so the present file should still be staged; nevertheless, restore an explicit validator check for 0091 after the build experiment, so a future missing patch cannot pass preflight silently.
- Patch 0087 itself still contains the original conditional Lavender personality insertion in `iokit/KernelConfigTables.cpp`; 0091 is a later placement/reordering patch layered on top. Confirm both patches apply in lexical order and inspect the resulting post-patch source in the build tree.

### Latest known failure remains the baseline until run #84 completes

- Run `43198618602`, job `131115903232`, failed during XNU 8796 `DEVELOPMENT / ARM64 / LAVENDER` compilation.
- Confirmed fatal diagnostic: `KernelConfigTables.cpp:93:5: error: expected unqualified-id`, at the string literal `"   {"`; `make[7]` failed target `KernelConfigTables.cpo`.
- The 178,298-byte artifact named `xnu-kernel` from that run is **not a kernel**; it contains KDK library metadata only and no `kernel.*` output. The 22,350,351-byte `xnu-buildtree` artifact is the useful diagnostic artifact, if it remains retained.
- No verified ARM64 Mach-O kernel has been produced in the evidence inspected so far. Do not claim otherwise unless run #84 or a later run proves it.

### Current continuation sequence

1. Poll run `43259106099` until it reaches a terminal state; do not start an overlapping full build while this run is queued/running.
2. If it fails, retrieve the full job log and extract every direct compiler/linker `error:` with surrounding lines. Check whether patch 0091 applied or was skipped; inspect the post-patch `KernelConfigTables.cpp` around the reported line.
3. If the table error is gone, continue through all subsequent compile/link failures. Do not stop at the first successful object or at a successful preflight.
4. If the build succeeds, inspect the artifact contents. Require an actual kernel image; verify it is ARM64 Mach-O and record file type, load/entry information, sections, symbols, relocations, unresolved dependencies and SHA-256.
5. Update this handoff with the exact source commit, workflow/run/job IDs, artifact ID/name/size/digest, verification commands/results, and remaining blockers.
6. Only after static kernel verification should the team prepare a reversible RAM-only boot path for the actual Lavender device. No flashing or repartitioning.

### User's current instruction and what this update accomplished

The user explicitly asked for all project knowledge to be consolidated into one Markdown file that a new chat session can discover and continue from. The intended canonical file is this repository-root `PROJECT_HANDOFF.md`, not a separate generated attachment. The user also reiterated that HakiOS/hackIOS, IOS-Sweet and public Lavender/SDM660 documentation must be used as references, with hardware-specific facts transferred only when verified for Lavender.

This update synchronizes the top-level timestamp and records the newly observed patch 0091 / queued run #84 state on both branches. It is a documentation update only: **it does not fix XNU, complete run #84, produce a verified kernel, or boot the phone.**


---


---

## 22. Cross-session master snapshot — 2026-11-12 22:16 IST

This is the latest direct revalidation of the repository entry points and the queued CI attempt. It supplements, rather than replaces, the historical engineering record above.

### 22.1 The one canonical project memory is in place

- Canonical file: **`PROJECT_HANDOFF.md` at the repository root** of https://github.com/NAANG360/hackios-lavender-xnu.
- Do not create a second competing project-summary Markdown file. This document already contains the mission, user constraints, sanitized hardware audit, machine-profile design, XNU patch history, build workflow, CI failures, artifact truth, reference projects, branch inventory, and continuation sequence.
- The current `README.md` on both `main` and `ci/lavender-machine-profile` starts with a Lavender-port notice and links to `AGENTS.md` and/or this handoff. The current `AGENTS.md` on both branches explicitly says **read `PROJECT_HANDOFF.md` first**.
- Verified entry-point file blobs during this inspection:
  - `README.md` / `main`: `871b224d73bf680d9561279b62e012ac9f24b7da`
  - `README.md` / `ci/lavender-machine-profile`: `1ca868709f64a83598e5867bec1587259ea4c618`
  - `AGENTS.md` / `main`: `28f0e100d23185c2b6530e214fdb3937fb61222a`
  - `AGENTS.md` / `ci/lavender-machine-profile`: `73159e440ef275cb13103699e1d253383fcf6cc8`
- Immediately before this update, GitHub returned the same `PROJECT_HANDOFF.md` blob on both branches: `9d91a48367895060fb5ef08ceefa19f53ddb346a`. The two copies had the same 1,537 lines and the same line content. This update is being applied to both branch refs so either entry point remains current.
- New-session discovery path: open the repository root → read `AGENTS.md` → read this entire `PROJECT_HANDOFF.md` → inspect the live branch HEAD and newest Actions run before touching source.

### 22.2 Latest CI attempt rechecked

At 2026-11-12 22:16 IST, workflow run **`43259106099` (#84)** was still **queued**:

- Job ID: `131325274032`
- Run source commit: `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`
- Commit title: `ci(lavender): validate config table insertion in platform patch`
- Job status: `queued`, conclusion: none
- Job steps/logs: not yet available
- Artifacts: none listed

A queued job is not evidence of compilation. Do not dispatch a duplicate full build until this attempt reaches a terminal state and its log/artifacts have been inspected.

**Branch association caution:** the connector did not expose the workflow run's branch field in the job result. Fetching `README.md` and `AGENTS.md` at the exact run source commit `9a76786...` returned the older SWEET-only versions, while the live `main` and `ci/lavender-machine-profile` refs now contain Lavender discovery notices. Therefore do not assume from the commit message alone that run #84 is testing the current integration-branch tip. Resolve the run's actual branch/ref from the Actions run metadata when available.

### 22.3 Exact patch/validator state at run commit 9a76786

The commit diff for `9a76786a5934f38e0a2a3a1935bfac61f2c00dde` changes only `tools/validate-lavender-profile.sh`:

- It removes the `table_patch="xnu/patches/0091-lavender-config-table-placement.patch"` variable.
- It removes the `test -s "$table_patch"` check.
- It removes patch 0091 from the validator's `git apply --numstat` loop.

The commit does **not** modify patch 0087 or patch 0091.

Directly fetched at that commit:

| File | Blob SHA | Observation |
|---|---|---|
| `xnu/patches/0087-lavender-linker-and-platform-expert.patch` | `12643c858539069eaad4dc1a65efb1242f643634` | Still inserts the conditional Lavender personality into `iokit/KernelConfigTables.cpp`; this is the patch associated with the previous line-93 compiler error |
| `xnu/patches/0091-lavender-config-table-placement.patch` | `61a0d542857095eeae4146138fbac9be97d01797` | Exists; moves the `IOPanicPlatform` string entries/closing tokens after the conditional Lavender personality block, apparently keeping the personality inside the shared configuration string |
| `tools/validate-lavender-profile.sh` | `785439a44ddffd23c6e93be6d4e8e40cf2478698` | No longer asserts patch 0091 exists or validates its patch syntax |

The workflow's wildcard staging command copies `xnu/patches/*.patch` into the pinned build-tool patch directory, so patch 0091 is eligible to be staged even though the profile validator no longer checks it. Whether the patch driver actually applies 0091 successfully in run #84 is **not known until the job log is available**.

Engineering interpretation:
- Patch 0091 is a plausible source-level correction to the previous malformed C++ configuration-string placement.
- The validator change in 9a767 is not itself proof that the compiler problem is fixed.
- When the run completes, inspect the patch-application output and the exact post-patch `KernelConfigTables.cpp`; restore an explicit required-patch/syntax check for 0091 once its application behavior is confirmed.
- If the same compiler error remains, fix the source-of-truth patch/generation mechanism rather than deleting the Lavender platform expert or editing a generated build artifact.

### 22.4 Current honest engineering state

- The last fully inspected completed build remains run `43198618602`, job `131115903232`, which failed at `KernelConfigTables.cpp:93:5: expected unqualified-id`.
- The `xnu-kernel` artifact from that failed run is not a kernel: it contains KDK library metadata only, with no `kernel.*` file. Its exact ID/hash and the diagnostic build-tree artifact are recorded above.
- No successful Lavender XNU compile, verified ARM64 Mach-O kernel, RAM-only device boot, display, touch, Darwin userspace, or SpringBoard boot is established by the evidence currently in this handoff.
- This update is documentation and discoverability work only. It does not fix XNU, complete run #84, produce a kernel, or boot the phone.

### 22.5 Next session: execute, do not restart the investigation

1. Read `AGENTS.md` and this file; do not ask the user to repeat the device identity, audit facts, target, or reference projects.
2. Inspect current refs and determine the actual branch/ref of run #84. Poll run `43259106099` / job `131325274032`; retrieve logs and artifacts as soon as available.
3. Compare the run's source commit and branch tip. If run #84 is not the current integration branch, do not treat its result as validation of the integration branch.
4. Confirm whether patches 0087 and 0091 apply in lexical order and inspect the resulting `KernelConfigTables.cpp` around the previous line-93 location.
5. If compilation proceeds past that point, continue from every direct compiler/linker error until the build either produces a kernel or exposes the next concrete blocker.
6. Harden artifact collection so a missing kernel is a hard failure. Require a real ARM64 Mach-O kernel and inspect its file type, architecture, load commands/entry, sections, symbols, relocations, and unresolved dependencies.
7. Record the exact source commit, workflow run/job, logs, artifact ID, SHA-256 and verification. Only then move to a reversible RAM-only boot experiment on the actual Lavender device.

The user's explicit project requirement remains: keep iterating toward a working XNU kernel for the physical SDM660 Lavender target, using HakiOS/hackIOS, IOS-Sweet and public Lavender/SDM660 documentation as complementary references. Do not substitute an emulator result or a generic ARM64 build for that deliverable.


---

## 17. Session update — 2026-11-12 22:18 IST

### Canonical handoff location confirmed

The requested single Markdown handoff already exists at the repository root as `PROJECT_HANDOFF.md`. It is the canonical project record, not a second competing summary. Its discoverability is wired into:
- the repository-root `README.md`, which directs readers to `AGENTS.md` and `PROJECT_HANDOFF.md`;
- `AGENTS.md`, which explicitly instructs every new agent/session to read this handoff first;
- the working branch README, which also points to this file.

At this inspection, the root `PROJECT_HANDOFF.md` blob was the same on `main` and `ci/lavender-machine-profile` (blob SHA before this update: `c4485905a4e5153161ced35db1aaac4c5fd9af7c`). This update is being synchronized to both branches so a new session starting from the default branch can discover the same current context.

**Do not create another handoff file.** Continue to update this one canonical document after meaningful work.

### User's explicit instruction reaffirmed

The user has now explicitly requested that all accumulated project knowledge be consolidated into one Markdown file that a new chat/session can discover and continue from. They also reiterated that the engineering reference set must include:
1. hackIOS / HakiOS;
2. IOS-Sweet;
3. public Lavender / SDM660 documentation and kernel sources.

The reference roles and transfer boundaries are recorded in Sections 7 and 12. In short: HakiOS supplies modern XNU-on-Android port architecture and boot/loader methodology; IOS-Sweet supplies XNU 8796 / iOS 16-era implementation and staged bring-up lessons; Lavender/SDM660 sources supply the actual target hardware facts. Never copy another device's hardware constants into Lavender without device-specific evidence.

The user expects actual repository work, builds, and evidence—not another promise or generic plan. When tools permit, perform the next concrete action, preserve the result in Git, and update this handoff. If a connector/build operation fails, record the exact limitation rather than implying work happened.

### Latest workflow observation

A newer Lavender workflow run was already present:
- Run: `43259106099` (run #84)
- Recorded run head from the previous repository inspection: `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`
- Job observed in GitHub Actions: `131325274032`, job name `build`
- State at the time checked: **queued** (not yet completed; no result or new compiler diagnostics available in this observation).

Re-check this run before acting. Do not infer success or failure from a queued job. If it has completed, fetch its exact logs and artifacts; if it is still queued/running, wait/check later rather than dispatching redundant identical work. The workflow's run-specific concurrency group is designed not to cancel concurrent builds.

### Re-confirmed previous fatal build evidence

The previous failed run `43198618602`, job `131115903232`, was fetched again. Its fatal compiler output is:

```
:93:5: error: expected unqualified-id
   93 |     "   {"
      |     ^
1 error generated.
make[7]: *** [KernelConfigTables.cpo] Error 1
```

The build was compiling `iokit/KernelConfigTables.cpp`. The diagnostic still correlates with the conditional Lavender IOKit personality block introduced by `xnu/patches/0087-lavender-linker-and-platform-expert.patch`. The root cause has not been proven; the next engineer must inspect the exact post-patch/generated source and its generation mechanism before changing it.

The same log contains extensive macro-expansion notes through `kalloc.h`, `atomic_private.h`, and `atomic_private_impl.h`; these are not to be promoted to independent fatal blockers without a primary compiler `error:` diagnostic. Patch skips and the missing Xcode private platform-header copy remain separate hygiene/packaging issues as detailed above.

The failed run's 178,298-byte `xnu-kernel` artifact still contained no `kernel.*` file; it was KDK metadata only. The 22,350,351-byte `xnu-buildtree` artifact is the useful diagnostic bundle. Neither artifact proves a bootable or compiled Lavender kernel.

### Exact next-session instruction

Start by:
1. reading this file and `AGENTS.md`;
2. checking current HEAD for `ci/lavender-machine-profile` and the latest state of run `43259106099`;
3. comparing the candidate fix branches listed in Section 13 before choosing a branch;
4. inspecting the post-patch/generated `KernelConfigTables.cpp` around the failing line and fixing the source-of-truth mechanism;
5. making a small, reviewable commit and running the Lavender workflow;
6. hard-failing the kernel artifact job if the expected ARM64 Mach-O kernel is absent;
7. verifying the actual kernel artifact before reporting success.

**Truthful state remains:** no verified Lavender XNU Mach-O kernel and no physical-device XNU boot have been demonstrated in the evidence recorded here. Do not claim otherwise.


---

## 23. Live handoff refresh — 2026-11-12 22:17 IST

This update responds to the user's explicit request that **all project knowledge be kept in one Markdown file that a new chat can discover and continue from**. This file remains the sole canonical project memory. No second handoff file is to be created.

### 23.1 Exact user goal — preserve this distinction

- Immediate deliverable: a **real, buildable ARM64 XNU 8796 kernel configured for Redmi Note 7 / Lavender / SDM660**.
- Larger target: native Darwin/iOS on the physical Lavender device, with the user's intended **iOS 16.5 userspace** and the real boot chain `launchd → backboardd → SpringBoard`.
- The user does **not** want a themed Android ROM, a mock SpringBoard, or an iOS-looking launcher. A UI mock is not a project milestone.
- Target display is the actual 1080 × 2340 panel and actual NVT multitouch hardware. These are hardware bring-up requirements, not evidence that XNU support exists yet.
- GPU/Adreno 512 acceleration is explicitly later. First achieve a real kernel, then early boot/CPU/memory/interrupts, then display/touch and userspace.
- IOS-Sweet's iPhone X iOS 16.7.16 userland is a reference project's demonstrated payload, **not a silent replacement for the user's iOS 16.5 target**. Record any deliberate userland-version change explicitly.

### 23.2 Boot / safety / interaction constraints

- The user requested an **untethered initial trial** before a later installation. Do not silently equate a host-triggered `fastboot boot` with “untethered”: it is temporary and non-flashing, but it is still initiated from a host. Work out a device-local, reversible trial route if untethered operation is required.
- Preserve the installed Android system and the working recovery path. Do not erase userdata, repartition, flash boot/recovery/vbmeta, or touch modem/persist/calibration partitions during kernel bring-up.
- Permanent installation is only considered after a successful trial and explicit user approval.
- OrangeFox/recovery-terminal handoff was mentioned as a later convenience, after the kernel/boot chain exists; it is not a substitute for compiling XNU.
- The phone's physical power button is broken. Do not give a procedure that depends on pressing it. The user previously asked for volume-up + volume-down recovery/reboot handling; this has not been verified as a universal recovery key combination, so check the actual recovery's documented behavior before relying on it.
- Do not repeatedly ask the user to run commands or collect data already present in the uploaded hardware audits, repository, CI logs, or artifacts.
- The user wants actual source changes and iterative CI/build work, not promises or status-only responses. Keep polling an active run and continue with a new evidence-based fix after a failure. GitHub Actions has finite runner timeouts/queues; do not claim one process can run forever or continue after a session ends without a durable automation.
- QEMU may be used for narrow kernel-development experiments only. It is never acceptance evidence for Lavender and must not replace the real device target.

### 23.3 Live repository pointers at this update

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Default branch: `main`; live head observed: `0873d30ea70ae959b41edc422b4d3a59da7b9c59`.
- Engineering branch: `ci/lavender-machine-profile`; live head observed: `8a27149e9c97b1963fba0d5cdbb1832f1efd789e`.
- Both live branch refs had the exact same `PROJECT_HANDOFF.md` content/blob immediately before this update: `c4485905a4e5153161ced35db1aaac4c5fd9af7c`.
- The two observed branch-tip commits above are documentation synchronization commits; do not mistake them for new kernel fixes.
- Repository metadata identifies this repository as a public fork of `dipa-studio/ios-sweet`. GitHub's repository description still says Redmi Note 10 Pro / `sweet` / SM7150; that metadata is stale for the active Lavender port. The root README begins with a Lavender-port notice, and `AGENTS.md` tells agents to read this handoff first.
- New-chat entry path: repository root → `AGENTS.md` → this entire `PROJECT_HANDOFF.md` → refresh live branch HEAD and newest Actions run → continue from Section 23.4. Do not start by reading the inherited Sweet README as if it describes Lavender.

### 23.4 Fresh CI state — run #84 remains queued

At this inspection, GitHub's run API still reported:

| Field | Value |
|---|---|
| Workflow | `xnu` |
| Run number / ID | #84 / `43259106099` |
| Branch | `ci/lavender-machine-profile` |
| Source commit | `9a76786a5934f38e0a2a3a1935bfac61f2c00dde` |
| Commit title | `ci(lavender): validate config table insertion in platform patch` |
| Job ID | `131325274032` |
| Status | **queued** |
| Conclusion | none |
| Job logs | not available |
| Artifacts | none listed |

A queued run is neither a pass nor a failure. Re-query this run before starting another full build. The run is associated with the engineering branch; its source SHA predates the documentation-only branch tip above.

The source commit `9a76786...` changes only `tools/validate-lavender-profile.sh`: it removes the explicit required-file and `git apply --numstat` validation for `0091-lavender-config-table-placement.patch`. It does not change patch 0087 or patch 0091. The workflow copies `xnu/patches/*.patch` into the pinned build-tool patch directory, so 0091 is eligible for application, but the validator no longer proves that it exists or parses. **Restore explicit validation of 0091 after inspecting the run result.**

Patch 0091, fetched from the current engineering branch at this update (blob `61a0d542857095eeae4146138fbac9be97d01797`), moves the trailing `IOPanicPlatform` personality and closing configuration-string tokens after the conditional Lavender personality. This is a plausible correction for the earlier C++ string-table placement error, but it is not proven until the run log and post-patch source are inspected.

### 23.5 Confirmed failed baseline — exact facts from run #43198618602

- Run: `43198618602`; job: `131115903232`; branch: `ci/lavender-machine-profile`; source head: `1842f223f3e3926710b9f53c86b586dfd9b62a99`.
- The job passed checkout, Xcode selection, pinned build-tool fetch, Lavender profile preflight, and dependency installation. The XNU 8796 build step failed.
- The confirmed fatal diagnostic was `iokit/KernelConfigTables.cpp:93:5: error: expected unqualified-id`, pointing at the inserted C++ string literal `"   {"`. The failure occurred while building `KernelConfigTables.cpo`.
- The most relevant source is `xnu/patches/0087-lavender-linker-and-platform-expert.patch`, which inserts the conditional Lavender IOKit personality into `iokit/KernelConfigTables.cpp`. Inspect the complete post-patch source and the XNU 8796 table-generation path; do not patch a generated output blindly or remove the Lavender platform expert merely to silence the compiler.
- The same run's patch driver skipped/failed several inherited SWEET-era patches:
  - `0031-sweet-read-write-root.patch`: target `bsd/sweet_hfs/core/hfs_vfsutils.c` absent.
  - `0041-sweet-hfs-link-notes.patch`: target `bsd/sweet_hfs/core/hfs_catalog.c` absent.
  - `iobuffermemd_monterey.patch`: failed at `iokit/Kernel/IOBufferMemoryDescriptor.cpp:332`.
  - `skywalk_sonoma.patch`: failed at `bsd/skywalk/packet/os_packet.h:883`.
  - `syntax_checker_sonoma.patch`: failed at `tools/lldbmacros/core/syntax_checker.py:1`.
  - `TrustCache.patch`: files already existed under `EXTERNAL_HEADERS/TrustCache/`.
- These patch warnings are not the confirmed fatal compiler error. Determine whether each intended change is already upstream, obsolete, or needed; do not force old patches onto XNU 8796 without source comparison.
- The log also reports the Xcode SDK path `.../Kernel.framework/.../PrivateHeaders/platform` missing during build-tree packaging. Treat this as a separate packaging/build-environment issue.
- The job log is approximately 48.9 MB. Filter primary compiler/linker diagnostics rather than dumping all macro-expansion notes. In that run, `kalloc.h` and `atomic_private*.h` references were largely macro-expansion notes, not independently confirmed fatal errors.

### 23.6 Artifact truth — the failed run produced no kernel

- `xnu-buildtree`: artifact ID `13936585997`, 22,350,351 bytes, SHA-256 `dc33aa55cd59eca652b656f02e9b46dbf31a744daa86f39fab00e7521544ebcf`; it is the useful diagnostic artifact, subject to its retention window.
- `xnu-kernel`: artifact ID `13935079884`, 178,298 bytes, SHA-256 `58f19909065d9711fe5d213406ab2779537b2cdb18baeb6d51cffd18901009b4`. It contains KDK library metadata only; the output directory had no `kernel.*` file, and the log says `shasum: kernel.*: No such file or directory`.
- Therefore no real ARM64 Mach-O kernel has been established. Do not call the 178 KB artifact a kernel, boot image, or usable output.
- Fix artifact collection so a missing expected kernel fails the workflow. Upload the kernel artifact only after a non-empty expected kernel file exists; then inspect its Mach-O file type, CPU type/subtype, load commands/entry, segments, symbols, relocations and unresolved dependencies, and record SHA-256.

### 23.7 Next-session execution checklist

1. Poll run `43259106099` / job `131325274032`; it was still queued at the last check. Do not infer its result from the commit title.
2. Confirm current branch HEAD again. Compare the run source SHA with the live engineering branch before treating its result as validation of current code.
3. When the run finishes, retrieve the full job log and artifacts. Extract primary compiler/linker errors with surrounding lines; inspect whether patch 0091 applied and inspect the post-patch `KernelConfigTables.cpp` around the previous line-93 location.
4. If the same syntax failure remains, trace how XNU 8796 creates `KernelConfigTables.cpp` and move the Lavender personality to the correct source-of-truth input/registration point. Do not just remove the personality or alter generated files.
5. Re-add validator checks for patch 0091 and any other required Lavender patch. Keep static preflight distinct from compilation.
6. Continue through later compile and link errors until the workflow produces a real kernel. Harden the output step so it fails if the kernel is missing.
7. Verify the resulting ARM64 Mach-O kernel and its dependencies; attach exact commit/run/job/artifact/hash and verification evidence here.
8. Only after static verification, design the user's requested untethered/reversible trial boot. Preserve Android and recovery. No destructive flash or repartition without explicit approval.

### 23.8 What this refresh did and did not do

- Revalidated the root README/AGENTS discovery path, canonical handoff blob on both branches, live branch heads, run #84 metadata, run #431986 job log/artifacts, and patch 0091.
- Added the user's explicit iOS 16.5 userspace goal and no-mock-Android requirement to the handoff.
- This is a **documentation-only** update. It does not fix the compiler error, start or complete run #84, produce a kernel, verify a Mach-O image, or boot the phone.


---

## 24. First successful Lavender XNU kernel build — 2026-11-13

### 24.1 Run #84 is a real compile success

The previously queued run was rechecked and completed:

- Workflow: `xnu`
- Run: **#84 / `43259106099`**
- Job: `131325274032` (`build`)
- Branch/source commit: `ci/lavender-machine-profile` / `9a76786a5934f38e0a2a3a1935bfac61f2c00dde`
- Result: **success**; all build steps completed, including **Build XNU 8796**.
- Build environment: `macos-15-intel`, Xcode 16.0, `DEVELOPMENT_ARM64_LAVENDER`; `MACHINE_CONFIG=LAVENDER`, `KERNEL_CONFIG=DEVELOPMENT`, XNU 8796 (macOS 13.5 source selection).
- Build log reports `🎉 XNU Build Done!` and installs `kernel.development.lavender` for `arm64`.

Artifact `xnu-kernel`: ID `13966815907`, 58,306,556 bytes, ZIP SHA-256 `ca56d0a3b12cd9dd660442d5bf243b3f2de77d0d8842d324baf775b5c731ebee`.

### 24.2 Downloaded kernel static verification

Downloaded and extracted the exact run #84 artifact. The kernel is:

- File: `kernel.development.lavender`
- Size: **16,586,120 bytes**
- File type: **Mach-O 64-bit ARM64 executable** (`MH_MAGIC_64`, `MH_EXECUTE`, CPU type ARM64, subtype ALL)
- Mach-O flags: `NOUNDEFS | PIE`
- Load commands: 26; symbol table: 69,021 entries; undefined symbols: **0**.
- Has `LC_UNIXTHREAD` with `ARM_THREAD_STATE64`; entry PC `0xfffffe000747c488`, inside the executable `__TEXT_EXEC` segment.
- Binary contains the Lavender-specific IOKit personality, `LavenderPlatformExpert`, `qcom,sdm660-mtp`, `Redmi Note 7`, and its driver log strings.
- Extracted binary SHA-256: `58ab0b425eed4b2134a126aa6a04c915bd44a4ee64096f51a20933485f7477b3`.

This closes the earlier “no kernel artifact” blocker. It is a real linked ARM64 XNU kernel, not a KDK-only artifact or a generic arm64 build.

### 24.3 What this does not prove

- No actual Lavender phone boot has been performed or verified.
- This is a raw XNU kernel, **not** a bootable Android `boot.img`, shim image, or complete iOS system.
- The build does not prove that the platform expert matches the live DTB, that the interrupt controller/timer/memory map are correct at runtime, or that display/touch/userspace work.
- Do not flash it or overwrite Android/recovery partitions. Continue with reversible loader/payload preparation and static checks, then a temporary device boot only when the boot chain and target FDT are verified.

### 24.4 CI hardening follow-up

Run #84 succeeded even though its source commit had removed the explicit preflight check for patch 0091. The patch is present and the compiled binary contains the Lavender platform personality. Restore the required-file and `git apply --numstat` checks for `0091-lavender-config-table-placement.patch` so a future build cannot silently omit the fix.

The XNU workflow should also fail a nominally successful build if the expected non-empty `kernel.development.lavender` is absent, and verify its ARM64 Mach-O type and Lavender identity strings before artifact publication. Keep this validation separate from the claim of device bootability.

### 24.5 Next engineering actions

1. Restore 0091 validator coverage and add strict kernel artifact verification to CI.
2. Inspect the existing shim and packaging scripts against the Lavender-specific hardware audit and live device tree; identify which inherited Sweet constants are still present and must be replaced or gated.
3. Build the Lavender ADT and payload using verified Lavender DT data only; do not use Sweet framebuffer, touch, storage, IRQ or memory assumptions.
4. Verify the resulting boot image structure and loader handoff without writing any device partition.
5. Plan the requested reversible, non-destructive physical-device trial. No userdata changes, repartitioning, or permanent flashing.
6. Only after kernel early boot is confirmed should display/NVT touch and the iOS 16.5 userspace chain be attempted. Adreno 512 remains later.

The project has now crossed from “no kernel” to “first verified static ARM64 Lavender XNU kernel artifact.” The device boot milestone remains open.


---

## 25. SDM660 16 KiB translation-granule compatibility gate — 2026-11-13

### 25.1 Newly confirmed core-specific constraint

The Lavender XNU profile and shim use a 16 KiB translation granule. This must not be treated as a generic “ARM64 supports it” assumption:

- Arm's official Cortex-A53 TRM specifies `ID_AA64MMFR0_EL1.TGran16 = 0x0`: 16 KiB translation granules are **not supported** on Cortex-A53.
- Arm's official Cortex-A73 TRM specifies `TGran16 = 0x1`: 16 KiB granules **are supported** on Cortex-A73.
- Snapdragon 660 has two four-core Kryo 260 clusters. Independent architecture reporting identifies the high-performance cluster as Cortex-A73-derived and the efficiency cluster as Cortex-A53-derived. Qualcomm confirms the two-cluster, four-plus-four Kryo 260 topology.

References:
- Arm Cortex-A53 TRM: https://documentation-service.arm.com/static/6040c321ee937942ba301626
- Arm Cortex-A73 TRM: https://documentation-service.arm.com/static/5e7b6c837158f500bd5c03fc
- Qualcomm SDM660: https://www.qualcomm.com/processors/application-processors/products/sdm660
- Cluster/core identification: https://www.anandtech.com/show/11338/qualcomm-announces-snapdragon-660-630-mobile-platforms-better-connectivity-camera-compute-at-14nm

### 25.2 Impact on this port

The XNU build can be perfectly valid ARM64 Mach-O and still be unable to run on the currently executing CPU if ABL enters the shim on a Cortex-A53. The existing loader correctly refuses to jump when the active core reports `TGran16 < 1`. However, it previously logged only the granule field, not which MPIDR/cluster was executing.

The kernel payload command line currently requests `cpus=1`, which is a useful conservative first-boot limit, but it does **not** select a Cortex-A73 boot core. The exact relationship between MPIDR Aff1 cluster IDs and the A73/A53 clusters must be read from the live FDT/CPU audit and verified on-device. Do not assume cluster 0 is the performance cluster.

If the boot CPU is A53 (`TGran16=0`), the current 16 KiB XNU cannot be entered on that core. Possible engineering paths are to start an A73 core through a verified PSCI CPU_ON handoff before entering XNU, or to investigate a genuine XNU 4 KiB-granule port. Neither path is implemented or proven. Do not enable eight-core SMP until the per-core granule support and cluster mapping are known.

### 25.3 Instrumentation change

`shim/loader.c` now prints the active boot MPIDR (Aff2:Aff1:Aff0), raw `ID_AA64MMFR0_EL1`, and decoded `TGran16` before the XNU jump gate. This is diagnostic-only and does not change the jump policy. The Lavender shim compile workflow is triggered by this change.

### 25.4 Next gate

1. Confirm the diagnostic-only shim compile succeeds.
2. On the actual device, collect the boot MPIDR and `ID_AA64MMFR0_EL1.TGran16` without writing any partition.
3. Match Aff1 to the actual A73/A53 cluster using the live device tree and CPU MIDR data.
4. Only permit XNU entry on a core with 16 KiB support; keep `cpus=1` until the SMP strategy is proven.
5. If the active boot core lacks 16 KiB support, stop rather than attempting a blind jump. Investigate the PSCI handoff path or 4 KiB XNU feasibility.

This is a newly identified hardware compatibility gate, not evidence that the current kernel build failed.
