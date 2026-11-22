# LIOS — current continuation checkpoint

**Snapshot:** 22 November 2026. Always re-check live branch HEAD and Actions before changing source.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Verified source HEAD: `a9af621d0b31356383ed549004dcd051c17655b4` — `test(lavender): verify QUP-v2 tag definitions and encoder use`
- Active issue: Run #471 failed in `tools/verify-lavender-qup.py` with `QUP-v2 tags missing`. XNU compilation and the RPM object-inclusion check passed; the QUP source verifier stopped the job before the RPM source verifier, final Mach-O audit, or packaging.
- Root cause: the QUP tag encoder used correct numeric opcode values but did not define/use the symbolic `QUP_TAG2_*` names required by the verifier. A comment alone was not sufficient evidence.
- Repair commits: `8cc3017a5f978db550d1ddefa2613c84cf1b747b` defines and uses all five QUP-v2 opcodes without changing their wire values; `a9af621d0b31356383ed549004dcd051c17655b4` strengthens the verifier to require definitions and actual encoder use.
- Run #472 (`45054780165`) was cancelled when the verifier commit superseded it. Run #473 (`45054790646`), testing exact HEAD `a9af621d0b31356383ed549004dcd051c17655b4`, is in progress. QUP and RPM/GLINK host regressions passed; source verification, XNU rebuild/link, final Mach-O, and packaging remain pending at this snapshot.
- Latest accepted kernel/image remains Run #400, source `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`, artifact `14615937969`, `iosboot.img` 16,621,568 bytes, SHA-256 `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`. This is not an RPM-enabled accepted build.
- Next action: inspect Run #473 after completion. Require QUP source guard, RPM source guard, final ARM64 Mach-O symbol verification, RAM-only packaging, and independent manifest/SHA-256 agreement. If a gate fails, fix only that gate and run one validation cycle.
- Safety: Android/Lucid remains read-only. No flashing, partition writes, PMIC writes, or panel initialization. No physical test until the exact new image passes all software/package gates; then only explicitly gated RAM-only `fastboot boot iosboot.img` may be considered.
- Next-session reading: `AGENTS.md` working rules and only the relevant RPM/QUP section in `docs/handoff/05-native-lavender-iokit-drivers.md`. Read Chapter 02 only if the next blocker is linker/Mach-O/package integration.

Do not infer physical GLINK OPEN/OPEN_ACK, RPM-SMD ACK, LOW_SVS transition, QUP bus transactions, eMMC, touch, display, or Darwin userspace from host tests or compilation.
