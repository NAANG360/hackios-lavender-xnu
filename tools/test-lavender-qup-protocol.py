#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
src=root/"tests"/"test_lavender_qup_protocol.cpp"
hdr=root/"xnu"/"patches"/"support"/"LavenderQUPI2C.h"
with tempfile.TemporaryDirectory(prefix="lavender-qup-") as d:
 exe=Path(d)/"qup-test"
 subprocess.run(["c++","-std=c++14","-Wall","-Wextra","-Werror","-I",str(hdr.parent),str(src),"-o",str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print("Lavender QUP-v2 protocol tests passed")
