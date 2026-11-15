# Lavender XNU — master handoff

**Read this file only at session start.** Do not read every chapter. Use the chapter index below and open only the chapter needed for the task.

- Repo: https://github.com/NAANG360/hackios-lavender-xnu
- Active branch: `ci/lavender-machine-profile`
- Target: Xiaomi Redmi Note 7/7S (`lavender`), Qualcomm SDM660, ARM64
- Mission: XNU 8796 / iOS 16 generation on physical Lavender; longer-term iOS 16.5-era Darwin userspace.
- Last branch HEAD observed before this documentation split: `e8c2e2303d765835e95fb79f3f15e711f3f82020`. **Always resolve live HEAD before editing.**

## Current state — read first

**Current blocker:** Run #231's official Mach-O verifier rejects valid non-executable, file-backed `__CTF,__ctf` metadata because its segment has `vmsize=0`. XNU compiled and source guards passed; packaging was skipped.

- Latest run: #231 / `43677707091`, attempt 1, source `abff246071db828dab395bfa2d98587bf86b2474` — failed only at Mach-O verifier.
- Retained #231 candidate artifact: `14147107920`; extracted kernel SHA-256 `b2d0abef3e20282d24945083cf530ce82c42bbfaeec2f1fa796d3aa0a8e6c1b3`.
- Independent inspection confirms stage 8 at file offset `0x47a8c0` / VM `0xfffffe000747e8c0`, and stage 9 at `0x47a94c` / VM `0xfffffe000747e94c`. Both actual instruction sequences were independently disassembled. **This is not an official verifier pass.**
- Last fully verified kernel + structural RAM-only image: Run #230 / `43677616958`. Image artifact `14147915512`, exact filename `iosboot.img`, SHA-256 `aed49f435687ec05d970f8f350ee0e3bd83d2a362d9166e5e505e5298a770f06`. Not physically tested.
- No Apple logo or Darwin userspace has been verified.

## Immediate next action

1. Query live branch HEAD and newest Actions runs.
2. Fix `tools/verify-lavender-kernel.py`: allow file-backed sections in zero-VM-size segments only for non-executable metadata and only when file bounds are valid. Preserve strict Mach-O/load-command/section/file bounds. Executable sections must remain mapped, executable and VM/file-contained.
3. Test locally against retained #231 candidate; print section inventory and stage-8/9 locations + decoded instructions. Add negative tests for malformed executable/file ranges.
4. Ensure `.github/workflows/xnu.yml` retains raw kernel, section/load-command inventory, verifier report, SHA-256 and full `__TEXT_EXEC,__text` disassembly even on failure.
5. Commit one narrow fix; run one XNU workflow. Confirm real build path, verifier pass and cache save.
6. Manually dispatch `.github/workflows/package-lavender.yml` using that successful run ID; independently verify exact `iosboot.img`, kernel identity, payload, DTB/ADT, load ranges and hash.
7. Do not request physical testing until a recovery/reboot path independent of the broken power button is established. Then use RAM-only boot only.

**Do not restart XNU source work or weaken verifier guards. Do not overlap Actions builds.**

## Chapter index — load selectively

| Chapter | Open when |
|---|---|
| [01 — Device and safety](docs/handoff/01-device-and-safety.md) | Hardware, FDT/memory, recovery, physical test safety, source authority |
| [02 — Architecture and build](docs/handoff/02-architecture-and-build.md) | XNU markers, patches, workflow/cache, package construction, key paths |
| [03 — Current debugging](docs/handoff/03-current-debugging.md) | Resuming the current verifier failure; detailed #231 facts and exact next steps |
| [04 — Evidence and history](docs/handoff/04-evidence-and-history.md) | Prior verified artifacts, physical observations and old failure traps |

## Session protocol

- Start: read this file, then inspect live branch HEAD and latest relevant Actions run.
- For the active task: read Chapter 03 only; open another chapter only if needed.
- Before changing workflow/package: read Chapter 02. Before device testing: read Chapter 01. For historical comparisons: read Chapter 04.
- Never load all chapters "for context" by default.
- Treat GitHub Actions logs/artifacts and current source as authoritative over stale prose. Update only the relevant chapter and this short current-state section after a milestone.
- Keep observed facts separate from hypotheses. Distinguish compile, verified kernel, packaged image, physical boot, Apple logo and Darwin userspace.
