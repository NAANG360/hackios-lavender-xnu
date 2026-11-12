#!/usr/bin/env python3
"""Create the Android boot-image v2 format used by the Lavender fastboot test.

This intentionally supports only legacy boot header v2. It does not implement
vendor_boot, GKI signatures, AVB signing, or boot header v3/v4.
"""
import argparse
import hashlib
import struct
from pathlib import Path

MAGIC = b"ANDROID!"
HEADER_V1_SIZE = 1648
HEADER_V2_SIZE = 1660


def encode_os_version(version: str, patch_level: str) -> int:
    major, minor, patch = (int(x) for x in version.split("."))
    year, month = (int(x) for x in patch_level.split("-"))
    if not (0 <= major <= 127 and 0 <= minor <= 127 and 0 <= patch <= 127):
        raise ValueError("Android version components must be in 0..127")
    if not (2000 <= year <= 2127 and 1 <= month <= 12):
        raise ValueError("patch level must be YYYY-MM, year 2000..2127, month 01..12")
    os_version = (major << 14) | (minor << 7) | patch
    os_patch_level = ((year - 2000) << 4) | month
    return (os_version << 11) | os_patch_level


def image_id(parts) -> bytes:
    digest = hashlib.sha1()
    for part in parts:
        if part is None:
            digest.update(struct.pack("<I", 0))
        else:
            digest.update(part)
            digest.update(struct.pack("<I", len(part)))
    return digest.digest().ljust(32, b"\0")


def pad_to_page(out: bytearray, page_size: int) -> None:
    out.extend(b"\0" * ((-len(out)) % page_size))


def build(args) -> int:
    kernel = Path(args.kernel).read_bytes()
    ramdisk = Path(args.ramdisk).read_bytes()
    dtb = Path(args.dtb).read_bytes()
    if not kernel or not ramdisk or not dtb:
        raise ValueError("kernel, ramdisk, and DTB must all be non-empty")
    if args.header_version != 2:
        raise ValueError("only Android boot header version 2 is supported")
    if args.pagesize <= 0 or args.pagesize & (args.pagesize - 1):
        raise ValueError("page size must be a positive power of two")

    os_field = encode_os_version(args.os_version, args.os_patch_level)
    cmd = args.cmdline.encode("ascii", "strict")
    if len(cmd) > 1536:
        raise ValueError("cmdline exceeds 1536 bytes")
    cmdline, extra_cmdline = cmd[:512], cmd[512:]

    # AOSP mkbootimg's boot image ID covers kernel, ramdisk, second,
    # recovery_dtbo, and DTB, each followed by its little-endian size.
    ident = image_id((kernel, ramdisk, None, None, dtb))
    out = bytearray()
    out += struct.pack(
        "<8s10I",
        MAGIC,
        len(kernel), args.base + args.kernel_offset,
        len(ramdisk), args.base + args.ramdisk_offset,
        0, args.base + args.second_offset,
        args.base + args.tags_offset,
        args.pagesize, args.header_version, os_field,
    )
    out += struct.pack("<16s512s32s1024s",
                       args.board.encode("ascii")[:16],
                       cmdline, ident, extra_cmdline)
    # Header v1 fields: no recovery_dtbo, followed by the total v2 header size.
    out += struct.pack("<IQI", 0, 0, HEADER_V2_SIZE)
    # Header v2 fields: DTB byte size and its physical load address.
    out += struct.pack("<IQ", len(dtb), args.base + args.dtb_offset)
    if len(out) != HEADER_V2_SIZE:
        raise AssertionError(f"header is {len(out)} bytes, expected {HEADER_V2_SIZE}")

    pad_to_page(out, args.pagesize)
    for part in (kernel, ramdisk, dtb):
        out.extend(part)
        pad_to_page(out, args.pagesize)

    Path(args.output).write_bytes(out)
    print(f"Created {args.output}: {len(out)} bytes, header v2, "
          f"OS field 0x{os_field:08x}, DTB {len(dtb)} bytes")
    return len(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--header_version", type=int, default=2)
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--ramdisk", required=True)
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--base", type=lambda x: int(x, 0), default=0)
    parser.add_argument("--kernel_offset", type=lambda x: int(x, 0), default=0x8000)
    parser.add_argument("--ramdisk_offset", type=lambda x: int(x, 0), default=0x1000000)
    parser.add_argument("--second_offset", type=lambda x: int(x, 0), default=0)
    parser.add_argument("--tags_offset", type=lambda x: int(x, 0), default=0x100)
    parser.add_argument("--dtb_offset", type=lambda x: int(x, 0), default=0x1f00000)
    parser.add_argument("--pagesize", type=lambda x: int(x, 0), default=4096)
    parser.add_argument("--os_version", default="15.0.0")
    parser.add_argument("--os_patch_level", default="2025-03")
    parser.add_argument("--board", default="lavender")
    parser.add_argument("--cmdline", default="console=null")
    parser.add_argument("-o", "--output", required=True)
    args = parser.parse_args()
    build(args)


if __name__ == "__main__":
    main()
