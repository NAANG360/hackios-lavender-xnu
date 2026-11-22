# LIOS Validation and Physical Bring-up Evidence Ledger

This ledger records only observed evidence. Build, package, RAM boot, and subsystem operation are separate acceptance levels.

## 22 November 2026 — Run #470 completed; false-negative link assertion

| Field | Value |
|---|---|
| Date | 2026-11-22 |
| Commit | `6c52daeb43a9663feb133adf076e0596b2324d10` |
| CI run | [Run #470 / 45018828868](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45018828868) |
| Artifacts | `xnu-buildtree` ID `14753287881`; `xnu-build-log` ID `14752839688` |
| SHA-256 | Buildtree ZIP: `782e6d7e9071dd3ab79ca3ac6e47425d570a676a50c34190b89f7abd0997d27f`; build-log ZIP: `bb34a7e59286914f9352a86bf34ddd9a533837748258ed864befd457c9c90ef0` |
| Tests | QUP host protocol regression; RPM/GLINK host protocol regression; patch application; staged-source verifier; XNU 8796 compile/link; non-LTO RPM final-link-input verifier |
| Result | Failed at a redundant CI assertion after compilation and pre-link verification |
| Evidence | Both host regressions, patch application and staged-source verifier passed. XNU 8796 compiled and linked. The pre-link verifier reported that `LavenderRPMGLINK.cpo` is a final kernel link input and defines the strong global text symbol `lavender_rpm_vote_vdd_dig_low_svs`. The archived `link.filelist` contains the RPM object exactly once. |
| Failure detail | The next shell step incorrectly grepped the raw link transcript for `_lavender_rpm_vote_vdd_dig_low_svs`. The transcript records the linker input object path; the symbol table was already checked by the pre-link helper. |
| Limitations | The buildtree artifact is diagnostic and does not contain the final kernel Mach-O. The run stopped before the independent final-Mach-O symbol audit, ARM64 artifact verification, RAM-only image packaging, manifest/checksum verification or physical testing. No physical RPM/GLINK/QUP behavior is established. |

### Follow-up CI correction

- Workflow repair: [`d72ff5025bc66907ce3fb306bae672e90ba769d2`](https://github.com/NAANG360/hackios-lavender-xnu/commit/d72ff5025bc66907ce3fb306bae672e90ba769d2).
- Replaced the invalid raw-transcript symbol grep with assertions against the pre-link verifier's explicit PASS output.
- Added an independent final-kernel `nm` gate requiring exactly one strong global text definition of `_lavender_rpm_vote_vdd_dig_low_svs` in the ARM64 Mach-O.
- Await the workflow triggered by this repair before treating final-link inclusion or packaging as accepted evidence.

## 22 November 2026 — initial iterator repair source review (superseded by Run #470)

| Field | Value |
|---|---|
| Date | 2026-11-22 |
| Commit | `6c52daeb43a9663feb133adf076e0596b2324d10` |
| CI run | None observed for this commit at checkpoint |
| Artifact | None |
| SHA-256 | Not applicable |
| Tests | Independent unified-diff hunk count; XNU 8796.141.3 API/source inspection |
| Result | Partial source validation |
| Evidence | Patch hunks match declared old/new counts; API confirms `createNubs()` consumes `OSIterator *`; collection iterator factory is available |
| Limitations | No host tests on this exact commit, no fresh staged-tree application, no XNU compile/link, no image, no physical test |

## 21 November 2026 — Run #469 failure

| Field | Value |
|---|---|
| Date | 2026-11-21 |
| Commit | `7abd0cf29074da5a34dee77da58fd022b6912534` |
| CI run | [Run #469 / 44936263614](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44936263614) |
| Artifact | `xnu-build-log` ID `14720735281`; `xnu-buildtree` ID `14720276362` (diagnostic only) |
| SHA-256 | Log ZIP `c356cdb989468cc820f3a1f6ed9923108ac7e607dc52bcb260d7ad5409b758f9`; buildtree ZIP `e1a551f5a272b42ef73fcfb5291738ba983d3d0b4ea56b6285150b3e5b447a45` |
| Tests | QUP protocol host regression; RPM/GLINK protocol host regression; XNU 8796 build |
| Result | Failed |
| Evidence | Both host regressions passed. XNU compile failed in `LavenderPlatformExpert.cpp:86`: `OSArray *` cannot initialize `OSIterator *` parameter. Package job skipped. |
| Limitations | No final ARM64 kernel link, no RPM object link verification, no `iosboot.img`, no physical RPM/QUP operation. Diagnostic artifacts are not bootable. |

## 20 November 2026 — latest accepted image baseline

| Field | Value |
|---|---|
| Date | 2026-11-20 |
| Commit | `7a15aaad2d649f570ffe3f9ad23d1a98f659085d` |
| CI run | [Run #400 / 44706182280](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/44706182280) |
| Artifact | `lavender-fastboot-boot-test`, ID `14615937969` |
| SHA-256 | `f0239f08c402d0a97e95ceddbee10cf61ec7c76d94258619716d038518c84ce2` |
| Tests | XNU 8796 build, QUP host protocol, QUP source/ADT/IOKit guards, linked kernel verification, RAM-only image packaging and payload identity |
| Result | Passed build and package acceptance |
| Evidence | `iosboot.img` 16,621,568 bytes; manifest and checksum recorded in AGENTS.md / Chapter 05 |
| Limitations | QUP physical transaction not proven; RPM/GLINK not included in this accepted build; no eMMC/touch/display/Darwin userspace operation established; no checksum-bound physical test |

## 22 November 2026 — Run #471 in progress on workflow repair

| Field | Value |
|---|---|
| Date | 2026-11-22 |
| Commit | `d72ff5025bc66907ce3fb306bae672e90ba769d2` |
| CI run | [Run #471 / 45028377026](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45028377026) |
| Artifact | Pending |
| SHA-256 | Pending |
| Tests | Lavender ADT topology; QUP host protocol regression; RPM/GLINK host protocol regression; XNU 8796 compilation; pre-link RPM object/symbol check; final Mach-O verification |
| Result | In progress |
| Evidence | ADT validation, both host protocol suites, profile validation, dependency setup and cache preparation passed. XNU 8796 compilation is running. |
| Limitations | No final-link result, final ARM64 Mach-O symbol audit, kernel artifact, RAM-only image, manifest/SHA-256 or physical test is confirmed yet. Do not package or boot until the run succeeds and all independent checks pass. |

## Physical safety status

- No physical test of the current RPM iterator repair has been performed.
- No Android/Lucid ROM file, DTB, boot image, recovery image, vendor file, partition table, or partition has been modified.
- Future handset tests remain RAM-only via `fastboot boot iosboot.img`, after complete image acceptance.
- Preserve `cpus=1` and the independent volume-key recovery route.
- Never use flashing, erase, format, repartition, or partition-write operations.

## Acceptance rules

A future evidence row must include exact source commit, CI run, artifact ID/name, image size, full SHA-256, tests, result, evidence, and limitations. Do not promote a host test to physical evidence or an image package to a successful boot.
