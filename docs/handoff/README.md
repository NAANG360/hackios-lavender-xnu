# Handoff chapter router

Read `PROJECT_HANDOFF.md` first. Resolve the live branch HEAD and newest Actions run before acting. Then open **only** the chapter routed by its “Current task / chapter route” section.

| Chapter | Open when |
|---|---|
| [01 — Device and safety](01-device-and-safety.md) | Physical device, memory map, recovery, display or safety question |
| [02 — Architecture and build](02-architecture-and-build.md) | XNU internals, patches, comparative references, CI, cache or packaging |
| [03 — Current debugging](03-current-debugging.md) | Current boot failure, latest test, marker interpretation or immediate next operation |
| [04 — Evidence and history](04-evidence-and-history.md) | Historical run comparison, regression or provenance question |

Do not read all chapters at session start. Do not read historical material unless it changes the current decision.

The 15 November checkpoint-video batch and later close-up hook screenshots are indexed in Chapter 03 and Chapter 04. The close-ups report HOOK idx=680 sub=3 rank=3, resolved against Run #240's kernel/table/symbol data to _lck_mtx_startup_init(___startup_lck_mtx_spec_processor_updown_lock). This identifies the callback entry, not conclusively a hang inside its body; screenshot-to-image provenance remains unverified. Latest XNU build #252 failed on two handwritten LSE CASA instructions in commpage assembly; Chapter 03 records the exact error and ARMv8.0 LL/SC correction.

## Resume prompt

```text
Continue from latest operations in this repository. Read AGENTS.md and PROJECT_HANDOFF.md, inspect the live branch HEAD and latest relevant GitHub Actions runs, then read only the chapter selected by the current-task router. Resume the unfinished operation from actual source, CI artifacts/logs and physical evidence. Do not repeat completed investigations, ask me to restate repository context, overlap builds, weaken verification, or make destructive device changes. Update the handoff and affected chapter when a milestone is reached.
```
