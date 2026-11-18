#!/usr/bin/env python3
"""Guard Lavender's IOKit platform match and root-nub initialization trace."""
from pathlib import Path
import re
import sys


def require(condition, message):
    if not condition:
        raise SystemExit("FAIL: " + message)


def ordered_numbers(source, numbers, context, macro="LAVENDER_IOKIT_TRACE"):
    positions = []
    for number in numbers:
        match = re.search(rf"{macro}\(\s*{number}\s*(?:,\s*[^)]*)?\)", source)
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
    require(init_body.index("gRootNub = new IOPlatformExpertDevice;") <
            init_body.index("LAVENDER_IOKIT_TRACE(120)") <
            init_body.index("gRootNub->init(dtTop);") <
            init_body.index("LAVENDER_IOKIT_TRACE(121)"),
            "root-nub allocation/init checkpoints do not bracket the calls")
    require("LAVENDER_IOKIT_TRACE(123);" in init_body,
            "InitIOKit completion marker is missing")
    config_start = iokit.index("ConfigureIOKit(void)")
    config_body = iokit[config_start:iokit.index("void\nStartIOKitMatching", config_start)]
    require(config_body.index("LAVENDER_IOKIT_TRACE(124)") < config_body.index("configureDefaults();"),
            "ConfigureIOKit entry marker must precede configureDefaults")

    root_expert = (root / "iokit/Kernel/IOPlatformExpert.cpp").read_text()
    root_start = root_expert.index("IOPlatformExpertDevice::init(void *dtRoot)")
    root_end = root_expert.index("IOPlatformExpertDevice::startIOServiceMatching(void)", root_start)
    root_body = root_expert[root_start:root_end]
    ordered_numbers(root_body, [126, 127, 128, 129, 130, 131, 132],
                    "IOPlatformExpertDevice::init", macro="LAVENDER_ROOT_INIT_TRACE")
    require(root_body.index("LAVENDER_ROOT_INIT_TRACE(126") <
            root_body.index("IODeviceTreeAlloc(dtRoot)") <
            root_body.index("LAVENDER_ROOT_INIT_TRACE(128"),
            "device-tree allocation markers do not bracket IODeviceTreeAlloc")
    require(root_body.index("LAVENDER_ROOT_INIT_TRACE(129") <
            root_body.index("super::init( dt, gIODTPlane )") <
            root_body.index("LAVENDER_ROOT_INIT_TRACE(130"),
            "device-tree registry superclass markers do not bracket init")
    require(root_body.index("LAVENDER_ROOT_INIT_TRACE(131") <
            root_body.index("ok = super::init()") <
            root_body.index("LAVENDER_ROOT_INIT_TRACE(132"),
            "fallback superclass markers do not bracket init")
    require("IOWorkLoop::workLoop()" not in root_body and "registerService()" not in root_body,
            "service matching/workloop setup must remain in the later lockdown path")

    early = (root / "osfmk/arm64/lavender_early.c").read_text()
    trace_start = early.index("lavender_early_trace_startup(unsigned int event")
    trace_end = early.index("lavender_early_logo(void)", trace_start)
    trace = early[trace_start:trace_end]
    for event in range(107, 126):
        require(re.search(rf'case {event}:.*?"[^"\n]+"', trace) is not None,
                f"trace renderer case {event} is missing")
    expected = {
        126: "IOKIT ROOT NUB INIT ENTER dtRoot=",
        127: "IOKIT DEVICE TREE ALLOC BEGIN dtRoot=",
        128: "IOKIT DEVICE TREE ALLOC RETURNED dt=",
        129: "IOKIT ROOT REGISTRY INIT BEGIN dt=",
        130: "IOKIT ROOT REGISTRY INIT RETURNED ok=",
        131: "IOKIT ROOT REGISTRY FALLBACK BEGIN",
        132: "IOKIT ROOT REGISTRY FALLBACK RETURNED ok=",
    }
    for event, label in expected.items():
        case = re.search(rf'case {event}:(.*?)(?=\n\tcase |\n\tdefault:)', trace, re.S)
        require(case is not None and f'"{label}"' in case.group(1),
                f"trace renderer event {event} must display {label!r}")
    require("event >= 47 && event <= 106" in trace,
            "existing POST-PLATFORM event range must remain unchanged")
    print("PASS: Lavender IOKit platform match, InitIOKit boundaries, root-nub init branches, and trace labels 126–132")
    

if __name__ == "__main__":
    main()
