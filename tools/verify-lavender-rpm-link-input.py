#!/usr/bin/env python3
"""Fail XNU's final link unless RPM object and strong C symbol are link inputs."""
from pathlib import Path
import argparse
import shutil
import subprocess

SYMBOLS = {"_lavender_rpm_vote_vdd_dig_low_svs", "lavender_rpm_vote_vdd_dig_low_svs"}
OBJECT_NAME = "LavenderRPMGLINK.cpo"


def fail(message: str) -> None:
    raise SystemExit("FAIL: " + message)


def read_link_list(path: Path) -> list[Path]:
    if not path.is_file():
        fail(f"link-input file list does not exist: {path}")
    result = []
    for raw in path.read_text(errors="strict").splitlines():
        entry = raw.strip()
        if not entry:
            continue
        candidate = Path(entry)
        if not candidate.is_absolute():
            candidate = Path.cwd() / candidate
        result.append(candidate.resolve())
    return result


def find_nm() -> str:
    xcrun = shutil.which("xcrun")
    if xcrun:
        proc = subprocess.run([xcrun, "-find", "nm"], text=True, capture_output=True)
        if proc.returncode == 0 and proc.stdout.strip():
            return proc.stdout.strip()
    nm = shutil.which("nm")
    if nm:
        return nm
    fail("could not locate nm for strong-symbol verification")


def verify_object(obj: Path, nm: str) -> None:
    if not obj.is_file() or obj.stat().st_size == 0:
        fail(f"RPM object is missing or empty at final-link time: {obj}")
    proc = subprocess.run([nm, "-g", str(obj)], text=True, capture_output=True)
    if proc.returncode != 0:
        fail(f"nm could not inspect final-link RPM object {obj}: {proc.stderr.strip()}")
    definitions = []
    for line in proc.stdout.splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[-1] in SYMBOLS and fields[-2] == "T":
            definitions.append(line)
    if len(definitions) != 1:
        fail(f"expected one strong global text definition of lavender_rpm_vote_vdd_dig_low_svs in {obj}; found {len(definitions)}")


def main() -> None:
    parser = argparse.ArgumentParser()
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--non-lto", action="store_true")
    modes.add_argument("--pre-lto", action="store_true")
    parser.add_argument("filelists", nargs="+", type=Path)
    args = parser.parse_args()
    expected_count = 1 if args.non_lto else 2
    if len(args.filelists) != expected_count:
        fail(f"mode requires {expected_count} link-input file list(s), got {len(args.filelists)}")
    entries = []
    for filelist in args.filelists:
        entries.extend(read_link_list(filelist))
    rpm_objects = [p for p in entries if p.name == OBJECT_NAME]
    if len(rpm_objects) != 1:
        fail(f"expected exactly one {OBJECT_NAME} in aggregate final link input(s); found {len(rpm_objects)}")
    obj = rpm_objects[0]
    verify_object(obj, find_nm())
    mode = "non-LTO" if args.non_lto else "PRE_LTO"
    print(f"PASS: pre-link ({mode}): {obj} is a final kernel link input and defines strong C symbol lavender_rpm_vote_vdd_dig_low_svs")


if __name__ == "__main__":
    main()
