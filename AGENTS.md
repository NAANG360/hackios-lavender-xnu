# Agent instructions — Lavender XNU port

**Read PROJECT_HANDOFF.md first.** It is the canonical, current cross-session record for the Redmi Note 7 / Lavender / SDM660 XNU port. It contains the project goal, hardware audit facts, source references, exact CI failures, artifacts, and next actions.

README.md and NOTES.md in this repository contain inherited IOS-Sweet / Redmi Note 10 Pro / SM7150 documentation. Use them only as XNU engineering references where explicitly relevant; do not treat their hardware facts or boot status as Lavender facts. Do not overwrite their historical content without a reason.

Working rules:
- At session start, inspect current branch HEAD and newest GitHub Actions run; handoff details can become stale.
- Keep the target on the physical Redmi Note 7 (lavender, SDM660). QEMU may support isolated tests but is not proof of device compatibility.
- Use HakiOS, IOS-Sweet, and public Lavender / SDM660 sources as documented in PROJECT_HANDOFF.md. Transfer methods, not other devices' MMIO addresses, IRQs, CPU topology, panel/touch, regulators or storage assumptions.
- Make small, evidence-based source changes; commit completed work; run relevant validation and CI; record exact run IDs, commit SHAs, diagnostics, artifact hashes and verification.
- A profile preflight, build tree or artifact upload is not a kernel. Require a real ARM64 Mach-O XNU kernel and verify it before reporting success.
- Never claim a build or device test passed without tool/device evidence.
- Keep raw device dumps, serials, calibration data, firmware blobs and personal data out of Git.
- Preserve the user's Android/recovery path. Do not flash or erase boot-critical partitions or repartition without explicit confirmation. Prefer RAM-only temporary boot for bring-up.
- Update PROJECT_HANDOFF.md after every meaningful milestone and before ending a session.


## Latest verified state — 2026-11-14

- Current branch: `ci/lavender-machine-profile`.
- Current verified HEAD at handoff update: `352f7c3cc61a7001c35edee4bccd5c91265a32d3`.
- Latest successful integrated build/package: GitHub Actions run **#138**, ID `43554970979`:
  https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43554970979
- Both XNU build and RAM-only packaging jobs passed. The kernel artifact is ID `14098277391`; the fastboot package artifact is ID `14098093027`.
- Raw `iosboot.img`: 16,527,360 bytes; SHA-256 `6e9a4cf545f644f6c7d064c421b19db92a2235bb1b4f19115083c82cfe5998ca`.
- Independently extracted kernel: Mach-O 64-bit ARM64 executable, 16,586,552 bytes; SHA-256 `6579b074f0942115ab5eb179aa1d0c3e277d677379fbfe35c485b30dea5972b0`.
- Build runner exposed 4 physical / 4 logical CPUs and ~15 GB RAM. Main XNU build used `make -j4`. Run #138 took 20m35s end-to-end and had a cold cache; do not claim a speedup until a same-configuration cache-hit build is measured.
- The image is for `fastboot boot` only. Never flash a partition. No physical Apple logo or successful XNU device boot has been confirmed.
- Before any physical boot request, review recovery/reset safety. The power button is broken and earlier automatic/volume-key recovery failed to recover the handset.
- For all further work, re-resolve live HEAD and latest Actions runs before editing. Read the entire PROJECT_HANDOFF.md, including appended sections, and all repository instructions. Record every meaningful commit, build run, artifact ID and hash in PROJECT_HANDOFF.md.
