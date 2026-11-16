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

As of 16 November 2026, Run #297 ([43853989024](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43853989024), commit `fd40a4953708f22e7df492bef699e657dade93ea`) is the latest accepted XNU build/verification and RAM-only package. Artifact `14219764264` contains `iosboot.img` (16,601,088 bytes; SHA-256 `5c71696d01283b354e20fdae111f40c9d5b41570d758977ab5f89222efd45e64`). It includes the genuine 20H392 AppleLogo and 0118 stage-18/19 post-logo checkpoints. The image is not yet checksum-bound to a physical test; no Darwin userspace success is claimed.

**Current route:** Chapter 03 for the exact-image diagnostic marker and next CI/physical evidence; Chapter 05 for post-logo kernel and userspace; Chapter 01 for device-test safety. Read only the routed chapter.
 
## Resume prompt

```text
Continue from latest operations in this repository. Read AGENTS.md and PROJECT_HANDOFF.md, inspect the live branch HEAD and latest relevant Actions runs, then read only the chapter selected by the current-task router. Resume the unfinished operation from actual source, CI artifacts/logs and physical evidence. Do not repeat completed investigations, ask me to restate repository context, overlap builds, weaken verification, or make destructive device changes. Update the handoff and affected chapter when a milestone is reached.
```
