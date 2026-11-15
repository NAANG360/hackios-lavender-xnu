#!/usr/bin/env python3
"""Validate a Lavender XNU ARM64 Mach-O and inspect compiled bootstrap markers."""
from __future__ import annotations

import struct
import sys
from dataclasses import dataclass
from pathlib import Path

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPE_ARM64 = 0x0100000C
MH_EXECUTE = 2
LC_SEGMENT_64 = 0x19
VM_PROT_EXECUTE = 0x4
S_ATTR_PURE_INSTRUCTIONS = 0x80000000
S_ATTR_SOME_INSTRUCTIONS = 0x00000400
SECTION_TYPE = 0xFF
S_ZEROFILL = 0x1
S_GB_ZEROFILL = 0xC
S_THREAD_LOCAL_ZEROFILL = 0x12
HEADER_SIZE = 32
SEGMENT_COMMAND_SIZE = 72
SECTION_SIZE = 80

# Expected checkpoint publication sequence from the Lavender start.S macro.
# The verifier reports actual compiled words and both file/VM offsets.
COMMON_WORDS = (0x91400680, 0xB9000001, 0xD50B7A20, 0xD5033F9F)
STAGE_MOV = {8: 0x52800101, 9: 0x52800121}


@dataclass
class ExecutableSection:
    segment: str
    section: str
    vmaddr: int
    size: int
    fileoff: int
    flags: int
    initprot: int
    data: bytes


