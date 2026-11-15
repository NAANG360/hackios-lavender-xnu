# Agent instructions — Lavender XNU port

**Read `PROJECT_HANDOFF.md` first.** It is the canonical engineering record. At every session start, re-resolve live branch HEAD, the newest Actions run, its attempt, jobs, artifacts, and source SHA before acting.

Repository: `NAANG360/hackios-lavender-xnu`  
Branch: `ci/lavender-machine-profile`  
Target: Xiaomi Redmi Note 7 / Redmi Note 7S, codename `lavender`, Qualcomm Snapdragon 660 (SDM660).

## Non-negotiable engineering rules

- The objective is a working Darwin/XNU boot on physical Lavender, ultimately reaching a usable Darwin userspace. A compile, Mach-O, package, Apple-like logo, emulator, or static source assertion is not a complete boot.
- Keep milestones distinct: source committed; patch applied; XNU compiled; source guard passed; ARM64 Mach-O verified; RAM-only image packaged; image structurally verified; physical stage observed; Apple logo observed; Darwin userspace reached.
- Never claim a physical stage without actual device evidence. Separate observed facts, hypotheses and unverified assumptions.
- Read the newest handoff section and then inspect current GitHub Actions. Historical summaries below older headings are not authoritative over live CI.
- Preserve the physical target. Do not transplant MMIO, IRQ, CPU topology, display, regulator, touch or storage assumptions from unrelated devices.
- Do not spend time on emulator work as a substitute for physical bring-up. QEMU may be used only for isolated tests where useful.
- Preserve Android and the recovery path. Never flash or erase boot-critical partitions for this experiment. Prefer temporary RAM-only `fastboot boot`. The user's power button is broken; do not request another handset test until a recovery path independent of that button is actually established.
- Make small, testable changes. Inspect actual build logs and generated source before modifying code. Do not respond to an assertion failure by weakening/removing the assertion; fix the faulty check or the implementation.
- After every meaningful milestone, record exact source SHA, workflow run ID/attempt, job IDs and conclusions, artifact IDs, byte sizes, SHA-256 hashes, and physical observations in `PROJECT_HANDOFF.md`.
- Use the exact output filename **`iosboot.img`** for any user-facing image. Do not append dates, timestamps, build numbers or suffixes to the filename.

## Current correction — 2026-11-14, Asia/Kolkata

- Run #220 (`43611119178`, source `4cb4eec5fd57c6bc678520e8178cf805ef36eb10`) **compiled XNU successfully** but failed the required-patch guard. The actual log shows patch 0107 was skipped because its hunks did not apply after patch 0106 modified the same `start.s` and `sweet_early.c` regions. The build-tree artifact confirms stage 8/9 and the new C marker namespace were absent. Do not weaken or bypass the guard.
- Cache reuse is now evidenced: #220 restored a compatible ~228 MiB cache via restore-prefix and saved a new ~239 MB cache. Exact cache key was not a hit. Measure compile time in subsequent runs before claiming a speedup.
- Patch 0107 was rebased against the generated source after 0106. Latest patch commit: `d6ea407a6a844cc92d277949e5ecec4dff7b569c`. A local dry-run/application and focused source assertions passed against the extracted #220 build tree. This is not yet CI proof.
- Runs triggered by patch commits: #221 `43615933214` uses intermediate commit `df63c971d1eb0e14c6c887e3fd071f175a5b4257`; #222 `43615959600` uses final corrected commit `d6ea407a6a844cc92d277949e5ecec4dff7b569c`. At last inspection #221 was in progress and #222 queued. Inspect both; #222 is canonical. Do not launch another overlapping full build.
- Run #220 artifacts: build tree `14120596087` (22,596,397 bytes; ZIP SHA-256 `bf271ae8013bf7e5258486a664eb7afea735b5986708e0a698bfd806e32ed5d3`) and build log `14119584797` (3,899 bytes; ZIP SHA-256 `1d8d57dc6e11fcc442f2aaf5e61c5e31368905dfcb335f8f5a8649607f1a755e`). Kernel output collection reported 16,587,136 bytes, but verification/upload/package were skipped. No new `iosboot.img`.
- Next gates: confirm patch 0107 applies with no skip warning; source guard passes; ARM64 Mach-O and Lavender identity checks pass; kernel artifact uploads; package job creates a structurally verified RAM-only image; independently verify SHA-256. Output filename must be exactly `iosboot.img`. Do not request physical testing until the broken-power-button recovery route is independently established.

