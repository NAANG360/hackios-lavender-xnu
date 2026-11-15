# LAVENDER XNU PORT — COMPACT MASTER HANDOFF

> Read this file first in every new session, then read `AGENTS.md`. This is the sole canonical project handoff. It replaces the duplicated historical log with a compact, current engineering record. Update this file after every meaningful source, CI, artifact or device milestone. Keep observed facts separate from hypotheses.

**Repository:** https://github.com/NAANG360/hackios-lavender-xnu  
**Engineering branch:** `ci/lavender-machine-profile`  
**Target:** Xiaomi Redmi Note 7 / Redmi Note 7S (`lavender`), Qualcomm SDM660 / Snapdragon 660, ARM64  
**Current source/workflow code revision:** `abff246071db828dab395bfa2d98587bf86b2474`  
**Branch HEAD before this documentation compaction:** `53fa7b7db22527aef56f55922a8375f38d9d920e`  
**Latest known Actions run:** #231, ID `43677707091`, attempt 1, source `abff246071db828dab395bfa2d98587bf86b2474`  
**Last known successful RAM-only image:** Run #230 artifact `14147915512`, SHA-256 `aed49f435687ec05d970f8f350ee0e3bd83d2a362d9166e5e505e5298a770f06`.

---

## 1. Mission and acceptance

Port Apple XNU 8796 (iOS 16 generation) to physical Lavender. The user's larger target is an iOS 16.5-era Darwin userspace on the actual phone; the immediate engineering deliverable is a real, correctly built and verified ARM64 Lavender XNU kernel, followed by a reversible RAM-only boot image.

The user wants a real iOS/XNU port, not an iOS-themed Android ROM, launcher, mock SpringBoard, generic ARM64 kernel or emulator demo. The intended userspace chain is Darwin `launchd → backboardd → SpringBoard`. GPU acceleration (Adreno 512) is later, not a prerequisite for first kernel bring-up.

Keep these gates distinct:
1. Source change committed and required patches applied.
2. XNU build succeeds for `DEVELOPMENT ARM64 LAVENDER`.
3. ARM64 Mach-O kernel identity and structure verified.
4. Compiled stage-8 and stage-9 instructions independently verified.
5. RAM-only payload/image packaged and structurally checked; exact filename `iosboot.img`.
6. Physical boot stages observed on Lavender.
7. Apple logo observed.
8. Darwin userspace / SpringBoard reached.

A compile, Mach-O, package, logo drawn by a shim, source grep, or QEMU run does not prove a later gate. No Apple logo or Darwin userspace has yet been demonstrated.

## 2. User constraints and device safety

- Physical target only. QEMU may be used for narrow isolated tests but is never proof of Lavender compatibility or a substitute for device bring-up.
- Preserve installed Android and its working recovery path. No flashing, erasing, repartitioning, or writing boot-critical partitions during bring-up. Prefer temporary RAM-only `fastboot boot iosboot.img`. Permanent installation requires explicit approval.
- User's physical power button is broken. Do not request another phone test until a recovery/reboot route independent of that button is actually established. Volume Up + Volume Down has been discussed as a reset chord, but it does not itself select recovery and its physical behavior remains unverified.
- No commands or data requests to the user when existing repository, audit, CI logs or artifacts already contain the answer.
- User-facing image filename must be exactly `iosboot.img`; no timestamp, date, build number or suffix.
- Keep raw device dumps, serials, calibration data, private data and firmware blobs out of Git.
- Report honestly and precisely. Do not claim a build, image, Apple logo, boot or userspace milestone without its corresponding evidence.

## 3. Hardware facts — sanitized audit

Facts below came from user-supplied Lavender hardware audits (2026-11-12); raw dumps are not committed.

