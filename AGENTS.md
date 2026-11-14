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
- Current branch HEAD: always re-resolve the live `ci/lavender-machine-profile` ref before editing; the latest branch pointer and session evidence are recorded in `PROJECT_HANDOFF.md`.
- Latest successful build source commit: `352f7c3cc61a7001c35edee4bccd5c91265a32d3` (documentation commits may advance branch HEAD without changing the built source).
- Latest successful integrated build/package: GitHub Actions run **#138**, ID `43554970979`:
  https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43554970979
- Both XNU build and RAM-only packaging jobs passed. The kernel artifact is ID `14098277391`; the fastboot package artifact is ID `14098093027`.
- Raw `iosboot.img`: 16,527,360 bytes; SHA-256 `6e9a4cf545f644f6c7d064c421b19db92a2235bb1b4f19115083c82cfe5998ca`.
- Independently extracted kernel: Mach-O 64-bit ARM64 executable, 16,586,552 bytes; SHA-256 `6579b074f0942115ab5eb179aa1d0c3e277d677379fbfe35c485b30dea5972b0`.
- Build runner exposed 4 physical / 4 logical CPUs and ~15 GB RAM. Main XNU build used `make -j4`. Run #138 took 20m35s end-to-end and had a cold cache; do not claim a speedup until a same-configuration cache-hit build is measured.
- The image is for `fastboot boot` only. Never flash a partition. No physical Apple logo or successful XNU device boot has been confirmed.
- Before any physical boot request, review recovery/reset safety. The power button is broken and earlier automatic/volume-key recovery failed to recover the handset.
- For all further work, re-resolve live HEAD and latest Actions runs before editing. Read the entire PROJECT_HANDOFF.md, including appended sections, and all repository instructions. Record every meaningful commit, build run, artifact ID and hash in PROJECT_HANDOFF.md.

## Latest engineering delta — 2026-11-14

- Run #138 (`43554970979`) is the latest fully verified XNU + RAM-only packaging run. Its source commit is `352f7c3cc61a7001c35edee4bccd5c91265a32d3`; build job `132325656246` and package job `132330952177` both succeeded. The raw `iosboot.img` is 16,527,360 bytes with SHA-256 `6e9a4cf545f644f6c7d064c421b19db92a2235bb1b4f19115083c82cfe5998ca`; artifact ID `14098093027`. This is a structurally validated RAM-only fastboot image, not proof of a physical XNU boot.
- The #126 failure (`43462868674`) was after successful XNU compilation: the post-build entry-guard validator failed with `unsafe Lavender framebuffer access remains before VBAR_EL1`. The later #138 run passed that validator.
- The `xnu/patches/0096-lavender-no-fb-before-vbar.patch` path mentioned in an earlier handoff is absent on this branch. The actual pre-VBAR transformation is `tools/fix-lavender-pre-vbar.py`, injected into the pinned build script after patch application by `.github/workflows/xnu.yml`. Keep the handoff accurate about this distinction.
- The latest source change is `ce526931bf4dae15098b372c033f96d7acea0152`. It corrects the guardian's Qualcomm watchdog fallback using the SDM660 vendor one-tick bite sequence; `CNTFRQ_EL0` is not the watchdog clock. The updated shim compiled successfully in Lavender shim run #38 (ID `43570550237`, artifact `14103808558`). The matching XNU build #141 (ID `43570550271`) is still in progress; #139 is also in progress and #140 was cancelled. Run #138 remains the latest completed integrated XNU + package image and contains the older guardian. No integrated image containing the new guardian is verified yet.
- #138 runner evidence: 4 physical / 4 logical CPUs, 15,032,385,536 bytes RAM, actual `make -j4`, `KERNEL_BUILDS_IN_PARALLEL=4`; exact-key cache was cold. End-to-end run was about 20m35s. Do not claim a speedup until a comparable cache-hit build is measured.

- Reset safety remains incomplete: `shim.c::reset()` is a separate PSCI-only path used before the A53 guardian starts, and `lavender_guardian.c::reset_now()` invokes PSCI before arming the watchdog. A wedged SMC can therefore prevent the watchdog fallback from running. Do not request physical testing until these paths are unified or independently bounded and reviewed.
- Marker ordering also needs correction before relying on fault localization: Lavender `sweet_early_mark()` currently writes its framebuffer bar before publishing stages 1 and 2 to the shared marker cell. Publish those two markers before their own framebuffer writes while retaining cache clean/DSB ordering; keep stage 20 terminal publication after its final display operation.


## 2026-11-14 bring-up correction — framebuffer ownership and early-stage observability

