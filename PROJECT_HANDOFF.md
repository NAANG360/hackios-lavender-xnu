# LAVENDER XNU PORT — MASTER PROJECT HANDOFF

> **Read this file first in every new agent/chat session.** This is the authoritative cross-session handoff for the Redmi Note 7 / Lavender XNU port. Update it after every meaningful inspection, patch, CI run, artifact, or device test. Keep observations, hypotheses, and completed verification clearly separate.
>
> Last consolidated: **2026-11-12 (Asia/Kolkata)**  
> Project repository: https://github.com/NAANG360/hackios-lavender-xnu  
> Active known branch: `ci/lavender-machine-profile`  
> Latest verified CI head in the recorded run: `1842f223f3e3926710b9f53c86b586dfd9b62a99`

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