| Item | Observed |
|---|---|
| Device | Xiaomi Redmi Note 7 / Note 7S |
| Codename / SoC | `lavender` / Qualcomm SDM660 |
| CPU | ARMv8-A Kryo 260, 8 logical CPUs, two clusters |
| Boot CPU for current shim/ADT | MPIDR `0x100` (Kryo 260 Gold/A73 cluster) |
| RAM visible to Linux | 2,803,968 kB (~2.8 GiB usable; 3 GB model) |
| Android | Android 15, SDK 35 |
| Running Linux kernel | 4.19.325-NYXION-SUSFS |
| Storage controller | `c0c4000.sdhci`; Lavender documentation points to eMMC 5.1 |
| Device-tree compatibles | `qcom,sdm660-mtp`, `qcom,sdm660`, `qcom,mtp` |
| Display | Tianma FHD+ DSI, `qcom,mdss_dsi_nt36672a_tianma_fhdplus_video`, 1080×2340 |
| Touch | `NVTCapacitiveTouchScreen`; exact GPIO/IRQ/address/protocol must come from device tree/driver |
| GPU | Adreno 512 |

Important cautions:
- Lavender is plain ARM64, not arm64e. Do not assume Apple pointer authentication, PPL or Apple SoC services exist.
- Do not import IOS-Sweet's SM7150/Kryo 470/Redmi Note 10 Pro hardware constants.
- Observed RAM regions include starts near `0x40000000`, `0x88f00000`, `0x95000000`, `0xa0400000`; persistent RAM around `0xa0000000–0xa03fffff`; DSP/CDSP reservation near `0x94a00000`; CMA about 278,528 kB. These are Linux observations, not an approved XNU map. Derive actual usable/reserved memory from the bootloader FDT/DTB; never guess a contiguous span.
- GICv3, architectural timer, SDHCI, SMMU, PMIC, MDSS, I²C/SPI/UART and Qualcomm DSP/modem interrupt paths are observed. Exact Lavender board wiring and overlays remain authoritative.
- Current ADT check for Run #230 used RAM `0xa0400000+0x4a448000`, framebuffer `0x9d400000+0x23ff000`, and eight Lavender CPU MPIDRs. These are build inputs verified by that run, not a complete independently validated XNU runtime map.

## 4. References and source boundaries

- **HakiOS / hackIOS:** modern XNU-on-Android architecture, boot/loader methodology and porting patterns.
- **IOS-Sweet:** XNU 8796 / iOS 16-era implementation and staged bring-up reference.
- **Public Lavender / SDM660 sources:** the only basis for target-specific CPU, FDT, memory, interrupt, panel, touch and storage facts.
- This repository is a fork of `dipa-studio/ios-sweet`. Root README/NOTES and inherited files may still describe Redmi Note 10 Pro / codename `sweet` / SM7150. Treat them as inherited reference material only, not Lavender evidence.
- Transfer engineering methods, not device-specific constants or drivers, unless independently validated for Lavender.

## 5. Current implementation

### XNU source and instrumentation
- XNU target: `DEVELOPMENT ARM64 LAVENDER`, XNU 8796.
- Required Lavender patches include:
  - `0106-lavender-verbose-boot-final.patch`: early framebuffer/boot diagnostics.
  - `0107-lavender-post-assembly-entry-trace.patch`: assembly stages 8/9 before return into `arm_init()`, namespaced C markers, cache-clean publication ordering.
- Stage 8 is after boot-args KVA calculation; stage 9 is immediately before `ret` into `arm_init()`.
- Marker namespace: assembly markers 3–9; C entry `0x100`; C progress `0x100 | stage`; terminal marker uses high-bit namespace. Publish markers before framebuffer text output; use `dc cvac` followed by `dsb sy`.
- The Apple logo is intentionally deferred until after `arm_vm_init` and permanent framebuffer alias setup. Its appearance is not implied by the assembly checkpoints.
- A53 guardian reads shared progress markers and reports the last marker after a timeout; watchdog reset behavior is source-grounded but physical validation remains pending.

