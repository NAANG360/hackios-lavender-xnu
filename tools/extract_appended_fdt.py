#!/usr/bin/env python3
"""Extract one appended Qualcomm FDT from an Android ARM64 kernel image.

Lavender's stock boot image is header v0: the DTB is appended to the kernel,
not stored in a boot-header-v2 DTB field. The live boot command line reports
androidboot.dtb_idx=2, so the default selected table entry is index 2.
"""
import argparse
import hashlib
import struct
from pathlib import Path

MAGIC = b"\xd0\x0d\xfe\xed"


def align4(value):
    return (value + 3) & ~3


def parse_fdt(blob, offset):
    if offset + 40 > len(blob) or blob[offset:offset + 4] != MAGIC:
        return None
    try:
        (magic, total, off_struct, off_strings, off_rsv, version,
         last_comp, boot_cpu, size_strings, size_struct) = struct.unpack_from(">10I", blob, offset)
    except struct.error:
        return None
    if magic != 0xd00dfeed or version < 17 or total < 40 or offset + total > len(blob):
        return None
    if not (40 <= off_struct < total and 40 <= off_strings < total):
        return None
    if off_struct + size_struct > total or off_strings + size_strings > total:
        return None

    strings = blob[offset + off_strings:offset + off_strings + size_strings]
    p = offset + off_struct
    end = p + size_struct
    depth = 0
    root_seen = False
    ended = False
    root = {}

    while p + 4 <= end:
        token = struct.unpack_from(">I", blob, p)[0]
        p += 4
        if token == 1:  # FDT_BEGIN_NODE
            nul = blob.find(b"\0", p, end)
            if nul < 0:
                return None
            if depth == 0:
                root_seen = True
            depth += 1
            p = align4(nul + 1)
        elif token == 2:  # FDT_END_NODE
            depth -= 1
            if depth < 0:
                return None
        elif token == 3:  # FDT_PROP
            if p + 8 > end:
                return None
            length, nameoff = struct.unpack_from(">II", blob, p)
            value_start = p + 8
            value_end = value_start + length
            if value_end > end or nameoff >= len(strings):
                return None
            name_end = strings.find(b"\0", nameoff)
            if name_end < 0:
                return None
            name = strings[nameoff:name_end].decode("ascii", "replace")
            if depth == 1:
                root[name] = blob[value_start:value_end]
            p = align4(value_end)
        elif token == 4:  # FDT_NOP
            continue
        elif token == 9:  # FDT_END
            ended = True
            break
        else:
            return None

    if not root_seen or not ended:
        return None
    compatible = [x.decode("ascii", "replace") for x in root.get("compatible", b"").split(b"\0") if x]
    if not compatible:
        return None
    model = root.get("model", b"").split(b"\0", 1)[0].decode("utf-8", "replace")
    return {
        "offset": offset,
        "size": total,
        "version": version,
        "model": model,
        "compatible": compatible,
        "msm_id": root.get("qcom,msm-id", b"").hex(),
        "board_id": root.get("qcom,board-id", b"").hex(),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kernel", type=Path)
    parser.add_argument("--index", type=int, default=2, help="zero-based DTB table index (default: 2)")
    parser.add_argument("--compatible", default="qcom,sdm660", help="required root compatible string")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    blob = args.kernel.read_bytes()
    candidates = []
    seen = set()
    pos = 0
    while True:
        offset = blob.find(MAGIC, pos)
        if offset < 0:
            break
        pos = offset + 4
        if offset in seen:
            continue
        seen.add(offset)
        candidate = parse_fdt(blob, offset)
        if candidate:
            candidates.append(candidate)

    candidates.sort(key=lambda item: item["offset"])
    for index, item in enumerate(candidates):
        print(
            f"DTB[{index}] offset=0x{item['offset']:x} size={item['size']} "
            f"model={item['model']!r} compatible={','.join(item['compatible'])}"
        )
    if args.index < 0 or args.index >= len(candidates):
        raise SystemExit(f"requested DTB index {args.index}; found {len(candidates)} valid FDTs")
    selected = candidates[args.index]
    if args.compatible not in selected["compatible"]:
        raise SystemExit(
            f"DTB[{args.index}] does not match {args.compatible!r}: {selected['compatible']}"
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob[selected["offset"]:selected["offset"] + selected["size"]])
    digest = hashlib.sha256(args.output.read_bytes()).hexdigest()
    print(f"SELECTED DTB[{args.index}] -> {args.output} ({selected['size']} bytes, sha256={digest})")


if __name__ == "__main__":
    main()
