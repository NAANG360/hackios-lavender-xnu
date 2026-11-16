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

As of 16 November 2026, Run #266 is the latest fully accepted old-logo package; Run #261 is the user-attributed physical diagnostic-logo result. Apple's real iPhone X 20H392 AppleLogo has been extracted and patch 0114 integrates it at the same post-VM stage. Run #267 exposed a shell-escaping bug in that preparation script; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` fixes it and Run #268 validates. The exact Build 4 FDT confirms removable SDHC2 at `0x0c084000` and its power IRQ/rails; Chapter 05 details the required V5 power handshake and read-only PIO driver plan. Internal Android eMMC remains off-limits.

## Resume prompt

```text
Continue from latest operations in this repository. Read AGENTS.md and PROJECT_HANDOFF.md, inspect the live branch HEAD and latest relevant Actions runs, then read only the chapter selected by the current-task router. Resume the unfinished operation from actual source, CI artifacts/logs and physical evidence. Do not repeat completed investigations, ask me to restate repository context, overlap builds, weaken verification, or make destructive device changes. Update the handoff and affected chapter when a milestone is reached.
```
