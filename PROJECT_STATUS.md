# LIOS Project Status

**Status date:** 22 November 2026  
**Repository:** https://github.com/NAANG360/hackios-lavender-xnu  
**Branch:** `ci/lavender-machine-profile`  
**Last verified branch HEAD before this status refresh:** `d72ff5025bc66907ce3fb306bae672e90ba769d2` (workflow repair; source under validation: `6c52dae`)  
**Mission:** Bring up a functional XNU 8796-based system on Redmi Note 7/7S (`lavender`, SDM660, ARM64), progressing through native hardware services to Darwin userspace.

## Current milestone

Native RPM/GLINK integration remains under validation. The PlatformExpert nub-publication iterator repair is source commit `6c52dae`. Run #470 passed both host suites, patch application, staged-source verification and XNU 8796 compilation, but failed on a stale CI assertion that searched the raw linker transcript for a symbol already verified by the pre-link `nm` helper. Workflow repair `d72ff50` now checks the helper's explicit PASS evidence and adds an independent final-Mach-O symbol check.

## Repository state

- Current verified workflow-repair commit: [`d72ff50`](https://github.com/NAANG360/hackios-lavender-xnu/commit/d72ff5025bc66907ce3fb306bae672e90ba769d2) — `fix(ci): verify RPM symbol at prelink and final Mach-O`.
- RPM iterator source repair remains [`6c52dae`](https://github.com/NAANG360/hackios-lavender-xnu/commit/6c52daeb43a9663feb133adf076e0596b2324d10).
- Workflow change: replace invalid transcript-symbol grep with explicit pre-link verifier PASS assertions; require exactly one strong global vote-symbol definition in the final ARM64 Mach-O.
- Previous live HEAD before workflow repair: `bf9e5bf1ddf7e535959781a6b6ccdb79e83ef21b`.

## Latest accepted build

- Run #400: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280
- Source: `7a15aaad2d649f570ffe3f9ad23d1a98f659085d`
- Artifact: `lavender-fastboot-boot-test`, ID `14615937969`
- Image: `iosboot.img`, 16,621,568 bytes
- SHA-256: `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2`
- This accepted image contains the QUP source/build milestone, not the currently staged RPM/GLINK integration. Preserve it unchanged.
- No physical RPM/QUP operation or full Darwin userspace is proven by this image.

## Run #470 — completed failure; workflow assertion repaired

- Run #470: https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45018828868
- Source commit: `6c52daeb43a9663feb133adf076e0596b2324d10`
- QUP and RPM/GLINK host protocol regressions: passed.
- Patch application and staged-source verifier: passed.
- XNU 8796 compilation and kernel link command: passed.
- Pre-link audit: passed; exact non-LTO `link.filelist` includes `LavenderRPMGLINK.cpo`, and the helper's `nm -g` audit found one strong global text definition of `lavender_rpm_vote_vdd_dig_low_svs`.
- Run failed immediately afterward because a separate shell step incorrectly expected the raw linker transcript to contain the symbol name. The link transcript contains the object path, not the object's symbol table.
- Buildtree artifact `xnu-buildtree` ID `14753287881`, ZIP SHA-256 `782e6d7e9071dd3ab79ca3ac6e47425d570a676a50c34190b89f7abd0997d27f`; diagnostic only. Its archived `link.filelist` includes the RPM object exactly once. The buildtree archive does not contain the final kernel Mach-O.
- Build log artifact `xnu-build-log` ID `14752839688`, ZIP SHA-256 `bb34a7e59286914f9352a86bf34ddd9a533837748258ed864befd457c9c90ef0`.
- Final ARM64 Mach-O symbol audit, image packaging, manifest and independent image checksum were not reached.
- Workflow repair commit `d72ff5025bc66907ce3fb306bae672e90ba769d2` fixes the false-negative assertion and adds a separate final-Mach-O symbol audit. Await its new Actions run.

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
| XNU 8796 compilation after iterator repair | Passed in Run #470 |
| Non-LTO final-link input includes RPM object exactly once | Passed by Run #470 pre-link verifier and archived link.filelist |
| Final Mach-O strong vote-symbol verification | Pending next workflow after `d72ff50` |
| RAM-only package and manifest/SHA-256 | Not produced for RPM source commit; next workflow pending |
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
- Nub publication compile fix compiled successfully in Run #470; its linked input and strong object symbol passed the pre-link verifier. Final Mach-O inclusion remains pending.
- No remote acknowledgement or voltage transition is claimed.

### QUP I2C
- Native `LavenderQUPI2C` previously compiled and linked in Run #400.
- Accepted scope is source/build integration only, not a physical bus.
- Remaining work includes block mode for the 68-byte Novatek read envelope, TLMM pinmux for GPIO2/3, bus-clear recovery, runtime transaction evidence and physical verification. The current source branch additionally gates QUP clocks on an acknowledged RPM voltage vote.

### Other hardware
- No accepted SDHCI/eMMC host, Novatek HID/input driver, or MDSS/DSI/Tianma panel driver.
- Do not start Novatek client integration before acknowledged RPM vote and a successful physical I2C transaction.

## Known blockers

1. Workflow repair `d72ff50` needs a fresh Actions run; Run #470 was blocked by the stale transcript grep, not by compilation or the pre-link audit.
2. The next run must independently prove the strong vote symbol survives in the final ARM64 Mach-O.
3. RAM-only image packaging, manifest identity and independent SHA-256 remain unverified for RPM-enabled source.
4. No physical GLINK OPEN/OPEN_ACK, RPM-SMD ACK, accepted LOW_SVS vote or QUP bus transaction has been observed.

## Safety restrictions

- Android 15/Lucid ROM is confirmed working and must remain strictly read-only.
- Never flash, erase, format, repartition or write any Android partition.
- Physical testing only after all build, link, Mach-O, packaging and checksum gates pass; then use only `fastboot boot iosboot.img`.
- Preserve `cpus=1` and independent volume-key recovery.
- No weak vote-symbol stubs, fake acknowledgements, bypassed voltage-vote gates or direct PMIC-register workarounds.
- No experimental display/panel bring-up during RPM/QUP validation.
- Never infer physical functionality or Darwin userspace from compilation, packaging or a static framebuffer.

## Exact next actions

1. Inspect the Actions run triggered by workflow repair `d72ff50`; confirm both host suites, staged-source checks and compilation.
2. Confirm the pre-link guard passes on exact linker inputs and the new final-Mach-O `nm` check finds exactly one strong global vote symbol.
3. Inspect ARM64 Mach-O type/architecture, load commands and the existing Lavender kernel invariants.
4. Confirm RAM-only image packaging, manifest-to-image identity and an independently recomputed SHA-256.
5. Preserve Run #400 unchanged. Do not flash or write Android partitions.
6. Only after all build/package gates pass, consider `fastboot boot` RAM-only testing and capture actual GLINK/RPM/QUP runtime evidence.

## Detailed documentation

- `AGENTS.md`: agent entrypoint and safety rules.
- `PROJECT_HANDOFF.md`: architecture and longer-term continuation guide.
- `docs/handoff/05-native-lavender-iokit-drivers.md`: active RPM/GLINK/QUP engineering record.
- `docs/handoff/06-validation-and-physical-bringup.md`: validation evidence ledger.
