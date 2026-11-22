# LIOS Validation and Physical Bring-up Evidence Ledger

This ledger records only observed evidence. Build, package, RAM boot, and subsystem operation are separate acceptance levels.

## 22 November 2026 — Run #470 in progress on iterator repair

| Field | Value |
|---|---|
| Date | 2026-11-22 |
| Commit | `6c52daeb43a9663feb133adf076e0596b2324d10` |
| CI run | [Run #470 / 45018828868](https://github.com/NAANG360/hackios-lavender-xnu/actions/runs/45018828868) |
| Artifact | None available at checkpoint |
| SHA-256 | Not applicable |
| Tests | QUP host protocol regression; RPM/GLINK host protocol regression; patch application; staged-source verifier; XNU 8796 compilation |
| Result | In progress |
| Evidence | Both host regressions passed; patch application and source verifier passed; XNU build step is still running |
| Limitations | Final link, pre-link object verification, Mach-O verification, image packaging, checksum and physical test not yet confirmed |

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

## Physical safety status

- No physical test of the current RPM iterator repair has been performed.
- No Android/Lucid ROM file, DTB, boot image, recovery image, vendor file, partition table, or partition has been modified.
- Future handset tests remain RAM-only via `fastboot boot iosboot.img`, after complete image acceptance.
- Preserve `cpus=1` and the independent volume-key recovery route.
- Never use flashing, erase, format, repartition, or partition-write operations.

## Acceptance rules

A future evidence row must include exact source commit, CI run, artifact ID/name, image size, full SHA-256, tests, result, evidence, and limitations. Do not promote a host test to physical evidence or an image package to a successful boot.
