# Lavender XNU — master handoff

**Read this file only at session start.** Do not read every chapter. Use the chapter index and open only the chapter needed.

- Repo: https://github.com/NAANG360/hackios-lavender-xnu
- Active branch: `ci/lavender-machine-profile`
- Target: Xiaomi Redmi Note 7/7S (`lavender`), Qualcomm SDM660, ARM64
- Mission: XNU 8796 / iOS 16 generation on physical Lavender; longer-term Darwin userspace.

## Current state — read first

<badge>Physical bring-up frontier: after cpu_bootstrap</badge>

The #233 image passed CI packaging and was tested on the actual handset. The new recording shows four C-level milestones after the former Stage 9 barrier:

1. `arm_init entered (MMU on)`
2. `PE_init_platform done`
3. `dt_parse_cpu_topology done`
4. `cpu_bootstrap done`

This confirms the old assembly-to-`arm_init()` failure is crossed. The latest observed frontier is after `cpu_bootstrap`; do not claim later initialization, scheduler, userspace or Apple logo yet. The next job is to expose exactly which operation after `cpu_bootstrap` stalls or faults.

## Latest validated build

- Source commit: `f2a4cfd4a65829352736783def33c6a5b61e4a68`
- XNU Run #233: `43695509652` — **success**.
- XNU build and Lavender kernel verification: passed.
- Packaging job “Package RAM-only Lavender fastboot image”: passed.
- Exact stock boot image/DTB extraction, Lavender ADT generation, XNU payload build, non-flashing fastboot image build, structure and payload identity checks: passed.
- Artifact: `lavender-fastboot-boot-test`, artifact ID `14154570502`; contains exact filename `iosboot.img`.
- Shim/ADT Run #47: `43695509648` — success.
- RAM-map audit supplied from Android: System RAM begins at `0xA0400000`; `0xA2000000` is inside System RAM and 32 MiB aligned; upper reserved range begins at `0xEA948000`, matching the chosen aperture boundary. This supports the chosen aperture for this observed boot configuration; do not generalize to other DTBs/firmware.

## Immediate next action

1. Inspect the latest physical recording and identify the exact final diagnostic marker, any displayed exception/register state, and whether the guardian reset is intentional or the target hangs.
2. Trace the code immediately after `cpu_bootstrap()` in this exact XNU build. Instrument the next meaningful C initialization boundaries and exception/fault diagnostics; avoid changing unrelated subsystems.
3. Check current `arm_init`/platform initialization source and the shim's marker/guardian implementation before deciding the next patch. Prefer adding precise markers over speculative broad changes.
4. Validate the instrumentation and payload locally, then run one CI build. Package as `iosboot.img` and repeat RAM-only physical testing.
5. Update this handoff with the newly observed last marker and next single hypothesis after each test.

**Safety:** user confirms recovery is built into the shim. Test with `fastboot boot iosboot.img` only. Never flash, erase, repartition or write boot-critical partitions.

## Chapter index — load selectively

| Chapter | Open when |
|---|---|
| [01 — Device and safety](docs/handoff/01-device-and-safety.md) | Hardware, memory, recovery, physical test safety |
| [02 — Architecture and build](docs/handoff/02-architecture-and-build.md) | XNU markers, patches, workflow/cache, package construction |
| [03 — Current debugging](docs/handoff/03-current-debugging.md) | Current post-`cpu_bootstrap` failure; exact evidence and next steps |
| [04 — Evidence and history](docs/handoff/04-evidence-and-history.md) | Prior verified artifacts and historical failure traps |

## Session protocol

- Start here, then inspect live branch HEAD and latest relevant Actions run.
- For current work, read Chapter 03 only. Open another chapter only when needed.
- Treat current source, Actions logs/artifacts and physical recordings as authoritative over stale prose.
- Keep observed facts separate from hypotheses. Distinguish compile, verified kernel, packaged image, physical boot, Apple logo and Darwin userspace.