### CI/cache/package
- `.github/workflows/xnu.yml` builds XNU, validates required patches/source, collects the kernel candidate, verifies Mach-O and stage instructions, and saves cache only after its gated conditions.
- Cache uses a versioned XNU 8796 v2 key. A non-exact/legacy cache restore must invalidate the completed cached kernel product while preserving reusable intermediate objects; the build log must prove `Building XNU kernel TARGET_CONFIGS="DEVELOPMENT ARM64 LAVENDER"` actually runs. Do not claim a speedup without a measured cache restore and duration. Runner previously reported 4 logical CPUs; `XNU_MAKE_JOBS=4` already used them.
- `.github/workflows/package-lavender.yml` is currently **manual** (`workflow_dispatch`) and requires a successful XNU run ID. It resolves that run's exact source SHA, downloads its kernel artifact, verifies the pinned Lucid Build 4 OTA, extracts the stock boot image/SDM660 DTB, builds the Lavender ADT and LAVENDRX payload, builds the non-flashing shim image, writes `iosboot.img`, then checks image structure and non-overlapping load ranges. Do not assume automatic packaging.
- Package must consume the exact kernel SHA accepted by the verifier. The stock DTB/ADT and payload must match the same source/run manifest.
- The package job's current build manifest describes RAM-only fastboot boot; it does not authorize flashing.

## 6. Verified milestones and artifact truth

### Physical video evidence (Run #203-era image)
- User videos showed A53 boot core MPIDR 0 handing off to A73/Kryo 260 Gold MPIDR `0x100`; PSCI CPU_ON returned 0 and the secondary core was observed.
- Independent A53 guardian displayed `A53 LAST MARK=0x00000007`. This indicates XNU reached assembly stage 7 (past MMU activation/CPU setup, before boot-args KVA calculation/return toward `arm_init`). No C entry marker or Apple logo was observed.
- The old Run #203 image's visible tile strip is consistent with stages 3–7; blurred video does not establish more.
- Do not infer a current image's physical behavior from this earlier test.

### Run #230 — latest fully verified kernel + package
- Run #230: ID `43677616958`, source `235d71e71dc834a6bda7289aa33b764d278a093e`; build job `132701098575`; package job `132704144253`.
- Both jobs succeeded. Required patches 0106/0107 applied. Cache restored a compatible legacy cache (~228 MB), removed stale completed kernel, and really ran the XNU build. Build duration was about 11m38s; v2 cache was saved.
- Kernel artifact `14147131789`, 16,587,304 bytes, SHA-256 `a979c798d2b206b8023ca7a798f325e2424a5a4f0786f64a815c2f30c9eef333`.
- ARM64 Mach-O/MH_EXECUTE and Lavender identity checks passed. Stage 8 matched at file offset `0x47a8c0`; stage 9 at `0x47a94c`. Independent LLVM disassembly confirmed both sequences: `add x0, x20, #1, lsl #12; mov w1, #8/9; str w1, [x0]; dc cvac, x0; dsb sy`.
- RAM-only package artifact `14147915512` contains exact `iosboot.img`, 16,531,456 bytes, SHA-256 `aed49f435687ec05d970f8f350ee0e3bd83d2a362d9166e5e505e5298a770f06`. Shim image has the same hash. Payload SHA-256 `66fa3107124d38d9285530f8036c56acaa411971020c262a1ee6ce1f5160a783`; ADT SHA-256 `eaf3c608195bca3a647aaea85ba7ae0b22073520ddb694e41e14a21072000281`.
- Package structural gate passed: ARM64 shim 155,648 bytes; LAVENDRX payload 15,959,424 bytes; DTB 406,950 bytes; load ranges non-overlapping and DTB page-aligned. Optional `/product` node was omitted because `research/ipsw-x/d22ap.adt` was absent; this did not fail packaging.
- This image has **not** been physically tested. It proves compile + real Mach-O marker verification + structural package only. It does not prove Apple logo, C entry or Darwin boot.
- Diagnostic issue in #230: captured `KERNEL-DISASSEMBLY.txt` only showed empty `__TEXT,__text`; explicitly select `__TEXT_EXEC,__text` when capturing disassembly.

