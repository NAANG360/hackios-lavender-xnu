#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ ! -f "iokit/Kernel/arm/SweetPlatformExpert.cpp" ] || [ ! -f "iokit/conf/files.arm64" ]; then
  echo "Deferring Lavender native IOKit staging until after the base XNU patches"
  exit 0
fi
for source in LavenderQUPI2C.cpp LavenderQUPI2C.h LavenderRPMGLINK.cpp LavenderRPMGLINK.h; do
  test -s "$SCRIPT_DIR/support/$source"
  cp "$SCRIPT_DIR/support/$source" "iokit/Kernel/arm/$source"
done
python3 - <<'PY'
from pathlib import Path

expert = Path("iokit/Kernel/arm/SweetPlatformExpert.cpp")
files = Path("iokit/conf/files.arm64")
if not expert.is_file() or not files.is_file():
    raise SystemExit("SweetPlatformExpert/files.arm64 missing")

s = expert.read_text()

anchor = "#include <IOKit/IOLib.h>"
add = """#include <IOKit/IOLib.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSCollectionIterator.h>
#if defined(ARM64_BOARD_CONFIG_LAVENDER)
extern "C" void lavender_register_rpm_glink_driver(void);
extern "C" void lavender_register_qup_i2c_driver(void);
#endif"""
if "lavender_register_rpm_glink_driver" not in s:
    if anchor not in s:
        raise SystemExit("SweetPlatformExpert include anchor changed")
    s = s.replace(anchor, add, 1)

anchor = "\tif (!IODTPlatformExpert::start(provider)) {\n\t\treturn false;\n\t}\n"
add = anchor + """#if defined(ARM64_BOARD_CONFIG_LAVENDER)
\t/* Register power transport before QUP; both drivers remain boot-arg gated (lavenderrpm/lavenderqup). */
\tlavender_register_rpm_glink_driver();
\tlavender_register_qup_i2c_driver();
#endif
"""
if "lavender_register_rpm_glink_driver();" not in s:
    if anchor not in s:
        raise SystemExit("SweetPlatformExpert start anchor changed")
    s = s.replace(anchor, add, 1)

# NOTE: 0046-sweet-iosurface-iomfb.patch already publishes /arm-io children via
# `IORegistryEntry *armio = root->childFromPath("arm-io", gIODTPlane);`.
# Do NOT redeclare `armio` here; publish only /soc/rpm-glink for RPM GLINK.
anchor = "\tcreateNubs(this, IODTFindMatchingEntries(root, kIODTExclusive, excludeList()));\n"
add = anchor + """#if defined(ARM64_BOARD_CONFIG_LAVENDER)
\t/* RPM GLINK is a root /soc child, not an /arm-io device. */
\t{
\t\tIORegistryEntry *soc = root->childFromPath("soc", gIODTPlane);
\t\tif (soc != NULL) {
\t\t\tIORegistryEntry *rpmGlink = soc->childFromPath("rpm-glink", gIODTPlane);
\t\t\tif (rpmGlink != NULL) {
\t\t\t\tOSArray *rpmNubs = OSArray::withObjects((const OSObject **)&rpmGlink, 1, 1);
\t\t\t\tif (rpmNubs != NULL) {
\t\t\t\t\tOSIterator *rpmIterator = OSCollectionIterator::withCollection(rpmNubs);
\t\t\t\t\trpmNubs->release();
\t\t\t\t\tif (rpmIterator != NULL) createNubs(this, rpmIterator);
\t\t\t\t}
\t\t\t\trpmGlink->release();
\t\t\t}
\t\t\tsoc->release();
\t\t}
\t}
#endif
"""
if 'childFromPath("rpm-glink"' not in s:
    if anchor not in s:
        raise SystemExit("SweetPlatformExpert nub anchor changed")
    s = s.replace(anchor, add, 1)

expert.write_text(s)

lines = files.read_text().splitlines()
anchor_index = next(
    (
        i for i, entry in enumerate(lines)
        if entry.strip().startswith("iokit/Kernel/arm/SweetPlatformExpert.cpp")
        and "optional iokitcpp" in entry
    ),
    None,
)
if anchor_index is None:
    raise SystemExit("files.arm64 SweetPlatformExpert entry changed")

for line in (
    "iokit/Kernel/arm/LavenderRPMGLINK.cpp optional iokitcpp",
    "iokit/Kernel/arm/LavenderQUPI2C.cpp optional iokitcpp",
):
    if line not in lines:
        lines.insert(anchor_index + 1, line)
        anchor_index += 1

files.write_text("\n".join(lines) + "\n")
print("staged Lavender native IOKit drivers cleanly (no duplicate armio)")
PY