## Latest verified state — 2026-11-14, Asia/Kolkata

- Latest known successful XNU + RAM-only package remains run #203, ID `43594864654`, source commit `e9890318f144df763a3ff655a16ca5b5b5447eb5`. Its image artifact was `14114386476`; its extracted `iosboot.img` was 16,527,360 bytes with SHA-256 `172f6cb8b39241dc9997ecf5da1d1eaf5d18d2edb447711baff407c44136ac5f`. It is the last known packaged physical-test image, not proof of a successful boot.
- Current XNU instrumentation source commit: `6b5c477d820656f4b029b005d63596cb54705c82`. Patch 0107 adds assembly stages 8/9 immediately before the return into `arm_init()`, a namespaced C marker scheme (`0x100` entry, `0x100 | stage` C progress), marker publication before framebuffer console output, and explicit `dc cvac` then `dsb sy` sequence. Patch 0106 is the verbose framebuffer diagnostic implementation. The latest cropped run #203 image is geometrically consistent with five green tiles (stages 3–7), but blur prevents an exact marker readback.
- Run #213, ID `43606468548`, attempt 2, source commit `6b5c477d820656f4b029b005d63596cb54705c82`, **completed with failure in the source guard, not in XNU compilation**. Build job `132487666362` reports `Build XNU 8796` success and collected a 16,587,304-byte `kernel.development.lavender`. The guard then raised `ValueError: substring not found` because it searched the assembly comment `// Set KVA of boot args as first arg` in `arm_init.c`, rather than `start.s`. Kernel verification and package were skipped; no new `iosboot.img` exists from this run. See PROJECT_HANDOFF section 44.32.
- Workflow fix commit: `4cb4eec5fd57c6bc678520e8178cf805ef36eb10`. It moves stage 8/9 assertions to a separately loaded `start.s` source; checks required patch skips for both 0106 and 0107; adds `id: xnu-build`; removes workflow YAML from the cache hash; adds compatible cache restore prefixes; and saves compiled cache when the XNU build succeeded even if a later source guard failed.
- Current candidate run #220: ID `43611119178`, attempt 1, source commit `4cb4eec5fd57c6bc678520e8178cf805ef36eb10`; build job `132496159699`. At last check it was in progress at runner setup. No XNU compile, validator, Mach-O, artifact, or package success has yet been established for #220.
- The run #213 log shows a cold cache miss. Runner reported 4 physical CPUs, 4 logical CPUs and 15,032,385,536 bytes RAM; the build used `XNU_MAKE_JOBS=4`. This is already all reported logical CPUs. Do not blindly raise `-j` beyond memory/CPU capacity. Judge cache improvements only from actual cache restore output and measured build duration.
- Run #213 artifacts retained: build log `14120260161`; build tree `14119559638`. The build-tree package excluded `*.o`, `*.a`, and the kernel, so it cannot be treated as an incremental object cache or bootable artifact.

## Immediate next actions

1. Inspect live branch HEAD and run #220 state. Confirm its source SHA matches the workflow-fix commit or identify newer commits.
2. Check the XNU cache restore logs: exact hit, restore-prefix hit, or cold miss. Record the cache key and duration.
3. Inspect the XNU build log for explicit application of 0106 and 0107; ensure neither is skipped.
4. Confirm `Build XNU 8796` succeeds, then inspect the corrected source guard. It must read C milestones from `arm_init.c` and assembly handoff markers from `start.s`.
5. Require the ARM64 Mach-O verification to pass. Confirm the generated kernel identity and hash.
6. Confirm the cache-save step ran after successful compilation where applicable.
7. Confirm the package job succeeds, extracts the expected kernel/ADT payload, and creates exactly `iosboot.img`. Verify its structure and SHA-256 independently. Present the user a download named exactly `iosboot.img` — no date in the filename.
8. Do not claim the build-time fix worked until a cache hit and reduced elapsed time are evidenced.
9. Do not request a physical test until the broken-power-button recovery route is independently established. Then use RAM-only boot and record the last visible tile/marker, C console and Apple-logo status.

