# Lavender XNU — agent entrypoint

**Start here.** Read `PROJECT_HANDOFF.md`, inspect live branch HEAD and latest Actions runs, then open only the chapter selected by its routing table. The user should be able to say: **“Continue from latest operations.”** That means resume autonomously from live repository/CI evidence; do not ask for already documented context.

## Mission
Bring XNU 8796 (iOS 16 generation) up on Xiaomi Redmi Note 7/7S (`lavender`, SDM660, ARM64); later investigate Darwin userspace.

## Mandatory session protocol
1. Read `PROJECT_HANDOFF.md` only.
2. Resolve `ci/lavender-machine-profile` HEAD and newest relevant Actions runs. Do not trust stale run numbers in prose.
3. Follow the single **Current task / chapter route** in the handoff. Read only that chapter. Read another chapter only if the task crosses its domain.
4. Inspect current source, exact CI logs/artifacts and physical evidence before changing anything.
5. Continue the latest unfinished operation; don't restart completed investigations or create overlapping Actions builds.
6. Make the smallest evidence-backed change. Validate locally/source-level before CI. Never weaken verifier or source guards to get green.
7. After each meaningful milestone, update the current pointer, affected chapter, evidence/history and this entrypoint if routing changes.

## Non-negotiable safety and truth
- Preserve Android and recovery. Only temporary RAM boot via `fastboot boot iosboot.img`; never flash, erase, repartition or write boot-critical partitions.
- Device power button is broken. Use the shim's documented recovery/reset path; don't assume volume keys select recovery unless physically verified.
- QEMU is not proof of Lavender hardware compatibility.
- Deliverable filename must be exactly `iosboot.img`.
- Never commit raw dumps, serials, private/calibration data or firmware blobs.
- Keep these gates separate: compile → Mach-O verification → package verification → physical XNU stages → Apple logo → Darwin userspace.
- Never claim physical boot, Apple logo or userspace without corresponding device evidence.

## Device facts
Xiaomi Redmi Note 7/7S, codename `lavender`, Qualcomm SDM660 / Kryo 260, ARM64; observed A53 Silver cluster MPIDRs `0x0–0x3`, A73 Gold cluster `0x100–0x103`. The physical XNU boot path has used Gold MPIDR `0x100` because the A53 reports no 16 KiB granule while the A73 reports support. Verify against current source and handset evidence before changing.

## Current task

The bottom-row patch-0109 screenshots now expose HOOK idx=680 sub=3 rank=3. The reported Run #240 kernel/table/symbol analysis resolves this entry to _lck_mtx_startup_init with argument ___startup_lck_mtx_spec_processor_updown_lock (LOCKS subsystem). This is the concrete source lead; it is not proof that the callback itself hangs because the trace is emitted before the call and has no post-call event. Inspect this exact callback/specification and ARM64 lock-init path in the Run #240 buildtree before proposing any change. Do not guess, skip, reorder or gate the initializer. Preserve uncertainty about screenshot-to-image provenance unless independently artifact-bound. Maintain the power-button-independent recovery/reset safety gate before another handset test.

Read Chapter 03.

## Comparative reference: iOS Topaz

Use [`MakrSas/ios-topaz`](https://github.com/MakrSas/ios-topaz) as a comparative XNU/iOS 16.7.16 bring-up reference for its Kryo 260-class CPU topology, ARMv8.0 constraints, Gold-core handoff, GIC setup and boot-image-v4 shim. It targets Redmi Note 12 4G / SM6225 and is a Sweet fork, **not** a Lavender port or independent corroboration. Its patch analysis and porting limits are documented in Chapter 02 and summarized in PROJECT_HANDOFF.md. Never copy Topaz MMIO/DT/ADT constants or cherry-pick patches into Lavender without validating against SDM660 hardware evidence, exact source/configuration and linked disassembly. Treat its LSE/LDAPR/PAN and 16K-granule findings as comparison leads, not proof of the current mutex stall's cause.

## Chapter router
- `docs/handoff/README.md` — chapter routing and one-line resume protocol.
- `docs/handoff/01-device-and-safety.md` — physical-device safety, hardware facts, memory/display.
- `docs/handoff/02-architecture-and-build.md` — architecture, patches, CI, packaging, **Topaz/Sweet comparative reference audit**.
- `docs/handoff/03-current-debugging.md` — active failure frontier, current run, immediate next operation.
- `docs/handoff/04-evidence-and-history.md` — immutable run history, verified evidence and regressions.

## Project identity
Repository: https://github.com/NAANG360/hackios-lavender-xnu
Branch: `ci/lavender-machine-profile`
