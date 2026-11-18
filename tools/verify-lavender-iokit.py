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

    dt_support = (root / "iokit/Kernel/IODeviceTreeSupport.cpp").read_text()
    alloc_start = dt_support.index("IODeviceTreeAlloc( void * dtTop )")
    alloc_end = dt_support.index("\n}\n#endif", alloc_start)
    alloc_body = dt_support[alloc_start:alloc_end]
    ordered_numbers(
        alloc_body, list(range(133, 147)), "IODeviceTreeAlloc",
        macro="LAVENDER_DEVICE_TREE_TRACE"
    )
    require(
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(137") <
        alloc_body.index("MakeReferenceTable((DTEntry)dtTop") <
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(138"),
        "root DeviceTree conversion markers do not bracket MakeReferenceTable"
    )
    require(
        alloc_body.index("SecureDTInitEntryIterator((DTEntry)dtTop") <
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(140"),
        "DeviceTree iterator-ready checkpoint must follow iterator initialization"
    )
    require(
        "if ((lavenderNodeIndex % 4) == 0 || lavenderNodeIndex >= 18)" in alloc_body and
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(141") <
        alloc_body.index("child = MakeReferenceTable( dtChild"),
        "bounded per-child progress checkpoint is missing or occurs after child conversion"
    )
    require(
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(142") <
        alloc_body.index("parent->attachToParent( IORegistryEntry::getRegistryRoot()") <
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(143"),
        "DeviceTree walk/registry attach checkpoints are out of order"
    )
    require(
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(144") <
        alloc_body.index("while ((child = regIter->getNextObject()))") <
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(145") <
        alloc_body.index("LAVENDER_DEVICE_TREE_TRACE(146") <
        alloc_body.index("return parent;"),
        "interrupt-map walk and allocator return checkpoints are out of order"
    )

    early = (root / "osfmk/arm64/lavender_early.c").read_text()
    require("static unsigned int lavender_guard_stage20_complete;" in early,
            "late-progress Guardian stage-20 state is missing")
    stage20_store = early.index("*early_guard_cell = (stage == 20) ? (0x80000100u | stage) : (0x100u | stage);")
    stage20_done = early.index("if (stage == 20) lavender_guard_stage20_complete = 1;", stage20_store)
    trace_publish = early.index("*early_guard_cell = 0x80010000u | (event & 0xffffu);")
    trace_fn = early.index("lavender_early_trace_startup(unsigned int event")
    require(stage20_store < stage20_done < trace_fn < trace_publish,
            "late Guardian event publication must begin only after stage 20")
    require('dc cvac, %0' in early[trace_publish:trace_publish + 220],
            "late Guardian event marker must be cleaned to shared memory")

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
        133: "DT ALLOC ENTER dt=",
        134: "DT PLANE READY ptr=",
        135: "DT SYMBOLS READY name=",
        136: "DT GLOBALS READY resolvers=",
        137: "DT ROOT BUILD BEGIN dt=",
        138: "DT ROOT BUILD DONE entry=",
        139: "DT STACK READY ptr=",
        140: "DT ITERATOR READY root=",
        141: "DT CHILD BUILD n=",
        142: "DT WALK DONE nodes=",
        143: "DT ROOT ATTACHED ptr=",
        144: "DT INTERRUPT WALK READY iter=",
        145: "DT INTERRUPT WALK DONE map=",
        146: "DT ALLOC RETURN root=",
    }
    for event in range(133, 147):
        require(re.search(rf'case {event}:.*?"[^"\\n]+"', trace) is not None,
                f"trace renderer case {event} is missing")
    for event, label in expected.items():
        case = re.search(rf'case {event}:(.*?)(?=\n\tcase |\n\tdefault:)', trace, re.S)
        require(case is not None and f'"{label}"' in case.group(1),
                f"trace renderer event {event} must display {label!r}")
    require("event >= 47 && event <= 106" in trace,
            "existing POST-PLATFORM event range must remain unchanged")
    print("PASS: Lavender IOKit platform match, InitIOKit boundaries, root-nub init branches, and trace labels 126–132")
    

if __name__ == "__main__":
    main()
