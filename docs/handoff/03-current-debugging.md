# Chapter 3 — Current debugging state (read first for engineering work)

**Last live inspection:** 2026-11-15. **Branch:** `ci/lavender-machine-profile`. **Branch HEAD at compaction start:** `e8c2e2303d765835e95fb79f3f15e711f3f82020` (documentation-only prior update). Re-resolve HEAD before edits.

## Latest Actions evidence
- Run #231, ID `43677707091`, attempt 1, source `abff246071db828dab395bfa2d98587bf86b2474`: failed in official Mach-O verifier. Build and source guard passed; package job skipped.
- Run #230, ID `43677616958`, source `235d71e71dc834a6bda7289aa33b764d278a093e`: XNU build, verifier and package succeeded.
- Run #229, ID `43677604405`: success.
- Latest run list and branch HEAD are mutable; always query live GitHub before acting.

## Exact #231 blocker
The verifier rejects file-backed non-executable metadata section `__CTF,__ctf` because its segment has `vmsize=0`. This section is non-loaded metadata, not kernel text.
- Segment: `fileoff=0xb9c000`, `filesize=0x9c000`, `vmsize=0`, non-executable (`initprot=1`).
- Section: file offset `0xb9c000`, size `0x9837a`.
- Error: `section __CTF,__ctf VM range exceeds segment '__CTF'`.
- Retained candidate artifact ID `14147107920`; ZIP digest `dd28d7ad2a9f592a071e6ec7f9fc34eb3a5fcec4798804e67c89040276a0ad13`.
- Extracted kernel: 16,587,304 bytes; SHA-256 `b2d0abef3e20282d24945083cf530ce82c42bbfaeec2f1fa796d3aa0a8e6c1b3`.

Independent inspection of that exact candidate found executable sections `__TEXT_EXEC,__text`, `__TEXT_EXEC,__hib_text`, `__TEXT_EXEC,__commpage_text`, `__KLD,__text`. LLVM disassembly confirms:
- Stage 8 at file offset `0x47a8c0`, VM `0xfffffe000747e8c0`: `add x0,x20,#1,lsl #12; mov w1,#8; str w1,[x0]; dc cvac,x0; dsb sy`.
- Stage 9 at file offset `0x47a94c`, VM `0xfffffe000747e94c`: same sequence with `mov w1,#9`.

This is independent candidate evidence, **not** an official verifier pass. #231 produced no accepted `xnu-kernel` artifact and no new `iosboot.img`.

## Required next actions
1. Re-resolve branch HEAD and newest Actions runs.
2. Inspect `tools/verify-lavender-kernel.py` and fix narrowly:
   - Keep strict header/load-command/section-table/file-range bounds.
   - Permit a file-backed section in a `vmsize == 0` segment only for non-executable metadata, and only if its file range lies within both segment and file bounds.
   - Keep executable sections fail-closed: mapped executable segment, VM containment and file containment required.
   - Emit complete section inventory, including non-mapped metadata.
3. Test locally against retained #231 kernel/ZIP before CI. Print stage-8/9 segment, section, file offsets, VM addresses, words and decoded instructions. Add negative tests for malformed executable ranges and file offsets.
4. Improve failure diagnostics in `xnu.yml`: preserve raw kernel, load commands/section inventory, verifier report, SHA-256 and full `__TEXT_EXEC,__text` disassembly (explicit section selector), even on failure.
5. Make one corrected commit; run one XNU CI build (no overlapping builds). Confirm real build path, verifier pass and cache save.
6. Manually dispatch package workflow with that successful run ID. Independently verify exact `iosboot.img`, kernel identity, package structure, load ranges and hash.
7. No handset test until power-button-independent recovery is established. Then RAM-only boot; record guardian marker, console/framebuffer, C-entry and logo observations.

**Do not restart from XNU source or weaken verifier guards.** The immediate issue is this metadata-vs-VM-range parser assumption.
