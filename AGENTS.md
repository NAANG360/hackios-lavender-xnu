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


## Latest verified state — 2026-11-14 (Asia/Kolkata)

- Live branch HEAD at this handoff: `e9890318f144df763a3ff655a16ca5b5b5447eb5`.
- Latest successful integrated XNU + RAM-only package run: **#203**, ID `43594864654`, source commit `e9890318f144df763a3ff655a16ca5b5b5447eb5`. Build job `132447103223` and package job `132451071971` both succeeded: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43594864654
- The final ordered Lavender verbose diagnostics patch is `xnu/patches/0106-lavender-verbose-boot-final.patch` (blob SHA `8dcb9b2f2ddc9dd43771986b1e61906ab32601b0`). Run #203's build log explicitly shows patch 0106 being applied; the entry-guard job then passed its source assertions and the ARM64 Mach-O kernel verification.
- Kernel SHA-256 from package manifest: `be2cdd6b08d8d687aa00d32b3b901878fd8d4d9fe49f2ae121d5b1f9046a61d8`.
- Latest fastboot artifact: `lavender-fastboot-boot-test`, artifact ID `14114386476`, 16,979,730 bytes, expires 2026-11-28. It contains `iosboot.img` (16,527,360 bytes), independently SHA-256 verified as `172f6cb8b39241dc9997ecf5da1d1eaf5d18d2edb447711baff407c44136ac5f`. It is a **RAM-only fastboot boot image**; do not flash it to a partition.
- Other run #203 artifacts: kernel ID `14114156891`; build log ID `14114141998`; build tree ID `14114590813`. The build log is 3,058,984 bytes; the build-tree artifact expires 2026-11-21.
- Verbose diagnostics design: assembly stages 3–7 paint green 16×16 tiles beside the existing blue stage-2 tile. C checkpoints 1–15 write a persistent 8×16 glyph text history into the lower framebuffer panel using `iso_font`; the Apple logo is deferred until after `arm_vm_init()` and the permanent framebuffer alias is established (stage 16/17). The kernel command line still includes `lavender_no_fbcon=1`, which suppresses the legacy pexpert framebuffer serial console; it does not disable the new direct early-checkpoint renderer.
- **Manifest wording defect:** run #203's `BUILD-MANIFEST.txt` incorrectly says C milestones 1–15 are 8×8 tiles before the Apple mark. The actual 0106 source renders C milestones as text glyphs; only assembly stages 3–7 are tiles. Correct the manifest wording in a future workflow-only commit and ensure it does not imply that the legacy serial fbcon is the new early console.
- The package workflow's structural verification passed. The image hash and successful CI prove build/package integrity, **not** physical boot success.
- **Physical status is unchanged:** last confirmed device checkpoint remains `XNUSTAGE=1`; no physical stage 2, verbose console, Apple logo, stable `arm_init()`, or iOS userspace is confirmed. Do not claim a successful handset boot.
- Watchdog-first reset code remains source/build-validated but not physically validated. The Volume Up + Volume Down 3-second chord requests reset only; it does not select recovery. Because the user's power button is broken, independently establish a reliable recovery route before requesting another handset test. Never flash or erase boot-critical partitions; prefer RAM-only boot.
- Known non-fatal upstream patch skips in run #203 include inherited Sweet HFS patches targeting absent `bsd/sweet_hfs` paths and upstream compatibility patches for APIs already changed in XNU 8796. Review only if they affect Lavender build/runtime; do not conflate them with the Lavender 0106 patch, which was explicitly applied.
- Older entries below this section in `PROJECT_HANDOFF.md` are historical. Use section **44.25** for the latest diagnostics implementation and run #203 evidence. Always re-resolve live HEAD and Actions state at session start.

## Working rules for the next session

- Read `PROJECT_HANDOFF.md` first, then inspect current branch HEAD and latest Actions run before editing.
- Preserve the physical target: Redmi Note 7/7S `lavender`, SDM660. QEMU is never device-compatibility proof.
- Do not infer physical success from compile, package, Mach-O, boot-header, or static source checks.
- Inspect the run #203 artifact and exact log before requesting a handset test. The next useful device observation is whether the screen displays assembly stage tiles, then the C-stage text panel, then the deferred Apple logo.
- If the phone is tested, capture the full boot sequence and note the last visible checkpoint. Do not alter the reset/recovery path without independently reviewing the broken-power-button recovery plan.
- Correct the #203 manifest's stale description of C-stage diagnostics in a workflow-only change; keep the build's `lavender_no_fbcon=1` isolation setting until the early path is proven stable.
- Record each meaningful source change, run ID, job IDs, artifact IDs, hashes, and physical observations in `PROJECT_HANDOFF.md`. Keep observations, hypotheses, and proven results separate.
