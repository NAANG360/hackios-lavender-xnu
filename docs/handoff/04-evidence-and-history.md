# Chapter 4 — Verified evidence and historical traps

## Last fully verified image: Run #230
- Run #230 ID `43677616958`; source `235d71e71dc834a6bda7289aa33b764d278a093e`.
- Kernel artifact `14147131789`; 16,587,304 bytes; SHA-256 `a979c798d2b206b8023ca7a798f325e2424a5a4f0786f64a815c2f30c9eef333`.
- Official ARM64 Mach-O/MH_EXECUTE and Lavender identity checks passed. Stage 8 at `0x47a8c0`, stage 9 at `0x47a94c`; LLVM disassembly confirmed both actual instruction sequences.
- Package artifact `14147915512` contains exact `iosboot.img`; 16,531,456 bytes; SHA-256 `aed49f435687ec05d970f8f350ee0e3bd83d2a362d9166e5e505e5298a770f06`.
- Shim 155,648 bytes; LAVENDRX payload 15,959,424 bytes; DTB 406,950 bytes. Load ranges non-overlapping; DTB page-aligned. Payload SHA-256 `66fa3107124d38d9285530f8036c56acaa411971020c262a1ee6ce1f5160a783`; ADT SHA-256 `eaf3c608195bca3a647aaea85ba7ae0b22073520ddb694e41e14a21072000281`.
- Optional `/product` node omitted because `research/ipsw-x/d22ap.adt` was absent; package still passed.
- Not physically tested. This proves compile + verifier + structural package only, not boot, Apple logo or Darwin.
- #230 diagnostic bundle's disassembly selected empty `__TEXT,__text`; explicitly select `__TEXT_EXEC,__text`.

## Earlier physical observations (Run #203-era image only)
User videos showed A53 boot core MPIDR 0 handing off to A73/Kryo 260 Gold MPIDR `0x100`; PSCI CPU_ON returned 0 and secondary core was observed. A53 guardian displayed `A53 LAST MARK=0x00000007`, indicating assembly stage 7. No C entry marker or Apple logo was observed. Do not project this behavior onto newer images.

## Repeated failure causes — avoid regressions
- Earlier IOKit personality config-table failure (`KernelConfigTables.cpp:93`) came from Lavender string insertion/patch placement; current build passes source guard.
- #220: patch 0107 skipped due to pre-0106 context; rebase and required-patch guard caught it.
- #224: legacy cache contained stale completed kernel and pinned build skipped make install; versioned v2 cache/invalidation addresses this. Confirm logs.
- #227/#228: verifier hard-coded `__TEXT_EXEC,__text`; NUL-name parser used literal backslash-zero instead of NUL byte. Commits `2a64ff651e0f98a0a09a9ea3a35b0a50c564c6b5` and `235d71e71dc834a6bda7289aa33b764d278a093e` corrected those issues.
- #231: verifier over-applied VM containment to non-executable, file-backed `__CTF` metadata. Fix this narrow parser defect; don't change XNU sources.

## Truth statement
XNU has compiled repeatedly. #230 is a verified kernel and structurally verified RAM-only image, not a physical boot. #231's candidate independently contains stages 8/9 but fails official verification. No Apple logo or Darwin userspace is verified.