### Run #231 — latest failed run and exact current blocker
- Run #231: ID `43677707091`, attempt 1, source `abff246071db828dab395bfa2d98587bf86b2474`; build job `132701378053`; package job `132703708799` skipped.
- Passed: toolchain/setup, profile validation, cache restore, legacy completed-kernel invalidation, XNU 8796 build, cache-aware build-path gate, source entry guard, build log/tree upload, kernel collection, raw candidate upload, and ARM64 Mach-O header identification.
- Failed only in `Verify Lavender kernel artifact` with:
  ```
  invalid Mach-O: section __CTF,__ctf VM range exceeds segment '__CTF':
  addr=0xfffffe0007c80000 size=0x9837a
  ```
- This is a parser assumption: the candidate has an `__CTF` segment with `vmsize=0`, but a non-empty file-backed `__ctf` section. Segment file range is `fileoff=0xb9c000, filesize=0x9c000`; section file offset `0xb9c000`, size `0x9837a`; segment is non-executable (`initprot=1`). It is non-loaded metadata, not executable kernel text. Do not reject the kernel because this metadata section has no mapped VM extent.
- Candidate artifact `xnu-kernel-candidate`, ID `14147107920`; ZIP size 6,223,783 bytes; ZIP digest `dd28d7ad2a9f592a071e6ec7f9fc34eb3a5fcec4798804e67c89040276a0ad13`. Extracted `kernel.development.lavender`: 16,587,304 bytes; SHA-256 `b2d0abef3e20282d24945083cf530ce82c42bbfaeec2f1fa796d3aa0a8e6c1b3`.
- Independent local inspection of this exact candidate found executable sections `__TEXT_EXEC,__text`, `__TEXT_EXEC,__hib_text`, `__TEXT_EXEC,__commpage_text`, and `__KLD,__text`. Both marker sequences are present in `__TEXT_EXEC,__text`: stage 8 file offset `0x47a8c0`, VM `0xfffffe000747e8c0`; stage 9 file offset `0x47a94c`, VM `0xfffffe000747e94c`. LLVM disassembly confirms:
  - stage 8: `add x0, x20, #1, lsl #12`; `mov w1, #8`; `str w1, [x0]`; `dc cvac, x0`; `dsb sy`.
  - stage 9: same, with `mov w1, #9`.
- This is strong independent evidence that the actual #231 candidate contains stages 8/9. It is **not** an official verifier pass: #231 ended in failure; cache-save was skipped; no `xnu-kernel` accepted artifact or new `iosboot.img` was produced.
- Exact log: `Verify Lavender kernel artifact` failed at 04:50:04Z after `file` identified `Mach-O 64-bit executable arm64`. XNU build and source guard had succeeded. The package job was skipped.

## 7. Immediate work — resume here, do not restart

1. Re-resolve branch HEAD and Actions runs. Branch HEAD was `53fa7b7db22527aef56f55922a8375f38d9d920e` before this docs compaction; the documentation commits will advance it. Workflow code is still `abff246...` unless a newer source commit exists. Do not assume the handoff's recorded SHA remains branch HEAD.
2. Fix `tools/verify-lavender-kernel.py` for Mach-O segments with zero VM size:
   - Preserve strict Mach-O header, load-command, command-size, section-table and file-range bounds checks.
   - For file-backed sections in a segment with `vmsize == 0`, allow non-loaded metadata (as in `__CTF`) only when the segment is non-executable; still require the section's file range to lie within the segment's file range and the file.
   - Do **not** exempt executable sections from VM containment. Every executable candidate must belong to a mapped executable segment / instruction section and fit its segment VM and file bounds.
   - Emit the section inventory, including non-mapped metadata, for diagnostics.
