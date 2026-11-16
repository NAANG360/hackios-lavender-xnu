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
- **Verifier correction**: commit `5dc07a913a1c4ac8fd1fb9e9d88e2f7d68a37664` bounds the trace-helper slice at the next function `sweet_early_logo(void)`. This fix is committed but its resulting workflow has not yet been validated.

## Current engineering direction

1. Accept #268 only after the AppleLogo prep guard, XNU compile, linked Mach-O verification and RAM-only package all pass.
2. Move to a read-only Lavender SDHC2 removable-card root. Exact Build 4 FDT confirms `sdhc@c084000`, 4-bit SD, GPIO54 card detect, host IRQ125, power IRQ221, PM660L L5/L2 rails and SDCC2 clocks 93/94.
3. Implement a strictly read-only register/state probe first. Then correctly service SDHCI MSM V5 power IRQ requests and PMIC/pinctrl transitions; only then add low-speed PIO SD-card reads and GPT/HFS+ block exposure.
4. Preserve internal SDHC1 eMMC at `0x0c0c4000` and all Android storage.
5. Mount the 20H392 HFS+ root and cryptex/dyld paths, then bring up launchd and essential daemons. IOMobileFramebuffer, input and SpringBoard are later.
6. Do not append the 16 GiB root image to RAM: current ADT has only roughly 1.10 GiB left after the kernel. The payload builder now rejects oversized RAMDisks before reading them.

**Read now:** Chapter 03 for the live build; Chapter 05 for genuine logo, post-logo kernel, SDHC2 and Darwin userspace.

## Safety

Power button is broken. Volume-up + volume-down is a reset request, not a verified recovery selector. Do not request another handset test until a power-button-independent recovery/reset route is physically verified. Preserve Android and recovery. RAM-only `fastboot boot iosboot.img` only; never flash, erase, repartition, or write boot-critical/internal-storage partitions.
