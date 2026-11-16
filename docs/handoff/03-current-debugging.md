# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Current validation:** Run #277, validating corrected 0115/0116 patch syntax

## Latest operations — 16 November 2026

- **Run #274** ([43814890363](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814890363)) passed kernel verification and RAM-only packaging, but the build log shows patches 0115 and 0116 were skipped as corrupt unified diffs. It remains an older AppleLogo/verbose-boot baseline without the post-logo Darwin trace overlay.
- **Run #275** ([43814903820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814903820)) failed a source guard because a literal C string was checked with basic-regex matching.
- **Run #276** ([43817621737](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43817621737)) failed the required-patch guard: 0115 was rejected as corrupt at line 66 and 0116 at line 46. The build was an exact-cache hit, not a fresh kernel compile.
- **Fix committed:** `2b327992dcef9223ddd8a4640bebc75fdd50c983` corrects every malformed old/new hunk count in 0115 and 0116.
- **Run #277** ([43818798098](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43818798098)) is queued. Next acceptance requires clean patch application, the required source guards, linked Mach-O verification and RAM-only packaging.
- **Important distinction:** the genuine AppleLogo was physically observed on-device in an earlier test. The 0115/0116 Darwin overlay is not yet built into an accepted artifact or physically verified.


## Physical baseline — Run #261

Run #261 (`43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed XNU build, source guards, linked Mach-O verification and RAM-only packaging. User video `1000317742.mp4` shows physical Lavender displaying the patch-0094 diagnostic Apple-shaped mark with the #261-attributed image. This is user-attributed, not checksum-bound. It confirms the framebuffer logo-rendering stage, not stage 20, root mount, launchd or SpringBoard. The user explicitly requires Apple's actual logo, not the custom approximation.

## Latest accepted package — Run #266

Run #266 (`43802934997`, source `aa07e58663cdbb7c4cc63084c55b6b7798b71cc5`) passed XNU build, kernel verification and RAM-only package:
- artifact ID `14197706518`, `lavender-fastboot-boot-test`;
- `iosboot.img` size 16,551,936 bytes;
- SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`.
This is an old diagnostic-logo baseline and has not been separately physically tested.

## Genuine AppleLogo integration

The exact iPhone X iOS 16.7.16 (20H392) firmware AppleLogo was extracted and hash-pinned:
- IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`;
- decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`.
Patch 0114 replaces the procedural logo pixels; it leaves framebuffer timing/call placement intact. Build-time helper generates the C header; the firmware image is not committed.

## Runs #267–#268 and exact correction

Run #267 (`43804842131`) failed in `prepare-lavender-applelogo.sh`: escaped shell variables caused `mktemp` to receive a literal `${TMPDIR:-/tmp}`. Commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` corrected shell expansion.

Run #268 (`43805363291`) compiled XNU and passed:
- generic ARMv8-A target check;
- both commpage LL/SC source guards;
- file-scoped SHA2 architecture check;
- exact 20H392 AppleLogo header hashes/dimensions and source integration check;
- pre-VBAR and deferred-logo source checks.

It then failed in the startup-trace verifier with `ValueError: substring not found`. The trace helper guard still sliced through `early.index("lavender_logo_ellipse", helper)`, but patch 0114 correctly removed that old custom-logo helper. This is a stale verifier boundary, not a compile or AppleLogo asset failure. Linked Mach-O verification and RAM-only packaging were skipped; raw candidate artifact `14198891426` is unverified and must not be booted.

**Fix committed:** `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` changes the slice end to the next function, `sweet_early_logo(void)`, keeping the trace-body check scoped to the trace helper. A local check against the #268 patched buildtree confirms the new boundary exists, the trace helper still contains events 5/7, and the helper slice does not include logo-renderer code or touch Guardian state. **Run #269** (`43809333900`) is executing the corrected workflow; at last inspection, `Build XNU 8796` was in progress. Its linked Mach-O verifier and RAM-only package must both pass before accepting the resulting image.

## Immediate next operations

1. Confirm the corrected follow-up run passes XNU build, all source guards, linked Mach-O verification and RAM-only package.
2. Preserve #274 as the successful baseline. Do not treat #275 as a package or use any raw, unverified kernel candidate.
3. Inspect the fresh `iosboot.img` artifact and record its exact size and SHA-256 before handset testing.
4. Continue the post-logo roadmap in Chapter 05: XNU stage-20/post-logo instrumentation, read-only SDHC2 discovery, V5 power sequencing, PIO SD reads, read-only BSD block device, GPT/HFS+ root and Darwin launchd.

## Storage frontier — exact Build 4 embedded FDT

The Build 4 boot image's own appended FDT has been extracted and confirms:
- removable SDHC2 at `0x0c084000/0x1000`, compatible `qcom,sdhci-msm-v5`, enabled, 4-bit;
- card-detect GPIO54, host IRQ125, power IRQ221;
- `iface_clk`/`core_clk` use GCC SDCC2 AHB/APPS clock IDs 93/94;
- PM660L L5 and L2 are the VMMC/VQMMC supplies;
- SDHC1 at `0x0c0c4000` is internal 8-bit non-removable eMMC with CQE/ICE and must not be used for iOS root.

The matching Xiaomi SDM660 downstream driver defines V5 power-control offsets 0x240/0x244/0x248/0x24c and services BUS_ON/OFF and IO_LOW/HIGH with PMIC rails and active/sleep pinctrl transitions before acknowledging IRQ221. The removable host is not ready for command I/O until this state is handled.

## Next operations

1. Inspect Run #268's full XNU build, official-logo generated-header guard, linked verifier and RAM-only package result. Do not use #267 raw candidate.
2. If #268 passes, preserve the artifact and do not physically retest until the independent recovery/reset gate is satisfied.
3. Start with a no-write SDHC2 register/GPIO/power-state probe, then implement the V5 power IRQ/regulator/pinctrl handshake, then low-speed PIO SD-card reads. No DMA/CQE/ICE/write path.
4. Expose a bounded read-only BSD block device and `rd=sdroot0`; validate GPT and HFS+ before launching Darwin userspace.

## Device safety

The power button is broken. Volume-up + volume-down is a reset request only, not a verified recovery selector. No handset test until an independent recovery/reset route is physically verified. RAM-only `fastboot boot iosboot.img` only. Never flash, erase, repartition or write internal Android eMMC/userdata.