## Required reporting

Be direct and precise. The user expects the image, not another generic progress explanation. Report exactly what passed and failed. Never substitute a partial build tree or unverified kernel for `iosboot.img`, and never claim Darwin userspace without evidence.


## Current verified diagnosis — 2026-11-14: Run #224 produced a stale cached kernel

- The user's two hardware videos show the SDM660 A53 boot core (MPIDR=0, TGran16=0) handing XNU to the A73/Kryo 260 Gold core (MPIDR=0x100, MMFR0=0x101122, TGran16=1). PSCI CPU_ON returned 0; the secondary core was observed at MPIDR 0x100; the shim then handed control to XNU at the displayed physical entry 0xA187C488.
- Both videos finish with the independent A53 guardian displaying A53 LAST MARK=0x00000007. This is not merely the loader's static "awaiting XNU bootstrap marks" line: the guardian is reading the shared checkpoint cell and reports XNU's last published assembly marker. Stage 7 means XNU has executed into common_start, past MMU activation and CPU setup, immediately before boot-args KVA calculation and the return toward arm_init(). No C entry marker or Apple logo was reached.
- Run #224 artifact 14122987795 contains a 16,587,136-byte ARM64 Mach-O kernel (SHA-256 4f5e3da26632f0dd1c2e934e05ab8fd71329c6ed08c1c30959e0c17b91306a32). Disassembly of its actual __TEXT_EXEC,__text shows stage 7 followed by the boot-args address calculation and a direct ret; the compiled stage-8 and stage-9 marker instruction sequences are absent. The repository source after patching does contain those stage-8/9 calls.
- Run #224's xnu-build-log artifact 14123311806 explicitly says "XNU kernel.development.lavender already built". The cache restore accepted a legacy compatible-prefix archive containing dxb/build/xnu.obj/kernel.development.lavender. The pinned darwin-xnu-build/build.sh skips make install whenever that file exists. Therefore 0107 was applied to the fresh source and the source guard passed, but the build script skipped compilation and returned the older cached kernel. The packaging job then packaged that stale binary. This is the confirmed cause of the stage-7-only result; it is not evidence that the Snapdragon A73 lacks the required 16K granule.
- Do not use Run #224's iosboot.img as the next test image. It is structurally packaged but contains this stale pre-0107 kernel.

## CI cache and binary-verification repair — current branch

- Workflow commit b5c16d33e2486023de5b4cbd7a9c68d8e9b13e90 introduced a versioned xnu8796-v2- cache key and invalidation of the cached completed-kernel product on legacy-prefix/non-exact restores, while preserving intermediate objects and support libraries. It also touches patched start.s and sweet_early.c after patch application to force their cached objects to be recompiled.
- The workflow now requires the expected build path when the v2 exact cache is not hit, and parses the Mach-O __TEXT_EXEC,__text section to require the actual ARM64 instruction sequences for both stage 8 and stage 9. Source grep alone is not enough. Cache save is gated on the build-path check and successful kernel binary verification.
- Runs #225 (43622736668, commit b5c16d33...) and #226 (43622763975, commit 67182b69...) failed before XNU build because the injected Python string in the workflow was malformed. No compilation was attempted in either run. The quote was corrected in commit ae26598100060260224837dfe76e4d296c3c451f; the exact-cache log-label match was corrected in commit 560a6fde4b3fe670c247532fffca2a5c818df599.
- At the time of this update, runs #227 (43622824076, commit ae265981...) and #228 (43622868931, commit 560a6fde...) are in progress. #227 uses the corrected Python injection but predates the exact-cache log-label correction; its non-exact restore path should be used. #228 contains both corrections. Do not start a third build. Inspect cache restore outcome, build log, binary-stage verification, and package result before asking for another device test.
- Next acceptance criteria: on non-exact/legacy restore, confirm cached final kernel is removed, cached intermediate objects remain, and make install really executes; confirm the resulting Mach-O contains both stage-8 and stage-9 sequences; confirm the package embeds that exact verified kernel. Only then issue a new iosboot.img with the exact requested filename.


