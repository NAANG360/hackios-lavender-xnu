# Lavender XNU — agent entrypoint

**Read `PROJECT_HANDOFF.md` first.** It is the canonical, compact project record. Then inspect live branch HEAD, newest Actions run/attempt, jobs, artifacts and source SHA before acting. Never rely on an old status paragraph over live evidence.

## Mission
- Physical target: Xiaomi Redmi Note 7 / Note 7S, codename `lavender`, Qualcomm SDM660, ARM64.
- Port XNU 8796 toward the user's iOS 16.5-era Darwin goal. Immediate target: verified Lavender kernel and reversible RAM-only image; ultimate target: actual Darwin userspace.
- Use HakiOS/hackIOS, IOS-Sweet and public Lavender/SDM660 sources. Transfer methods, not other devices' hardware constants. Inherited Sweet/SM7150 docs are not Lavender facts.

## Non-negotiable rules
- No emulator work as a substitute for the physical device; QEMU only for isolated tests.
- Keep compile, Mach-O verification, package, physical boot, Apple logo and Darwin userspace as separate gates. Never claim an unobserved stage.
- Inspect actual logs, artifacts, generated source and compiled instructions. Fix failed assertions; never weaken/remove guards just to get green CI.
- Test changes locally against retained artifacts before another full 10–12 minute build. Avoid overlapping Actions runs.
- Preserve Android/recovery: no flashing, erasing or repartitioning boot-critical storage. Prefer RAM-only `fastboot boot`; permanent writes require explicit approval.
- User's power button is broken. No new handset test until a recovery path independent of that button is established.
- Exact user-facing image name: `iosboot.img` (no suffix/date). Keep raw dumps, serials, calibration data and private/firmware data out of Git.
- Update the handoff after each meaningful milestone with commit/run/attempt, job IDs, artifacts, sizes, hashes, diagnostics and physical observations.

## Resume point — 2026-11-15
- Active engineering branch: `ci/lavender-machine-profile`. Re-resolve its HEAD.
- Run #231 (`43677707091`, source `abff246071db828dab395bfa2d98587bf86b2474`) compiled XNU and passed the source guard, but the official Mach-O verifier rejected non-executable file-backed `__CTF,__ctf` metadata because its segment has `vmsize=0`. Package and cache-save were skipped; no #231 image.
- Retained candidate artifact `14147107920`: 16,587,304-byte kernel, SHA-256 `b2d0abef3e20282d24945083cf530ce82c42bbfaeec2f1fa796d3aa0a8e6c1b3`. Independent local scan + LLVM disassembly confirms stage 8 at `0x47a8c0` and stage 9 at `0x47a94c`; official verifier/CI has not passed.
- Next: fix `tools/verify-lavender-kernel.py` to allow file-backed non-executable metadata in zero-VM-size segments while retaining strict file bounds and mapped/executable-section VM bounds. Test against the #231 candidate first; then restore rich diagnostics, run one corrected CI build, and manually package only after XNU CI succeeds.
- Package workflow `.github/workflows/package-lavender.yml` is manual and requires a successful XNU run ID. Run #230 is the last successful kernel + structurally verified RAM-only image; it has not been physically tested.

**Truth standard:** compiled candidate ≠ verified kernel ≠ boot image ≠ physical boot. No Apple logo or Darwin userspace is yet verified.