def c_name(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("ascii", errors="replace")


def parse_macho(data: bytes) -> tuple[int, int, list[ExecutableSection], list[str]]:
    if len(data) < HEADER_SIZE:
        raise ValueError("kernel is shorter than a Mach-O 64-bit header")

    magic, cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags, reserved = (
        struct.unpack_from("<IiiIIIII", data, 0)
    )
    if magic != MH_MAGIC_64:
        raise ValueError(f"unexpected Mach-O magic: 0x{magic:08x}")
    if cputype != CPU_TYPE_ARM64:
        raise ValueError(f"kernel is not ARM64 (cputype=0x{cputype:08x})")
    if filetype != MH_EXECUTE:
        raise ValueError(f"expected MH_EXECUTE (filetype={filetype})")

    commands_start = HEADER_SIZE
    commands_end = commands_start + sizeofcmds
    if commands_end > len(data):
        raise ValueError(
            f"load-command region exceeds file: end=0x{commands_end:x}, file=0x{len(data):x}"
        )

    cursor = commands_start
    sections: list[ExecutableSection] = []
    inventory: list[str] = []

    for command_index in range(ncmds):
        if cursor + 8 > commands_end:
            raise ValueError(f"load command {command_index} header exceeds load-command region")
        cmd, cmdsize = struct.unpack_from("<II", data, cursor)
        if cmdsize < 8:
            raise ValueError(f"load command {command_index} has invalid cmdsize={cmdsize}")
        command_end = cursor + cmdsize
        if command_end > commands_end:
            raise ValueError(f"load command {command_index} exceeds load-command region")

        if cmd == LC_SEGMENT_64:
            if cmdsize < SEGMENT_COMMAND_SIZE:
                raise ValueError(f"LC_SEGMENT_64 command {command_index} is shorter than 72 bytes")
            segname = c_name(data[cursor + 8:cursor + 24])
            vmaddr, vmsize, fileoff, filesize = struct.unpack_from("<QQQQ", data, cursor + 24)
            maxprot, initprot, nsects, segflags = struct.unpack_from("<iiII", data, cursor + 56)

            expected_cmdsize = SEGMENT_COMMAND_SIZE + nsects * SECTION_SIZE
            if expected_cmdsize > cmdsize:
                raise ValueError(
                    f"segment {segname!r} declares {nsects} sections but cmdsize={cmdsize}"
                )
            if fileoff > len(data) or filesize > len(data) - fileoff:
                raise ValueError(
                    f"segment {segname!r} file range exceeds kernel: "
                    f"off=0x{fileoff:x} size=0x{filesize:x}"
                )

            inventory.append(
                f"SEGMENT {segname} vm=0x{vmaddr:x}+0x{vmsize:x} "
                f"file=0x{fileoff:x}+0x{filesize:x} "
                f"maxprot=0x{maxprot:x} initprot=0x{initprot:x} nsects={nsects}"
            )

            section_cursor = cursor + SEGMENT_COMMAND_SIZE
            for section_index in range(nsects):
                if section_cursor + SECTION_SIZE > command_end:
                    raise ValueError(
                        f"section {section_index} of segment {segname!r} exceeds its command"
                    )
                sectname = c_name(data[section_cursor:section_cursor + 16])
                section_segname = c_name(data[section_cursor + 16:section_cursor + 32])
                addr, size, offset, align, reloff, nreloc, section_flags = (
                    struct.unpack_from("<QQIIIII", data, section_cursor + 32)
                )

                inventory.append(
                    f"  SECTION {section_segname},{sectname} "
                    f"addr=0x{addr:x} size=0x{size:x} fileoff=0x{offset:x} "
                    f"flags=0x{section_flags:08x} align={align} nreloc={nreloc}"
                )

                section_type = section_flags & SECTION_TYPE
                file_backed = section_type not in (
                    S_ZEROFILL, S_GB_ZEROFILL, S_THREAD_LOCAL_ZEROFILL
                )
                instruction_section = bool(
                    section_flags & (S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS)
                )
                executable_segment = bool(initprot & VM_PROT_EXECUTE)
                text_named = sectname in ("__text", "__text_exec", "__TEXT_EXEC")

                if size > 0:
                    if addr < vmaddr or addr > vmaddr + vmsize or size > vmaddr + vmsize - addr:
                        raise ValueError(
                            f"section {section_segname},{sectname} VM range exceeds segment "
                            f"{segname!r}: addr=0x{addr:x} size=0x{size:x}"
                        )
                    if file_backed:
                        if offset > len(data) or size > len(data) - offset:
                            raise ValueError(
                                f"section {section_segname},{sectname} file range exceeds kernel: "
                                f"off=0x{offset:x} size=0x{size:x}"
                            )
                        if (
                            offset < fileoff
                            or offset > fileoff + filesize
                            or size > fileoff + filesize - offset
                        ):
                            raise ValueError(
                                f"section {section_segname},{sectname} file range exceeds "
                                f"segment {segname!r}: off=0x{offset:x} size=0x{size:x}"
                            )

                if (
                    size > 0
                    and file_backed
                    and section_segname == segname
                    and (executable_segment or instruction_section)
                    and (instruction_section or text_named)
                ):
                    sections.append(
                        ExecutableSection(
                            segname, sectname, addr, size, offset, section_flags,
                            initprot, data[offset:offset + size]
                        )
                    )

                section_cursor += SECTION_SIZE

        cursor = command_end

    if cursor != commands_end:
        raise ValueError(
            f"load commands consumed 0x{cursor - commands_start:x} bytes, "
            f"header declares {sizeofcmds}"
        )

    return ncmds, flags, sections, inventory


def find_pattern(section: ExecutableSection, mov_imm: int) -> list[tuple[int, int, tuple[int, ...]]]:
    pattern = (COMMON_WORDS[0], mov_imm, *COMMON_WORDS[1:])
    usable = len(section.data) // 4
    if usable < len(pattern):
        return []
    words = struct.unpack("<" + "I" * usable, section.data[:usable * 4])
    found = []
    for index in range(usable - len(pattern) + 1):
        if tuple(words[index:index + len(pattern)]) == pattern:
            file_offset = section.fileoff + index * 4
            vm_address = section.vmaddr + index * 4
            found.append((file_offset, vm_address, pattern))
    return found


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {Path(sys.argv[0]).name} KERNEL")
    path = Path(sys.argv[1])
    data = path.read_bytes()
    required = (b"LavenderPlatformExpert", b"qcom,sdm660-mtp", b"Redmi Note 7")
    missing = [item.decode("ascii") for item in required if item not in data]
    if missing:
        raise SystemExit("missing Lavender identity strings: " + ", ".join(missing))

    ncmds, flags, sections, inventory = parse_macho(data)
    print("Mach-O load-command / section inventory:")
    for item in inventory:
        print(item)
    if not sections:
        raise SystemExit(
            "no valid non-empty executable instruction/text sections were discovered"
        )

    print("Executable text candidates:")
    for section in sections:
        print(
            f"  {section.segment},{section.section}: "
            f"vm=0x{section.vmaddr:x} file=0x{section.fileoff:x} "
            f"size=0x{section.size:x} flags=0x{section.flags:08x} "
            f"initprot=0x{section.initprot:x}"
        )

    for stage, mov_imm in STAGE_MOV.items():
        matches = []
        for section in sections:
            for file_offset, vm_address, words in find_pattern(section, mov_imm):
                matches.append((section, file_offset, vm_address, words))
        if not matches:
            raise SystemExit(
                f"compiled ARM64 executable sections contain no verified stage-{stage} "
                f"publication sequence (expected MOV=0x{mov_imm:08x})"
            )
        for section, file_offset, vm_address, words in matches:
            encoded = " ".join(f"{word:08x}" for word in words)
            decoded = (
                "ADD X0, X20, #1; "
                f"MOV W1, #{stage}; "
                "STR W1, [X0]; DC CVAC, X0; DSB SY"
            )
            print(
                f"PASS stage-{stage}: {section.segment},{section.section} "
                f"file+0x{file_offset:x} vm=0x{vm_address:x} "
                f"instructions=[{encoded}] decoded=[{decoded}]"
            )

    print(
        f"PASS: Mach-O 64-bit ARM64 executable; size={len(data)} bytes; "
        f"ncmds={ncmds}; flags=0x{flags:08x}; "
        "Lavender identity strings and compiled stage-8/stage-9 sequences present"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, struct.error) as exc:
        raise SystemExit(f"invalid Mach-O: {exc}")
