# Handoff chapter router

Read `PROJECT_HANDOFF.md` first. Resolve the live branch HEAD and newest Actions run before acting. Then open **only** the chapter routed by its current-task section.

| Chapter | Open when |
|---|---|
| [01 — Device and safety](01-device-and-safety.md) | Physical device, memory map, recovery, display or safety question |
| [02 — Architecture and build](02-architecture-and-build.md) | XNU internals, patches, comparative references, CI, cache or packaging |
| [03 — Current debugging](03-current-debugging.md) | Current build failure, latest workflow, marker interpretation or immediate CI operation |
| [04 — Evidence and history](04-evidence-and-history.md) | Historical run comparison, video provenance or regression |
| [05 — Post-logo and Darwin userspace](05-post-logo-userspace.md) | Genuine AppleLogo asset, post-logo kernel, storage/root mount, launchd, daemons, IOMobileFramebuffer, touch or SpringBoard |

Do not read all chapters at session start. Do not read historical material unless it changes the current decision.

## Current state

As of 16 November 2026, Run #261 is the latest successful packaged candidate and the user-supplied `1000317742.mp4` shows the diagnostic logo screen on physical Lavender. The image's visible mark is the procedural approximation in patch 0094, not Apple's official boot-logo asset. Run #262 compiled but failed the scoped source guard; the corrected validator is being exercised by Run #263. The main post-logo gap is now the Lavender eMMC/SDHCI root-device path: the inherited Sweet UFS root instructions are not compatible with Lavender and the destructive userdata-flash flow is forbidden.

## Resume prompt

```text
Continue from latest operations in this repository. Read AGENTS.md and PROJECT_HANDOFF.md, inspect the live branch HEAD and latest relevant Actions runs, then read only the chapter selected by the current-task router. Resume the unfinished operation from actual source, CI artifacts/logs and physical evidence. Do not repeat completed investigations, ask me to restate repository context, overlap builds, weaken verification, or make destructive device changes. Update the handoff and affected chapter when a milestone is reached.
```
