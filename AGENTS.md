# Agent entrypoint — Lavender XNU

1. Read `PROJECT_HANDOFF.md` only.
2. Query live branch HEAD and newest relevant GitHub Actions run before acting.
3. Follow **Immediate next action**. Read only the linked chapter needed; never bulk-read all chapters.
4. Inspect current source, actual logs and artifacts. Validate narrowly before spending a full CI build.
5. Keep changes narrow. Do not weaken guards, invent success, or overlap Actions builds.
6. After each milestone, update this short pointer and the affected chapter.

## Mission
Bring XNU 8796 (iOS 16 generation) up on physical Xiaomi Redmi Note 7/7S (`lavender`, SDM660, ARM64). Long-term goal: Darwin userspace. Physical device evidence, not emulator proof.

## Hard constraints
- Preserve Android/recovery. No flashing, erasing, repartitioning or boot-critical writes. RAM-only `fastboot boot iosboot.img` only; permanent writes require explicit approval.
- Recovery is built into the shim (user-confirmed); do not treat recovery as an outstanding blocker.
- User's power button is broken; use the shim's recovery path.
- QEMU is for isolated tests only, never compatibility proof.
- Exact deliverable filename: `iosboot.img` (no suffix).
- No raw dumps, serials, calibration/private data or firmware blobs in Git.
- Never conflate compile, verified kernel, packaged image, physical XNU progress, Apple logo and Darwin userspace.

## Current pointer
Run #233 (ID `43695509652`) succeeded: XNU 8796 build, kernel verification, Lavender ADT/payload generation, RAM-only fastboot image packaging and package identity checks all passed. Artifact `lavender-fastboot-boot-test` contains `iosboot.img`.

Physical test of that image crossed the previous Stage 9 assembly-to-C boundary. The new recording shows `arm_init entered (MMU on)`, `PE_init_platform done`, `dt_parse_cpu_topology done`, and `cpu_bootstrap done`. The current observed frontier is immediately after `cpu_bootstrap`; later platform initialization is not yet confirmed. Start with [Chapter 03](docs/handoff/03-current-debugging.md).

Latest branch HEAD at this update: `c6964327ef60c0bb3370da2630fab12332fbad8e`. Resolve live HEAD before editing.

## Selective chapters
- [01 Device and safety](docs/handoff/01-device-and-safety.md)
- [02 Architecture and build](docs/handoff/02-architecture-and-build.md)
- [03 Current debugging](docs/handoff/03-current-debugging.md)
- [04 Evidence and history](docs/handoff/04-evidence-and-history.md)
