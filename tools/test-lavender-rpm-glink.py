#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
import sys

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

# Exercise the pre-link input verifier with real host objects. These tests prove
# that the linker hook rejects missing, duplicate, and wrong-symbol inputs.
guard = root / "tools" / "verify-lavender-rpm-link-input.py"
injector = root / "tools" / "inject-lavender-rpm-link-guard.py"
with tempfile.TemporaryDirectory(prefix="lavender-rpm-prelink-") as directory:
    tmp = Path(directory)
    good_obj = tmp / "LavenderRPMGLINK.cpo"
    good_src = tmp / "good.cpp"
    good_src.write_text('extern "C" bool lavender_rpm_vote_vdd_dig_low_svs(void) { return true; }\n')
    subprocess.run(["c++", "-std=c++14", "-Wall", "-Wextra", "-Werror", "-c", str(good_src), "-o", str(good_obj)], check=True)
    good_list = tmp / "good.filelist"
    good_list.write_text(str(good_obj.resolve()) + "\n")
    subprocess.run([sys.executable, str(guard), "--non-lto", str(good_list)], check=True)

    bad_src = tmp / "bad.cpp"
    bad_src.write_text('extern "C" bool wrong_vote_symbol(void) { return true; }\n')
    subprocess.run(["c++", "-std=c++14", "-Wall", "-Wextra", "-Werror", "-c", str(bad_src), "-o", str(good_obj)], check=True)
    failed = subprocess.run([sys.executable, str(guard), "--non-lto", str(good_list)], capture_output=True, text=True)
    if failed.returncode == 0 or "strong global text definition" not in failed.stderr:
        raise SystemExit("pre-link guard did not reject an RPM object with the wrong symbol")

    missing_list = tmp / "missing.filelist"
    missing_list.write_text(str(tmp / "missing" / "LavenderRPMGLINK.cpo") + "\n")
    failed = subprocess.run([sys.executable, str(guard), "--non-lto", str(missing_list)], capture_output=True, text=True)
    if failed.returncode == 0 or "missing or empty" not in failed.stderr:
        raise SystemExit("pre-link guard did not reject a missing RPM object")

    duplicate_list = tmp / "duplicate.filelist"
    duplicate_list.write_text(str(good_obj.resolve()) + "\n" + str(good_obj.resolve()) + "\n")
    failed = subprocess.run([sys.executable, str(guard), "--non-lto", str(duplicate_list)], capture_output=True, text=True)
    if failed.returncode == 0 or "found 2" not in failed.stderr:
        raise SystemExit("pre-link guard did not reject duplicate RPM object entries")

    subprocess.run(["c++", "-std=c++14", "-Wall", "-Wextra", "-Werror", "-c", str(good_src), "-o", str(good_obj)], check=True)
    lto_list = tmp / "lto.filelist"
    nonlto_list = tmp / "nonlto.filelist"
    lto_list.write_text("")
    nonlto_list.write_text(str(good_obj.resolve()) + "\n")
    subprocess.run([sys.executable, str(guard), "--pre-lto", str(lto_list), str(nonlto_list)], check=True)

    fixture = tmp / "MakeInc.kernel"
    fixture.write_text(
        '\tprintf \'%s\\n\' "$lto" >ltolink.filelist; \\\n'
        '\tprintf \'%s\\n\' "$nonlto" >nonltolink.filelist\n'
        '\t@$(LOG_LD) "$(@F)"\n'
        '\t$(_v)$(CAT) $(filter %.filelist,$+) < /dev/null > link.filelist\n'
        '\t$(_v)$(LD) $(LDFLAGS_KERNEL) $(LDFLAGS_KERNEL_ONLY) -filelist link.filelist version.o $(filter %.o,$+) -o $@ $(LD_KERNEL_LIBS) $(LD_KERNEL_ARCHIVES)\n'
    )
    subprocess.run([sys.executable, str(injector), str(fixture)], check=True)
    injected = fixture.read_text()
    subprocess.run([sys.executable, str(injector), str(fixture)], check=True)
    if fixture.read_text() != injected:
        raise SystemExit("pre-link hook injector is not idempotent")
    if injected.count('verify-lavender-rpm-link-input.py" --non-lto') != 1 or injected.count('verify-lavender-rpm-link-input.py" --pre-lto') != 1:
        raise SystemExit("pre-link hook injector did not install both hooks exactly once")

print("Lavender SDM660 RPM/GLINK protocol tests passed")
