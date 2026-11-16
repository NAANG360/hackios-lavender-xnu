# Lavender XNU — live project handoff

**Resume command:** “Continue from latest operations.”

Read this file, inspect the live branch/Actions state, then load only the routed chapter.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Redmi Note 7/7S `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Kernel: XNU 8796, iOS 16 generation
- Intended Darwin userspace reference: iPhone X iOS 16.7.16 (20H392)

## Physical milestone

The user-supplied 20.295-second video `1000317742.mp4` shows the Redmi Note 7/Lavender displaying the Apple-shaped diagnostic mark with the #261-attributed image. This is a user-attributed physical observation, not device-checksum-bound. The mark is patch 0094's hand-drawn approximation, not Apple's original AppleLogo. It confirms arrival at the framebuffer logo-rendering stage; it does not prove stage 20, root mount, launchd, SpringBoard or a complete iOS boot.

The user explicitly rejects the custom approximation and requires the actual Apple logo. The exact iPhone X iOS 16.7.16 (20H392) `applelogo@3x~iphone.im4p` was extracted and hash-pinned (IM4P SHA `93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8`; decoded 194×238 PNG SHA `9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb`). Patch 0114 integrates that original raster at the same post-`arm_vm_init()` stage. It has not yet passed the full CI acceptance ladder or a physical test.

## Latest known build state

- **#261**: XNU build, verification and RAM-only packaging passed; user video shows the diagnostic logo-rendering stage.
- **#266**: full build/verification/package passed with the old diagnostic logo; artifact `14197706518`, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`; not separately physically tested.
- **#267**: failed during AppleLogo preparation because escaped shell variables reached `mktemp`; commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` fixed expansion.
- **#268**: XNU compiled; AppleLogo header/hash/source guard, commpage, SHA2 and framebuffer checks passed. It failed later in the startup-trace source verifier with `ValueError: substring not found`: the verifier still used removed `lavender_logo_ellipse` as the end of the trace-helper slice. Linked Mach-O verification and packaging were skipped.
- **Verifier correction**: commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` bounds the trace-helper slice at the next function `sweet_early_logo(void)`. A local check against the #268 patched buildtree confirms the new boundary and trace-scope assertions.
- **#269** (`43809333900`, source `5dc07a9`) is running the corrected workflow. At last inspection, the XNU build step was still in progress; require linked verification and RAM-only packaging before accepting its image.

## Latest operations — 16 November 2026

- **Run #274** ([43814890363](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814890363), source `5493d479039821941e0608a5e6191539eb11d52c`) passed kernel verification and RAM-only packaging, but its build log shows patches 0115 and 0116 were skipped as corrupt unified diffs. It remains a valid older AppleLogo/verbose-boot baseline; it does **not** contain the post-logo Darwin trace or overlay.
- **Run #275** ([43814903820](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43814903820)) failed a source guard after XNU build due to a literal-string regex mismatch; the package job was skipped.
- **Run #276** ([43817621737](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43817621737)) failed the required-patch guard. Its exact-cache build log reports patch 0115 corrupt at line 66 and patch 0116 corrupt at line 46; the restored kernel was not accepted as an updated overlay build.
- **Fix committed:** `2b327992dcef9223ddd8a4640bebc75fdd50c983` recalculates the old/new unified-diff hunk counts in both patch files without changing their intended code edits.
- **Run #277** ([43818798098](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/43818798098)) is queued. Require patch application, all source/binary guards, linked Mach-O verification and RAM-only packaging before accepting its image.
- **Visual status:** the genuine AppleLogo itself was physically observed in the earlier handset test. The 0115/0116 post-logo Darwin trace overlay has not yet been applied successfully or physically verified; replaying pre-logo console lines over the logo remains outstanding.


## Current engineering direction

1. Accept #268 only after the AppleLogo prep guard, XNU compile, linked Mach-O verification and RAM-only package all pass.
2. Use Lavender's **internal eMMC on SDHC1** as the iOS root target, stored in the existing `userdata` partition. Do not use removable SDHC2 as the root plan. The Build 4 FDT identifies SDHC1 at `0x0c0c4000` as an 8-bit, non-removable host with CQE/ICE resources; target-specific register, clock, rail and interrupt details still require exact audit.
3. Borrow Topaz's proven deployment architecture: an HFS+ iOS 20H392 root on `userdata`, XNU booted from RAM, and the root selected through a BSD block-device/root hook. Topaz uses UFS 3.0; Lavender needs its own SDHCI/eMMC transport and must not inherit Topaz UFS registers, descriptors or protocol.
4. Start with a non-destructive SDHC1 probe and read-only PIO discovery, then GPT and HFS+ reads. Add controlled writes only for the explicitly designated `userdata` root image after the read path and recovery procedure are validated. Never write other eMMC partitions.
5. Keep Android's boot/system/vendor/recovery/modem/persist partitions intact. The iOS `userdata` image will replace Android user data; returning to Android means using recovery to wipe/format `userdata`, not restoring an SD card or reflashing Android boot partitions. This is expected to erase personal data in `userdata`.
6. Mount the 20H392 HFS+ root and cryptex/dyld paths, then bring up launchd and essential daemons. IOMobileFramebuffer, input and SpringBoard are later. Do not append the 16 GiB root image to RAM: current ADT has only roughly 1.10 GiB left after the kernel.

**Read now:** Chapter 03 for the live build; Chapter 05 for genuine logo, post-logo kernel, SDHC1/eMMC `userdata` root and Darwin userspace.

## Safety

The user confirms the volume-key reset/recovery route works despite the broken power button. Preserve that route and verify the exact recovery behavior in the device procedure before any destructive `userdata` operation. RAM-only `fastboot boot iosboot.img` remains the kernel bring-up path. A later explicit `userdata` image write is in scope; no boot-critical, recovery, modem, persist or non-userdata partition writes are authorized. Wiping/formatting `userdata` is the planned Android-return path and destroys Android user data.
