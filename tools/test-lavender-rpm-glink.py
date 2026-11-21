#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
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