3. Run the corrected verifier locally against the retained #231 candidate ZIP/kernel before triggering CI. Require it to print stage-8 and stage-9 exact segment, section, file offsets, VM addresses, words and decoded instructions. Keep malformed executable-section and file-offset cases fail-closed.
4. Improve diagnostic retention in `.github/workflows/xnu.yml`: retain the exact raw kernel, Mach-O inventory/load commands, full `__TEXT_EXEC,__text` disassembly (explicit section selector), verifier report and SHA-256 even if verification fails. The current `xnu-kernel-candidate` upload preserves the raw candidate but is less informative than the intended full diagnostic bundle.
5. Confirm the cache behavior after verifier passes. #231 had `EXACT_CACHE_HIT=false`, entered the real XNU build path, but skipped cache-save because the verification step failed. Do not start overlapping builds. Once the parser is locally validated, make one corrected commit and run XNU CI.
6. Require CI verification to pass against the real rebuilt kernel. Confirm cache save and accepted kernel artifact. Then manually dispatch `.github/workflows/package-lavender.yml` with that successful XNU run ID.
7. Download/extract the package artifact and independently verify exact filename `iosboot.img`, package structure, payload identity, kernel SHA, DTB/ADT, load ranges and image SHA-256. Deliver only this verified image.
8. Do not request a physical test until a recovery route independent of the broken power button is established. Then use RAM-only boot; never flash or erase a boot-critical partition. Record last guardian marker, framebuffer/console state, C-entry marker and Apple-logo result.

## 8. Known historical failure causes — avoid repeating them

- Earlier C++ configuration-table failure at `KernelConfigTables.cpp:93` was caused by Lavender IOKit personality string insertion/patch placement. Do not regress to that obsolete failure; current builds pass the source guard and compile.
- Run #220: patch 0107 was skipped because it was authored against pre-0106 context. It was rebased; required-patch guard correctly caught the omission.
- Run #224: a legacy cache restore contained a completed stale kernel; pinned build tooling skipped make install. Versioned v2 cache key and invalidation now address this; confirm the build log proves fresh make install.
- Runs #227/#228: verifier hard-coded `__TEXT_EXEC,__text`; then NUL-name parsing used literal backslash-zero instead of a NUL byte. Corrected by commits `2a64ff651e0f98a0a09a9ea3a35b0a50c564c6b5` and `235d71e71dc834a6bda7289aa33b764d278a093e`.
- Run #231: standalone parser then over-applied VM-range validation to the non-executable, file-backed `__CTF` metadata segment. Current next task is this narrow parser correction, not XNU source changes.

## 9. Key repository paths

- `AGENTS.md` — short session rules and current task pointer.
- `PROJECT_HANDOFF.md` — this canonical project record.
- `.github/workflows/xnu.yml` — build, source guard, kernel verification, diagnostics/cache.
- `.github/workflows/package-lavender.yml` — manual RAM-only image packaging.
- `tools/verify-lavender-kernel.py` — Mach-O/identity/compiled stage verifier; current #231 blocker.
- `tools/mkpayload.py`, `tools/mkadt-lavender.py`, `tools/unpack_bootv2.py`, `tools/extract_appended_fdt.py` — payload, ADT, boot-image and stock-DTB tooling.
- `xnu/patches/0106-lavender-verbose-boot-final.patch`
- `xnu/patches/0107-lavender-post-assembly-entry-trace.patch`
- `shim/build.sh` — temporary boot shim.

**Current truth:** XNU has compiled repeatedly. Run #230 produced a verified ARM64 Mach-O kernel and structurally verified RAM-only image. Run #231's candidate independently contains both stage-8/9 instruction sequences, but the official verifier rejects its valid non-loaded CTF metadata section. The immediate job is to fix and locally test that parser, then obtain a clean CI verification and a new `iosboot.img`. Physical Apple-logo and Darwin boot remain unverified.
