# Lavender XNU — live project handoff

**Resume command:** “Continue from latest operations.”

Read this file, inspect the live branch/Actions state, then load only the routed chapter. Current repository state outranks this snapshot if it changes.

- Repository: https://github.com/NAANG360/hackios-lavender-xnu
- Branch: `ci/lavender-machine-profile`
- Target: Xiaomi Redmi Note 7/7S, `lavender`, Qualcomm SDM660 / Kryo 260, ARM64
- Kernel: XNU 8796 (iOS 16 generation)
- Intended userland reference: iPhone X iOS 16.7.16 (20H392)

## Current milestone

**Run #261 reaches the logo stage on the physical phone, according to the user-supplied recording.** The 20.317-second video `1000317742.mp4` first shows the red diagnostic display and then a white bitten-apple-shaped image on a black display. The user identifies the image as the Run #261 build.

Important qualification: this is the hand-drawn diagnostic logo from patch 0094, not Apple's actual boot-logo resource. The user explicitly wants the genuine Apple logo. Do not label this graphic “official Apple logo.” Replace the artwork without regressing the now-working startup path.

The clip shows no launchd, SpringBoard or Darwin userspace. It does not independently prove a device-side checksum. Treat the run association as user-attributed rather than cryptographically bound.

## Latest known CI state

- **#261** — workflow `43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`: build and package jobs both passed. The kernel verifier and RAM-only package checks passed.
- Image artifact `14193115710`, name `lavender-fastboot-boot-test`; exact `iosboot.img`, 16,551,936 bytes; SHA-256 `ee96e9273a4699ca1da78d85e761cc8947ee1c520757fe2f87d2c314d9ea445c`.
- **#262** — workflow `43795080764`, source `71bb3a0dc0a45a7d5482fe45de869af6281bca41`: XNU compilation passed, then the Lavender entry-guard step failed. Kernel verification and packaging were skipped. Do not use #262's raw kernel candidate.
- **#263** (`43800607832`) and **#264** (`43800747274`) passed source guards but failed because an exact cache hit emitted no compiler invocation while a log grep expected one.
- **#265** — workflow `43801366044`, source `f3526da7461c804188f7395ad964cda9c80542da`: build, source guards, linked Mach-O verification and RAM-only packaging all passed. Artifact `14196716179`; exact `iosboot.img` size 16,551,936 bytes, SHA-256 `c1dfc841fab6545e0c4892c7f01dba4ecd7c52f972e9ff7c00ca6230f6a8537d`. Kernel binary is identical to #261; full image differs due the randomized ADT seed.

## Current task / chapter route

1. Let #263 finish; inspect the guard, build, linked-kernel verification, package jobs and exact `iosboot.img` artifact. Do not overlap runs.
2. Replace the custom diagnostic mark with the real AppleLogo resource extracted from the same iPhone X iOS 16.7.16 (20H392) IPSW used by the userland reference. Do not approximate or redraw the mark. See Chapter 05.
3. Continue kernel startup beyond the visible logo. The video is 20.317 seconds and captures no stage-20/kprintf completion marker, root mount or userspace. Add/inspect narrowly scoped post-logo evidence before claiming those stages.
4. Begin the storage/root path. The inherited Sweet path is UFS-based and assumes an HFS+ root on `userdata`; Lavender uses eMMC/SDHCI. No Lavender SDHCI block driver is implemented in the inspected source. A non-destructive root-storage design is required before any system image is installed or mounted.
5. Once the root is reachable, bring up the actual iOS 20H392 root tree, launchd and required early daemons; then framebuffer, input and SpringBoard integration.

**Read now:** [Chapter 03 — Current debugging](docs/handoff/03-current-debugging.md) and [Chapter 05 — Post-logo and Darwin userspace](docs/handoff/05-post-logo-userspace.md).

**Read additionally only when needed:**
- [Chapter 02 — Architecture and build](docs/handoff/02-architecture-and-build.md): XNU source, patches, compiler, cache, linked verification and packaging.
- [Chapter 01 — Device and safety](docs/handoff/01-device-and-safety.md): mandatory before physical testing or any storage/recovery operation.
- [Chapter 04 — Evidence and history](docs/handoff/04-evidence-and-history.md): provenance and older diagnostic history.

## Hard constraints

Preserve Android and recovery. RAM boot only; no flashing, erasing, repartitioning or boot-critical writes. The handset power button is broken, and the volume-key chord is not a verified recovery selector. No new physical test until an independent recovery/reset route is physically verified.

Do not confuse a successful XNU build/package with a physical boot; do not confuse a diagnostic Apple-shaped mark with Apple's official boot artwork; and do not confuse a visible kernel logo with Darwin userspace.
