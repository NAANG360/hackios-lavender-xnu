# Handoff chapter router

Read `AGENTS.md`, resolve the live branch HEAD and newest Actions run, then open **only** the chapter routed by the current task. Use the chapters as the active engineering record; do not consult `PROJECT_HANDOFF.md` unless explicitly requested.

| Chapter | Open when |
|---|---|
| [01 — Device and safety](01-device-and-safety.md) | Physical device, memory map, recovery, display or safety question |
| [02 — Architecture and build](02-architecture-and-build.md) | XNU internals, patches, comparative references, CI, cache or packaging |
| [03 — Current debugging](03-current-debugging.md) | Current build failure, latest workflow, marker interpretation or immediate CI operation |
| [04 — Evidence and history](04-evidence-and-history.md) | Historical run comparison, video provenance or regression |
| [05 — Post-logo and Darwin userspace](05-post-logo-userspace.md) | Genuine AppleLogo asset, post-logo kernel, storage/root mount, launchd, daemons, IOMobileFramebuffer, touch or SpringBoard |

Do not read all chapters at session start. Do not read historical material unless it changes the current decision.

## Current state

As of 17 November 2026, Run #331 ([44075155032](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44075155032), commit `748d7d1c75fa0af5e3c178d510167b3314c21c12`) remains the latest accepted XNU build/verification and RAM-only package. Artifact `14325018991` contains `iosboot.img` (16,617,472 bytes; SHA-256 `f2b2bb70d236213f02fb8b3e25780a6b337ea8de26890c24ae11bb85a1fd806b`). The latest handset recording stops at event 87 after GICR mapping; it is not checksum-bound and no reset reason is visible.

Runs #334–#336 cover DS-aware GIC comments/guards, A53/A73 topology metadata, and exact SDHC1/eMMC ADT checks. #334 and #335 are in progress, #336 is queued. No replacement `iosboot.img` is accepted until the latest source run passes the full build, Mach-O verification and separate RAM-only package.

**Current route:** Chapter 03 for event-87/GIC trace and latest CI; Chapter 02 for build/GIC/ADT contracts; Chapter 05 for eMMC root-driver planning; Chapter 01 for device safety. Do not read unrelated historical chapters.
## Resume prompt

```text
Continue from latest operations in this repository. Read AGENTS.md, inspect the live branch HEAD and latest relevant Actions runs, then read only the chapter selected by the current-task router. Do not consult PROJECT_HANDOFF.md unless the user explicitly requests it. Resume the unfinished operation from actual source, CI artifacts/logs and physical evidence. Do not repeat completed investigations, ask me to restate repository context, overlap builds, weaken verification, or make destructive device changes. Update AGENTS.md and the affected chapter when a milestone is reached. Leave PROJECT_HANDOFF.md untouched unless explicitly requested.
```
