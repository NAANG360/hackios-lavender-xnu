#!/usr/bin/env python3
"""Remove the legacy Lavender framebuffer loop before VBAR_EL1 is installed."""
from pathlib import Path

source = Path("xnu/osfmk/arm64/start.s")
text = source.read_text()
entry = text.index("LEXT(start_first_cpu)")
vbar = text.index("MSR_VBAR_EL1_X0", entry)
first = text.index("lavender_early_mark 1", entry, vbar)
first_end = text.index("\n", first) + 1
last = text.index("lavender_early_mark 0x11", first_end, vbar)
last_line = text.rfind("\n", 0, last) + 1
last_end = text.index("\n", last) + 1
text = text[:first_end] + text[last_line:last_end] + text[last_end:]
source.write_text(text)

entry = text.index("LEXT(start_first_cpu)")
vbar = text.index("MSR_VBAR_EL1_X0", entry)
pre_vbar = text[entry:vbar]
if "0x9d400000" in pre_vbar or "Llavender_stage1_pixel" in pre_vbar:
    raise SystemExit("legacy framebuffer loop remains before VBAR_EL1")
if "lavender_early_mark 0x11" not in pre_vbar:
    raise SystemExit("RAM-only pre-VBAR completion marker missing")
print("PASS: removed pre-VBAR framebuffer loop; RAM-only markers retained")
