# LIOS Project Status

**Status date:** 22 November 2026  
**Repository:** https://github.com/NAANG360/hackios-lavender-xnu  
**Branch:** `ci/lavender-machine-profile`  
**Verified HEAD:** `6c52daeb43a9663feb133adf076e0596b2324d10`  
**Mission:** Bring up a functional XNU 8796-based system on Redmi Note 7/7S (`lavender`, SDM660, ARM64), progressing through native hardware services to Darwin userspace.

## Current milestone

Native RPM/GLINK integration is staged in the repository. The latest observed CI build exposed an IOKit iterator type mismatch in the PlatformExpert's `/soc/rpm-glink` nub publication. A source repair is committed; it has not yet passed a new CI build.

## Repository state

- Current commit: [`6c52dae`](https://github.com/NAANG360/hackios-lavender-xnu/commit/6c52daeb43a9663feb133adf076e0596b2324d10) — `fix(lavender): publish RPM nub through IOKit iterator`.
- Changed source files: `xnu/patches/0130-lavender-rpm-glink-v0.patch`, `tools/verify-lavender-rpm-glink.py`.
- Previous live HEAD: `837cff8ca90d0e6a510da628ba4c20185c33e34d`.
- Recheck the branch before editing; a remote view cannot establish an uncommitted working tree.

## Latest accepted build

- Run #400: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280
- Source: `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`
- Artifact: `lavender-fastboot-boot-test`, ID `14615937969`
- Image: `iosboot.img`, 16,621,568 bytes
- SHA-256: `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`
- This accepted image contains the QUP source/build milestone, not the currently staged RPM/GLINK integration. Preserve it unchanged.
- No physical RPM/QUP operation or full Darwin userspace is proven by this image.

## Current validation in progress

- Run #470: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45018828868
- Source commit: `6c52daeb43a9663feb133adf076e0596b2324d10`
- QUP and RPM/GLINK host protocol regression: passed.
- Patch application and staged-source verifier: passed.
- XNU 8796 compilation: in progress.
- Final ARM64 link, pre-link object audit, Mach-O verification, image packaging and checksum: not yet confirmed.

## Latest failed validation

- Run #469: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44936263614
- Source commit: `7abd0cf29074da5a34dee77da58fd022b6912534`
- QUP host protocol regression: passed.
- RPM/GLINK host protocol regression: passed.
- XNU build: failed at `LavenderPlatformExpert.cpp:86`; `OSArray *` was passed to `createNubs()`, which requires `OSIterator *`.
- ARM64 link, linked-object checks, kernel artifact verification and RAM-only package: skipped/not produced.
- Diagnostic artifacts are not bootable images.

## Current repair

Commit `6c52dae` changes the RPM patch and verifier:
- Constructs `OSCollectionIterator` from the one-entry RPM nub array.
- Releases the temporary array after iterator construction.
- Transfers the iterator to `createNubs()`.
- Verifier rejects the old invalid `createNubs(this, rpmNubs)` expression.
- Apple's XNU 8796.141.3 API confirms the consumed `OSIterator *` signature and collection iterator factory.
- Four patch hunk bodies pass independent count validation.

## Validation matrix

| Gate | Current status |
|---|---|
| QUP host tests on Run #469 source | Passed |
| RPM/GLINK host tests on Run #469 source | Passed |
| Patch hunk counts on current HEAD | Passed |
| XNU API signature/source inspection | Passed |
| QUP/RPM host tests on current source commit (`6c52dae`) | Passed in Run #470 |
| Patch application in Run #470 | Passed |
| Staged PlatformExpert verifier in Run #470 | Passed |
| XNU 8796 compilation after iterator repair | In progress in Run #470 |
| Final ARM64 link with RPM object and strong vote symbol | Not verified |
| Independent Mach-O verification | Not run |
| RAM-only package and manifest/SHA-256 | Not produced for current HEAD |
| Physical GLINK OPEN / OPEN_ACK | Not tested |
| RPM-SMD ACK / accepted LOW_SVS vote | Not tested |
| Physical QUP I2C transaction | Not tested |
| eMMC, touch, display, BSD init, root mount, launchd, SpringBoard | Not established |

## Implementation status

### RPM / GLINK
- Native driver source and protocol implementation are present.
- Intended transport: SDM660 RPM shared message RAM/TOC/FIFO, APCS mailbox, GLINK v1 `rpm_requests` OPEN/OPEN_ACK, RPM-SMD v0 request/ACK.
- Initial request: `rwcx`, resource ID 0 (`vdd_dig`), `vlvl=64` LOW_SVS.
- QUP clock enable remains gated on a matching successful RPM ACK.
- Nub publication compile fix is committed but unbuilt.
- No remote acknowledgement or voltage transition is claimed.

### QUP I2C
- Native `LavenderQUPI2C` previously compiled and linked in Run #400.
- Accepted scope is source/build integration only, not a physical bus.
- Remaining work includes block mode for the 68-byte Novatek read envelope, TLMM pinmux for GPIO2/3, bus-clear recovery, runtime transaction evidence and physical verification. The current source branch additionally gates QUP clocks on an acknowledged RPM voltage vote.

### Other hardware
- No accepted SDHCI/eMMC host, Novatek HID/input driver, or MDSS/DSI/Tianma panel driver.
- Do not start Novatek client integration before acknowledged RPM vote and a successful physical I2C transaction.

## Known blockers

1. No new XNU workflow result has been observed for commit `6c52dae`.
2. Current repair still needs host regression, exact staged-source application and source-verifier validation.
3. The RPM translation unit must compile and appear exactly once in the actual final ARM64 link input.
4. Strong global definition of `lavender_rpm_vote_vdd_dig_low_svs` must be verified by the pre-link guard.
5. A successful RAM-only image and checksum-bound physical evidence do not yet exist for RPM-enabled source.

## Safety restrictions

- Android 15/Lucid ROM is confirmed working and must remain strictly read-only.
- Never flash, erase, format, repartition or write any Android partition.
- Physical testing only after all build, link, Mach-O, packaging and checksum gates pass; then use only `fastboot boot iosboot.img`.
- Preserve `cpus=1` and independent volume-key recovery.
- No weak vote-symbol stubs, fake acknowledgements, bypassed voltage-vote gates or direct PMIC-register workarounds.
- No experimental display/panel bring-up during RPM/QUP validation.
- Never infer physical functionality or Darwin userspace from compilation, packaging or a static framebuffer.

## Exact next actions

1. Recheck live branch HEAD and newest Actions runs.
2. Run QUP and RPM/GLINK host suites on the exact current checkout.
3. Apply patch 0130 to a fresh exact post-0129 XNU staged tree; run the source verifier.
4. Confirm non-LTO and PRE_LTO link hooks inspect the exact linker inputs immediately before linking.
5. Trigger one XNU workflow after all preceding checks pass.
6. Inspect build logs, linked ARM64 Mach-O, artifact manifest and independent SHA-256.
7. Only after acceptance consider a RAM-only physical boot and capture GLINK/RPM/QUP evidence.

## Detailed documentation

- `AGENTS.md`: agent entrypoint and safety rules.
- `PROJECT_HANDOFF.md`: architecture and longer-term continuation guide.
- `docs/handoff/05-native-lavender-iokit-drivers.md`: active RPM/GLINK/QUP engineering record.
- `docs/handoff/06-validation-and-physical-bringup.md`: validation evidence ledger.
