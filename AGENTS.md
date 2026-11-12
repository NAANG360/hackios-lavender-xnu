# Agent instructions — Lavender XNU port

**Read PROJECT_HANDOFF.md first.** It is the canonical cross-session record for the Redmi Note 7 / Lavender / SDM660 XNU port. It contains the project goal, sanitized hardware audit facts, source references, exact CI failures, artifact identifiers, and next engineering actions.

README.md and NOTES.md contain inherited IOS-Sweet / Redmi Note 10 Pro / SM7150 documentation. Use them only as XNU engineering references where relevant; do not treat their hardware facts or boot status as Lavender facts. Preserve their historical content unless a deliberate update is needed.

Working rules:
- At session start, inspect current branch HEAD and newest GitHub Actions run; the handoff may become stale.
- Keep the target on the physical Redmi Note 7 (lavender, SDM660). QEMU may support isolated tests but is not proof of device compatibility.
- Use HakiOS, IOS-Sweet, and public Lavender / SDM660 sources as documented in PROJECT_HANDOFF.md. Transfer methods, not other devices' MMIO addresses, IRQs, CPU topology, panel/touch, regulator or storage assumptions.
- Make small, evidence-based source changes; commit completed work; run relevant validation and CI; record exact run IDs, commit SHAs, diagnostics, artifact hashes and verification.
- A profile preflight, build tree or artifact upload is not a kernel. Require a real ARM64 Mach-O XNU kernel and verify it before reporting success.
- Never claim a build or device test passed without tool/device evidence.
- Keep raw device dumps, serials, calibration data, firmware blobs and personal data out of Git.
- Preserve the user's Android/recovery path. Do not flash or erase boot-critical partitions or repartition without explicit confirmation. Prefer RAM-only temporary boot for bring-up.
- The user's power button is broken; do not assume procedures requiring it are usable.
- Update PROJECT_HANDOFF.md after every meaningful milestone and before ending a session.