## 2026-11-15 update — Runs #227/#228: build/cache repair passed; Mach-O section locator failed

### Verified run outcomes
- Run #227: ID 43622824076, workflow source ae26598100060260224837dfe76e4d296c3c451f. Run #228: ID 43622868931, workflow source 560a6fde4b3fe670c247532fffca2a5c818df599. Both are completed with conclusion FAILURE.
- In both runs, these steps succeeded: runner/toolchain setup, Lavender profile validation, cache restore, runner-capacity detection, dependency installation, legacy cached-kernel invalidation, Build XNU 8796, cache-aware build-path validation, Lavender source entry-guard validation, and kernel output collection.
- The build logs contain the line Building XNU kernel TARGET_CONFIGS="DEVELOPMENT ARM64 LAVENDER" and do not contain XNU kernel.development.lavender already built. This is positive evidence the legacy completed-kernel cache bypass was addressed and the real XNU build path ran.
- Both runs failed only at Verify Lavender kernel artifact. Exact terminal error: could not locate __TEXT_EXEC,__text for binary stage verification. The log first identifies out/kernel.development.lavender as Mach-O 64-bit executable arm64; the verifier then exits 1 while searching for one hard-coded segment/section name pair.
- The RAM-only package job was skipped. The XNU cache-save step was also skipped because binary verification did not pass. Neither #227 nor #228 produced a new iosboot.img.
- Do NOT report that stages 8/9 are absent from the #227/#228 kernels. The verifier failed before reading a text section or testing the stage instruction patterns. Their presence/absence remains unknown until the Mach-O section locator is corrected and run against the collected kernel.
- Run #224's stale-kernel diagnosis remains valid for that artifact only. #227/#228 appear to have fixed the cache bypass; their failure is a new, separate Mach-O parser assumption.

### Immediate engineering action
1. Inspect the actual #228 kernel output and enumerate every LC_SEGMENT_64 command, segment name, section name, section size and file offset. Do not assume __TEXT_EXEC,__text; the actual kernel layout used by this build is authoritative.
2. Fix the verifier to safely locate executable ARM64 text sections from the Mach-O load commands. Validate command bounds, section bounds, nonzero size, file offsets, and executable segment/section flags. Prefer scanning all executable text sections, rather than hard-coding one segment name. Print the discovered segment/section inventory into the verification log for diagnosis.
3. Decode the marker instructions robustly. First compare the expected ARM64 instruction encodings against disassembly of the actual rebuilt start.s object/final kernel. Do not rely on a guessed byte pattern if compiler/assembler output differs. Verify stage 8 and stage 9 independently and report their exact offsets/instructions.
4. Once binary verification passes, ensure the exact verified kernel file is the one inserted into the RAM-only package. Package job must consume the verified artifact, not rebuild or select another path.
5. Confirm cache-save behavior after a successful verified build, then extract the final image and independently verify payload identity, size and SHA-256. Deliver only as iosboot.img.
6. No further physical test until a newly built image has passed all binary and package checks. Use RAM-only fastboot boot; no partition flashing or erasure.

### Reporting state
- XNU build path: PASSED in #227 and #228.
- Legacy stale-kernel invalidation: PASSED in #227 and #228.
- Source entry-guard validation: PASSED in #227 and #228.
- Mach-O ARM64 header identification: PASSED.
- Mach-O executable-text discovery: FAILED (hard-coded section lookup).
- Compiled stage-8/stage-9 presence: NOT YET DETERMINED for #227/#228.
- RAM-only image packaging: SKIPPED.
- New verified iosboot.img: NOT PRODUCED.


## Current correction — 2026-11-15: Mach-O parser NUL-terminator defect

