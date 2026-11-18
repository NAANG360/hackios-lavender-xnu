#!/usr/bin/env python3
"""Guard Lavender's IOKit platform match and early initialization checkpoints."""
from pathlib import Path
import re
import sys


def require(condition, message):
    if not condition:
        raise SystemExit("FAIL: " + message)


def ordered_numbers(source, numbers, context):
    positions = []
    for number in numbers:
        match = re.search(rf"LAVENDER_IOKIT_TRACE\(\s*{number}\s*\)", source)
        require(match is not None, f"{context}: checkpoint {number} missing")
        positions.append(match.start())
    require(positions == sorted(positions), f"{context}: checkpoints are out of order")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {Path(sys.argv[0]).name} XNU_SOURCE_ROOT")
    root = Path(sys.argv[1])

    config = (root / "iokit/KernelConfigTables.cpp").read_text()
    start = config.index("'IOClass'         = LavenderPlatformExpert;")
    end = config.index("},", start)
    personality = config[start:end]
    require("'IOProviderClass' = IOPlatformExpertDevice;" in personality,
            "Lavender platform expert provider class changed")
    require("'IONameMatch'     = 'xiaomi,lavender';" in personality,
            "Lavender platform expert must match generated ADT root compatible xiaomi,lavender")
    require("qcom,sdm660-mtp" not in personality,
            "stale MTP-only Lavender platform match remains")

    pe = (root / "pexpert/arm/pe_init.c").read_text()
    ordered_numbers(pe, [107, 108, 109, 110, 111, 112, 113, 114, 125],
                    "PE_init_iokit")
    require(pe.index("LAVENDER_IOKIT_TRACE(114)") < pe.index("InitIOKit(PE_state.deviceTreeHead)"),
            "InitIOKit begin marker must precede InitIOKit call")
    require(pe.index("ConfigureIOKit();") < pe.index("LAVENDER_IOKIT_TRACE(125)"),
            "ConfigureIOKit return marker must follow the call")

    iokit = (root / "iokit/Kernel/IOStartIOKit.cpp").read_text()
    ordered_numbers(iokit, [115, 116, 117, 118, 119, 120, 121, 122, 123],
                    "InitIOKit")
    init_start = iokit.index("InitIOKit(void *dtTop)")
    init_end = iokit.index("void\nConfigureIOKit(void)", init_start)
    init_body = iokit[init_start:init_end]
    require(init_body.index("LAVENDER_IOKIT_TRACE(115)") < init_body.index("IOLibInit();") <
            init_body.index("LAVENDER_IOKIT_TRACE(116)"),
            "IOLibInit markers do not bracket the call")
    require("LAVENDER_IOKIT_TRACE(123);" in init_body,
            "InitIOKit completion marker is missing")
    config_start = iokit.index("ConfigureIOKit(void)")
    config_body = iokit[config_start:iokit.index("void\nStartIOKitMatching", config_start)]
    require(config_body.index("LAVENDER_IOKIT_TRACE(124)") < config_body.index("configureDefaults();"),
            "ConfigureIOKit entry marker must precede configureDefaults")

    early = (root / "osfmk/arm64/lavender_early.c").read_text()
    trace_start = early.index("lavender_early_trace_startup(unsigned int event")
    trace_end = early.index("lavender_early_logo(void)", trace_start)
    trace = early[trace_start:trace_end]
    for event in range(107, 126):
        require(re.search(rf'case {event}:.*?"[^"\n]+"', trace) is not None,
                f"trace renderer case {event} is missing")
    require("event >= 47 && event <= 106" in trace,
            "existing POST-PLATFORM event range must remain unchanged")
    print("PASS: Lavender IOKit personality matches the generated ADT; PE/IOKit checkpoints 107–125 are present and ordered")


if __name__ == "__main__":
    main()
