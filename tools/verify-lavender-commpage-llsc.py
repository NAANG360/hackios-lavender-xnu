#!/usr/bin/env python3
"""Verify Lavender commpage queue locks use ARMv8.0 LL/SC in source and Mach-O code."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def source_symbol_body(source: str, symbol: str) -> str:
    """Return one global assembly symbol body, stopping at the next global label."""
    label = re.compile(rf"(?m)^{re.escape(symbol)}:[ \t]*$")
    match = label.search(source)
    if match is None:
        raise ValueError(f"missing required commpage source symbol: {symbol}")

    tail = source[match.end():]
    next_symbol = re.search(r"(?m)^_[A-Za-z0-9_.$]+:[ \t]*(?://.*)?$", tail)
    body = tail[:next_symbol.start()] if next_symbol else tail
    # Strip comments before checking instructions so prose cannot satisfy a guard.
    return "\n".join(line.split("//", 1)[0] for line in body.splitlines())


def disassembly_symbol_body(disassembly: str, symbol: str) -> str:
    """Return an llvm-objdump symbol body, tolerating both symbol heading formats."""
    lines = disassembly.splitlines()

    def is_heading(line: str, wanted: str | None = None) -> bool:
        value = line.strip()
        if wanted is not None:
            return value == f"{wanted}:" or value.endswith(f"<{wanted}>:")
        return (
            re.fullmatch(r"_[A-Za-z0-9_.$]+:", value) is not None
            or re.search(r"<_[A-Za-z0-9_.$]+>:$", value) is not None
        )

    start = next((i for i, line in enumerate(lines) if is_heading(line, symbol)), None)
    if start is None:
        raise ValueError(f"missing linked Mach-O disassembly symbol: {symbol}")

    end = next((i for i in range(start + 1, len(lines)) if is_heading(lines[i])), len(lines))
    return "\n".join(lines[start + 1:end])


def decoded_mnemonics(body: str) -> list[str]:
    """Decode mnemonics from llvm-objdump lines with four ARM64 instruction bytes."""
    mnemonics: list[str] = []
    for line in body.splitlines():
        if ":" not in line:
            continue
        words = line.split(":", 1)[1].split()
        if len(words) < 5:
            continue
        if not all(re.fullmatch(r"[0-9a-fA-F]{2}", word) for word in words[:4]):
            continue
        mnemonics.append(words[4].lower())
    return mnemonics


def require_instruction(body: str, pattern: str, description: str) -> None:
    if re.search(pattern, body, flags=re.IGNORECASE | re.MULTILINE) is None:
        raise ValueError(f"missing expected instruction/control-flow: {description}")


def validate_source_function(source: str, name: str, address_reg: str) -> None:
    stem = name.removeprefix("_pfz_trylock_and_")
    body = source_symbol_body(source, name)

    if re.search(r"\bcasa\b", body, flags=re.IGNORECASE):
        raise ValueError(f"{name} still contains an LSE CASA instruction")

    checks = [
        (rf"^[ \t]*Ltrylock_{stem}_lock:[ \t]*$", "exclusive-retry label"),
        (rf"^[ \t]*ldaxr[ \t]+w10,[ \t]*\[{address_reg}\]", "acquire exclusive load"),
        (rf"^[ \t]*cbnz[ \t]+w10,[ \t]*Ltrylock_{stem}_failed\b", "busy-lock failure branch"),
        (rf"^[ \t]*stxr[ \t]+w12,[ \t]*w11,[ \t]*\[{address_reg}\]", "exclusive store of locked value"),
        (rf"^[ \t]*cbnz[ \t]+w12,[ \t]*Ltrylock_{stem}_lock\b", "retry after failed exclusive store"),
        (rf"^[ \t]*b[ \t]+Ltrylock_{stem}_success\b", "success branch"),
        (rf"^[ \t]*Ltrylock_{stem}_failed:[ \t]*$", "busy-lock failure label"),
        (r"^[ \t]*mov[ \t]+x0,[ \t]*#-1\b", "existing failure return value"),
        (rf"^[ \t]*b[ \t]+Ltrylock_{stem}_exit\b", "existing failure exit"),
    ]
    for pattern, description in checks:
        require_instruction(body, pattern, f"{name}: {description}")

    # Clear the exclusive monitor on the contended/busy-lock return path only.
    failed_label = f"Ltrylock_{stem}_failed:"
    exit_label = f"Ltrylock_{stem}_exit:"
    if failed_label not in body or exit_label not in body:
        raise ValueError(f"{name}: cannot isolate the busy-lock return path")
    failed_path = body.split(failed_label, 1)[1].split(exit_label, 1)[0]
    require_instruction(
        failed_path,
        r"^[ \t]*clrex[ \t]*$",
        f"{name}: CLREX on the busy-lock return path",
    )


def validate_disassembly_function(disassembly: str, name: str) -> None:
    body = disassembly_symbol_body(disassembly, name)
    mnemonics = decoded_mnemonics(body)
    if not mnemonics:
        raise ValueError(f"could not decode instructions for linked symbol {name}")

    forbidden = {"casa", "cas", "casal", "casl", "casp", "swp", "ldadd", "stadd", "ldclr", "ldeor", "ldset"}
    present = sorted(forbidden.intersection(mnemonics))
    if present:
        raise ValueError(f"linked {name} still contains LSE atomic instruction(s): {present}")
    if "ldaxr" not in mnemonics or "stxr" not in mnemonics:
        raise ValueError(
            f"linked {name} is missing expected ARMv8.0 LDAXR/STXR; found {sorted(set(mnemonics))}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path,
                        help="patched XNU commpage_asm.s source file")
    parser.add_argument("--disassembly", type=Path,
                        help="llvm-objdump ARM64 disassembly of the linked kernel")
    args = parser.parse_args()

    if args.source is None and args.disassembly is None:
        parser.error("provide --source, --disassembly, or both")

    try:
        if args.source is not None:
            source = args.source.read_text(encoding="utf-8")
            validate_source_function(source, "_pfz_trylock_and_enqueue", "x3")
            validate_source_function(source, "_pfz_trylock_and_dequeue", "x2")
            print("PASS: commpage source uses both ARMv8.0 LDAXR/STXR retry loops; no CASA remains")

        if args.disassembly is not None:
            disassembly = args.disassembly.read_text(encoding="utf-8", errors="replace")
            validate_disassembly_function(disassembly, "_pfz_trylock_and_enqueue")
            validate_disassembly_function(disassembly, "_pfz_trylock_and_dequeue")
            print("PASS: linked Mach-O contains ARMv8.0 LDAXR/STXR in both commpage helpers; no LSE atomics")
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"FAIL: Lavender commpage ARMv8.0 LL/SC verification: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
