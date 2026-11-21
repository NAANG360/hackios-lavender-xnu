#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
patch = root / "xnu" / "patches" / "0130-lavender-rpm-glink-v0.patch"
lines = patch.read_text().splitlines()
import re
for i, line in enumerate(lines):
    if not line.startswith("@@ "):
        continue
    match = re.match(r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", line)
    if not match:
        raise SystemExit(f"Malformed unified-diff hunk header at {patch}:{i+1}: {line}")
    old_expected = int(match.group(2) or "1")
    new_expected = int(match.group(4) or "1")
    old_seen = new_seen = 0
    for body in lines[i+1:]:
        if body.startswith("@@ ") or body.startswith("diff --git "):
            break
        if body.startswith(" ") or body.startswith("-"):
            old_seen += 1
        if body.startswith(" ") or body.startswith("+"):
            new_seen += 1
        if body.startswith("\\"):
            continue
        if not body.startswith((" ", "+", "-")):
            raise SystemExit(f"Invalid unified-diff body line at {patch}:{i+2}: {body!r}")
    if (old_seen, new_seen) != (old_expected, new_expected):
        raise SystemExit(f"Hunk count mismatch at {patch}:{i+1}: declared {old_expected}/{new_expected}, actual {old_seen}/{new_seen}")
print("Lavender RPM/GLINK patch hunk accounting passed")
source = root / "tests" / "test_lavender_rpm_glink.cpp"
header = root / "xnu" / "patches" / "support" / "LavenderRPMGLINK.h"
with tempfile.TemporaryDirectory(prefix="lavender-rpm-glink-") as directory:
    executable = Path(directory) / "rpm-glink-test"
    subprocess.run([
        "c++", "-std=c++14", "-Wall", "-Wextra", "-Werror",
        "-I", str(header.parent), str(source), "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)
print("Lavender SDM660 RPM/GLINK protocol tests passed")