- Live branch HEAD was verified as `af097580a5f46be2a5f0d4540c874cf9a18e546d` before the fix.
- Inspection of the exact Run #228 job log and `.github/workflows/xnu.yml` found the concrete section-name bug: the Python parser used `split(b"\\0", 1)`, which searches for the two literal bytes backslash and zero rather than the NUL byte in fixed-width Mach-O names. Use `split(b"\\0", 1)`.
- The verifier has been changed to parse all `LC_SEGMENT_64` commands, validate the complete load-command region and each command/section table before reading, inspect all non-empty executable sections, validate file bounds, and emit a segment/section inventory. It no longer assumes the executable text must be named exactly `__TEXT_EXEC,__text`.
- The verifier now reports each stage marker's segment, section, file offset and instruction words. It also captures `otool -l` and `otool -tvV` output for independent inspection.
- A diagnostic artifact named `xnu-kernel-diagnostics` is uploaded after successful compilation even if binary verification fails. It includes the exact compiled kernel and any available parser inventory, load-command dump, disassembly, verification report and SHA-256 file.
- Fix commits: parser correction `2a64ff651e0f98a0a09a9ea3a35b0a50c564c6b5`; diagnostic-artifact addition `235d71e71dc834a6bda7289aa33b764d278a093e`.
- These two workflow commits triggered runs #229 (`43677604405`, parser fix only) and #230 (`43677616958`, parser fix plus diagnostic upload). Both were in progress at the time of this record. Do not start another build while either is active.
- Stage 8/9 instruction patterns remain provisional until compared with the actual compiled kernel's disassembly. Do not claim their presence or absence until the new verifier reports exact offsets and the diagnostic disassembly is inspected.
- No new `iosboot.img` exists yet. Packaging remains gated on binary verification; do not request physical testing.


## 2026-11-15 correction — reconcile existing Mach-O parser fix and follow-on verifier refactor

- The NUL-terminated fixed-width Mach-O name defect was already corrected on this branch by commit `2a64ff651e0f98a0a09a9ea3a35b0a50c564c6b5`; the follow-on diagnostic-artifact commit is `235d71e71dc834a6bda7289aa33b764d278a093e`. Do not describe that root-cause fix as newly introduced by a later session.
- Runs #229 (`43677604405`, source `2a64ff651e0f98a0a09a9ea3a35b0a50c564c6b5`) and #230 (`43677616958`, source `235d71e71dc834a6bda7289aa33b764d278a093e`) were already in progress when the current session inspected the live branch. #230 is the preferred evidence run because it includes compiled-kernel diagnostic artifact retention. Inspect these before launching any further full build.
- Current session added standalone `tools/verify-lavender-kernel.py` in commit `3da171772362b9346adb7e6f7378a8636c585940` and refactored `.github/workflows/xnu.yml` to call it in commit `abff246071db828dab395bfa2d98587bf86b2474`. The workflow also uploads a raw `xnu-kernel-candidate` artifact before verification so a failed verifier cannot discard the compiled binary.
- The standalone parser enumerates `LC_SEGMENT_64` commands, validates load-command/section bounds and segment file/VM ranges, skips zero-fill sections, and searches non-empty executable text candidates. It reports section inventory plus stage-8/9 file offsets, VM addresses, instruction words and their expected ARM64 mnemonics.
- Local synthetic Mach-O checks passed for a valid ARM64 executable containing both marker sequences; a deliberately out-of-file section offset was rejected. This is parser unit evidence only, not evidence about the actual XNU kernel.
- Important: the actual #227/#228 kernels were not retained in their artifacts. Do not claim their stage markers are present or absent. The actual kernel from #229/#230 (or a later candidate artifact) must be inspected, and the stage opcode patterns must be compared with its `otool -tvV` disassembly before accepting them.
- The refactor commit `abff246...` currently preserves the raw candidate kernel before verification. It replaced the prior post-verification diagnostics upload with that raw-only artifact; restore/retain the richer load-command/disassembly diagnostics after the already-running runs complete, without starting overlapping builds.
- Current branch head after these changes: `abff246071db828dab395bfa2d98587bf86b2474`. No new verified `iosboot.img` has been produced. Do not request physical testing.
- Python source must use `split(b"\0", 1)` to split on the actual NUL byte. The literal source form `split(b"\\0", 1)` searches for backslash + zero and is incorrect.