- Two new device recordings show the same red shim console ending at `XNU jumping (fbcon clears screen)`. The A53 guardian is still printing `GUARDMK`, SPMI, and key-state diagnostics while the A73 loader/XNU are also using the same framebuffer. The crossed/merged console text is consistent with an unsynchronized dual-core framebuffer race.
- `shim.c` now completes the A53's final status line **before** publishing `A73_STATE_GO`. After release, the A53 guardian must not write routine framebuffer output.
- `shim/lavender_guardian.c` no longer prints cell, SPMI, or key-state diagnostics while XNU owns the display. Timeout/reset diagnostics remain fatal-path-only.
- Added `xnu/patches/0100-lavender-stage-entry-marker-order.patch`: stages 1 and 2 are flushed to the shared guard cell **before** their framebuffer writes; stages 3–20 remain post-display, with stage 20's high bit still terminal. This distinguishes arm_init/logo entry from a framebuffer-store stall.
- `reset_now()` now arms the SDM660 one-tick watchdog before invoking PSCI, so the watchdog is already armed if the PSCI SMC blocks. This follows the vendor `watchdog_v2.c` sequence of setting BITE_TIME, ordering the write, and resetting the counter; the current implementation explicitly enables the watchdog first. Do not claim physical validation of this new ordering until a test confirms it.
- The two uploaded recordings show the screen going dark at about 60 seconds and a charging icon shortly afterward. This is consistent with a reset after the guardian timeout, but does not identify whether PSCI or the watchdog caused it.
- Current source branch head at time of this entry: `59e6024ab1d17c5bd7d293b2ed7305df54ba3ed7`. Integrated build run #147 (`43572507408`) is in progress. Older overlapping pushes were cancelled; do not treat intermediate cancelled runs as validation.
- A local patch dry-run/application of patch 0100 against the exact run #141 XNU build tree succeeded.
- **No Apple logo has yet been observed. No physical validation of these new changes has occurred.** Continue with the latest integrated build and inspect the exact build log/artifact before requesting another RAM-only test. Never flash a partition.


## Final guardian ownership tightening — 2026-11-14

- Follow-up review found that fatal-path console output could still block before the reset watchdog was armed. Removed the guardian's console dependency and **all** framebuffer-console calls, including timeout and volume-key messages. The A53 guardian now only polls the shared RAM stage cell, timer, and PMIC; reset is display-independent.
- `reset_now()` no longer paints status text before reset. It first configures the SDM660 watchdog, then invokes PSCI. The `why` argument is intentionally ignored to avoid any display/log side effects.
- Final source commit: `f9555af4660a854ec03fb8d82e5a0ec94d9f566d`. Integrated run #43 (ID `43572760628`) is the queued run for that exact commit. Cancelled the prior #147 / #148 / run #42 builds so they cannot be mistaken for validation.
- Do not use any older artifact for the next device trial. Require the final commit's integrated XNU and packaging jobs to succeed first; then inspect its manifest and hashes. Still no confirmed Apple logo or physical validation of this revision. Never flash a partition.


## 2026-11-14 final-framebuffer-alias correction

- Source audit found a second distinct framebuffer bug at the permanent TTBR alias transition. `arm_vm_init.c` maps a 32 MiB VA block to PA `0x9c000000`, while Lavender scanout begins at PA `0x9d400000` (offset `0x01400000`, 20 MiB). The Lavender `sweet_early_relocate(base)` previously set `early_fb=base`, dropping the PA offset. Subsequent progress markers after TTBR0 teardown were therefore written to the wrong location.
- Added `xnu/patches/0101-lavender-preserve-framebuffer-alias-offset.patch`, which defines `LAVENDER_FB_ALIAS_OFFSET` and relocates to `base + LAVENDER_FB_ALIAS_OFFSET`.
- `xnu.yml` now checks the PA alias base, computed offset, adjusted relocation expression, and the mapping's 0x9c000000 physical base after patch application.
- The exact committed patch passed `patch --dry-run -p1` against the #141 build-tree version of `sweet_early.c`; both hunks applied (first with fuzz 1, second exact).
- Latest source commit for the integrated run: `d863da08a707353e73166502dcdaee3f5a117fdc`; integrated run #153 / ID `43573274639` is the selected build. Runs #149 and #152 were cancellation-requested as superseded.
- The shim compile-only run #43 (source commit `f9555af...`) had passed, but that run predates patch 0101; do not treat it as verification of the adjusted XNU alias. Require #153 to finish its XNU compile and RAM-only image packaging.
- No new image for patch 0101 and no physical test yet. No Apple logo has been confirmed. Never flash a partition.


### Ordered patch-chain verification
- Replayed the exact committed patch files 0100 then 0101 against the #141 build-tree copy of `xnu/osfmk/arm64/sweet_early.c`. Patch 0100 applied; patch 0101 then applied with its relocation hunk offset by the expected added marker lines. The ordered series succeeds, not just each patch independently.


### Packaging manifest reset semantics
- Corrected `.github/workflows/package-lavender.yml` so `BUILD-MANIFEST.txt` reflects watchdog-first reset ordering, with physical validation pending.
- Corrected the key-chord wording: Volume Up + Volume Down held for 3 seconds requests a hardware reset only; it does not set a recovery boot target. Do not instruct the user that this chord enters recovery.
- Package workflow is manually dispatched after a successful XNU run, with that XNU run ID. The selected XNU run #153 has not completed yet.
