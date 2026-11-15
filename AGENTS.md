# Lavender XNU — agent instructions

**Start here:** read `PROJECT_HANDOFF.md` (especially **Current state / next action**), then refresh the live branch HEAD and newest GitHub Actions run before changing code. The handoff is the single source of project truth; do not reconstruct context from chat or old sections.

## Target and objective
- Physical target: Xiaomi Redmi Note 7 / Redmi Note 7S, codename `lavender`, Qualcomm Snapdragon 660 / SDM660, ARM64.
- Port XNU 8796 (iOS 16 generation) toward the user's iOS 16.5 / Darwin goal. Immediate goal is a correctly built, verified Lavender kernel and a reversible boot image; ultimate goal is real Darwin userspace on the phone.
- Use HakiOS/hackIOS, IOS-Sweet, and public Lavender/SDM660 sources as references. Transfer architecture/methods, never another device's MMIO, IRQ, CPU, display, regulator, touch or storage assumptions.
- README/NOTES inherit Sweet / Redmi Note 10 Pro / SM7150 facts. They are not Lavender hardware evidence.

## Hard rules
- No emulator work as a substitute for physical bring-up. QEMU is acceptable only for isolated tests.
- Keep compile, Mach-O verification, package verification, physical boot, Apple logo, and Darwin userspace as separate milestones. Never claim a later stage from an earlier one.
- Inspect real logs, artifacts, generated source and compiled instructions. Do not weaken or remove a failing guard to make CI green.
- Make small source changes; test locally against retained artifacts before paying for another 10–12 minute XNU build.
- Preserve Android and recovery. No flashing/erasing boot-critical partitions or repartitioning. Prefer RAM-only `fastboot boot`; permanent writes require explicit approval.
- The user's power button is broken. Do not request another handset test until a recovery route independent of that button is established.
- User-facing image filename must be exactly `iosboot.img` (no dates or suffixes).
- Keep raw device dumps, serials, calibration data, firmware blobs and personal data out of Git.
- After meaningful work, update the handoff with exact commit, run/attempt, jobs, artifacts, sizes, hashes, diagnostics and what was/wasn't physically observed.

## Current task (2026-11-15)
- Branch: `ci/lavender-machine-profile`; live HEAD at last check: `53fa7b7db22527aef56f55922a8375f38d9d920e`.
- Latest run: #231, ID `43677707091`, source `abff246071db828dab395bfa2d98587bf86b2474`, build job `132701378053`. XNU compile, source guard and raw candidate upload passed; official Mach-O verifier failed on `__CTF,__ctf`: the CTF segment has `vmsize=0` but a file-backed section. Package job was skipped; cache save was skipped. No image from #231.
- Candidate artifact ID `14147107920`; kernel 16,587,304 bytes; SHA-256 `b2d0abef3e20282d24945083cf530ce82c42bbfaeec2f1fa796d3aa0a8e6c1b3`.
- Independent inspection of that exact candidate found stage 8 at file offset `0x47a8c0` and stage 9 at `0x47a94c`; LLVM disassembly matches the expected five instructions for each. This is local binary evidence, not an official verifier/CI pass.
- Fix `tools/verify-lavender-kernel.py`: retain strict load-command and file-range checks; treat a zero-VM-size, non-executable, file-backed metadata segment such as `__CTF` as non-mapped data, not as a malformed executable section. Continue requiring every executable candidate section to fit its mapped segment and file. Test against the retained #231 kernel before triggering another full build.
- Then restore rich diagnostics (section inventory, load commands, full `__TEXT_EXEC,__text` disassembly, report and SHA); ensure the exact verified kernel is packaged. The package workflow `.github/workflows/package-lavender.yml` is currently **manual** (`workflow_dispatch`) and requires a successful XNU run ID.
- Do not start an overlapping build. Check current Actions state first. Full next-step detail and prior successful image evidence are in the handoff.

## Acceptance language
Say exactly which gates passed. A compiled candidate is not a verified kernel; a verified kernel is not a boot image; a boot image is not a successful physical boot. No Apple logo or Darwin userspace is established yet.
