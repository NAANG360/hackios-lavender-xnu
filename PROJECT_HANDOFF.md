# Lavender XNU — live project handoff

**Resume command:** “Continue from latest operations.”

Read this file, inspect the live branch/Actions state, then load only the routed chapter.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Device: Redmi Note 7/7S `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Kernel: XNU 8796, iOS 16 generation
- Intended Darwin userspace reference: iPhone X iOS 16.7.16 (20H392)

## Physical milestone

The user-supplied 20.317-second video `1000317742.mp4` shows Run #261's attributed image reaching the diagnostic logo screen on physical Lavender. Patch 0094 renders a hand-drawn approximation, not Apple's original logo. There is no device-side checksum in the recording and no launchd/SpringBoard evidence.

The real AppleLogo IM4P has since been extracted from the exact iPhone X 20H392 IPSW and hash-pinned. Patch 0114 and build-time conversion integrate the original 194×238 alpha mask at the same post-`arm_vm_init()` stage; Run #268 validates that build integration. Do not call it physically displayed until a new safe test.

## Latest known build state

- **#261**: fully built/verified/packaged and user-attributed physical logo-stage baseline.
- **#266**: latest previously completed full build/verify/package baseline using old diagnostic logo; artifact `14197706518`, SHA-256 `86518b66b094d25fe0124d091dd32f6a64841273d87c597bbed97a45e7280cf1`. Not separately physically tested.
- **#267**: failed in AppleLogo preparation at `mktemp` because shell variables were unnecessarily backslash-escaped.
- Commit `477a77fa43ad2bcb99c80550c96cccf9b33b3f31` fixes the script.
- **#268** (`43805363291`) is validating the corrected 0114 + AppleLogo preparation path. Check final build, linked verifier and RAM-only package result before claiming a new image.

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
