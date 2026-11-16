# Chapter 3 — Current debugging state

**Branch:** `ci/lavender-machine-profile`
**Current CI operation:** XNU #263 / workflow `43800607832`
**Source commit:** `b6879a835d61a3382740737254a5c54b2eeeb2ee`
**Current physical frontier:** Run #261 image reaches the diagnostic logo screen on the handset, per user-supplied video.

## Latest accepted build — Run #261

Run #261 (`43789919931`, source `65f94b72544722d921dd332e2da94540d86b6709`) passed:
- XNU 8796 build;
- Lavender source/entry guards;
- ARM64 Mach-O kernel verification;
- RAM-only fastboot package job;
- boot-image structure and payload identity checks.

Artifact:
- Name: `lavender-fastboot-boot-test`
- ID: `14193115710`
- Exact image: `iosboot.img`
- Size: 16,551,936 bytes
- SHA-256: `ee96e9273a4699ca1da78d85e761cc8947ee1c520757fe2f87d2c314d9ea445c`
- The package also contains `shim/out/shim-boot.img`; the two image files are byte-identical.

This image is the accepted physical-test candidate. Run #262 produced no package and must not replace it.

## New physical evidence — `1000317742.mp4`

- Duration: 20.317 seconds; 1920×1080, H.264.
- At the beginning, the phone shows the red Lavender/XNU diagnostic screen.
- The recording then shows the phone display go black and a white bitten-apple-shaped mark illuminate in the center; this screen state remains visible in multiple sampled frames through the clip.
- The user identifies the tested image as Run #261.
- The video itself does not show a device-side checksum or a build identifier; preserve the Run #261 association as user-attributed, not byte-for-byte proven.

**Physical milestone:** XNU's framebuffer logo stage is now reported reached on the real Lavender handset. This supersedes the earlier #240/#236 statements that the Apple-logo stage had not been reached.

**Not proven by this clip:** stage-20/kprintf startup completion, stable scheduler/interrupt operation, root filesystem mount, launchd, system daemons, backboardd, SpringBoard, touch input or a usable iOS interface.

## Logo discrepancy — exact source found

Patch `xnu/patches/0094-lavender-logo-and-stage-markers.patch` implements `sweet_early_logo()` by drawing a white shape procedurally:
- three overlapping ellipse regions for the body;
- a circular bite subtraction;
- a calculated leaf ellipse;
- a rectangular stem;
- a framebuffer clear and white pixel writes.

The source comment calls this a **diagnostic Apple mark**. It is an approximation, not Apple's original boot image. The clip visibly shows this custom-rendered shape.

The requested correction is to use the actual AppleLogo image resource from the iPhone X iOS 16.7.16 (20H392) IPSW, not another hand-authored path. The AppleLogo is an iBoot image packed in an IMG4/IM4P resource in the IPSW's `Firmware/all_flash` tree (path varies by release). The `ipsw fw ibootim` tool can inspect/dump iBoot images; verify exact dimensions, pixel format and alpha channel before converting to the Lavender framebuffer format. Keep the logo call after `arm_vm_init()` and the permanent framebuffer alias; do not move it earlier to make the artwork easier to draw.

## Run #262 failure and fix

Run #262 (`43795080764`, source `71bb3a0dc0a45a7d5482fe45de869af6281bca41`):
- XNU 8796 compilation succeeded.
- The cache-aware build-path check succeeded.
- `Require Lavender XNU entry-guard patch` failed.
- Kernel verification was skipped.
- RAM-only image packaging was skipped.
- Only raw diagnostic artifacts were preserved; they are not a boot candidate.

Source inspection found the scope bug in the inline verifier: enqueue used the dequeue label as its end boundary, but dequeue passed `None`, so its scan extended to end-of-file. The second check could therefore be contaminated by unrelated later commpage functions. The small build-log artifact does not preserve the validator's full stderr, so do not invent a more specific unmatched instruction.

**Correction committed:**
- `276fb31395998538afcbb620d25c647ad9f5f47e`: the reusable verifier now requires CLREX inside the isolated busy-lock return path.
- `b6879a835d61a3382740737254a5c54b2eeeb2ee`: workflow guard now calls the reusable symbol-bounded source validator for the two target helpers, rather than scanning the second function to EOF.

**Run #263** started from the corrected workflow commit. It was in progress at handoff update time. Inspect this run rather than creating another one. A successful result must include both build and package jobs and a fresh exact `iosboot.img`; compilation alone is not acceptance.

## Next immediate operations

1. Inspect Run #263 completion and its artifacts. Confirm source guard, XNU build, kernel verifier and package all pass. Do not use #262's raw candidate.
2. Prepare the genuine AppleLogo asset from the matching iPhone X 20H392 IPSW and validate the binary format before changing `sweet_early_logo()`.
3. Instrument/observe the next existing XNU milestone after the logo and through stage 20 (`kprintf` startup complete). Do not infer this stage from the logo alone.
4. Move the main bring-up to the post-logo/root-device path in [Chapter 05](05-post-logo-userspace.md).

## Device safety

The power button is broken. The volume-up + volume-down chord is a reset request only; it is not a verified recovery selector. The watchdog-first reset path remains source-grounded but physically unvalidated. No new handset retest until an independent recovery/reset route is physically verified.

Preserve Android/recovery. Use RAM-only `fastboot boot iosboot.img`; never flash, erase, repartition or write boot-critical partitions.
