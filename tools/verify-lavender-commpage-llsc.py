#!/usr/bin/env python3
"""Fail the Lavender build unless both commpage queue locks use ARMv8.0 LL/SC."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

DEFAULT_SOURCE = Path("xnu/osfmk/arm/commpage/commpage_asm.s")


def symbol_body(source: str, symbol: str) -> str:
    """Return one global assembly symbol body, stopping at the next global label."""
    label = re.compile(rf"(?m)^{re.escape(symbol)}:[ \t]*$")
    match = label.search(source)
    if match is None:
        raise ValueError(f"missing required commpage symbol: {symbol}")

    tail = source[match.end():]
    next_symbol = re.search(r"(?m)^_[A-Za-z0-9_.$]+:[ \t]*(?://.*)?$", tail)
    body = tail[:next_symbol.start()] if next_symbol else tail
    # Strip comments before checking instructions so prose cannot satisfy a guard.
    return "\n".join(line.split("//", 1)[0] for line in body.splitlines())


def require_instruction(body: str, pattern: str, description: str) -> None:
    if re.search(pattern, body, flags=re.IGNORECASE | re.MULTILINE) is None:
        raise ValueError(f"missing expected instruction/control-flow: {description}")


def validate_function(source: str, name: str, address_reg: str) -> None:
    stem = name.removeprefix("_pfz_trylock_and_")
    body = symbol_body(source, name)

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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help=f"patched XNU assembly source (default: {DEFAULT_SOURCE})",
    )
    args = parser.parse_args()

    try:
        source = args.source.read_text(encoding="utf-8")
        validate_function(source, "_pfz_trylock_and_enqueue", "x3")
        validate_function(source, "_pfz_trylock_and_dequeue", "x2")
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"FAIL: Lavender commpage ARMv8.0 LL/SC guard: {exc}", file=sys.stderr)
        return 1

    print("PASS: both Lavender commpage queue locks use LDAXR/STXR retry loops; no CASA remains")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
