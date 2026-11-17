#!/usr/bin/env python3
"""Guard the Lavender ADT -> GICv3 -> XNU virtual-timer IRQ contract."""
from pathlib import Path
import re
import sys


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def extract_branch(source: str, marker: str, end: str) -> str:
    require(marker in source, f"missing source branch {marker}")
    part = source.split(marker, 1)[1]
    if end:
        require(end in part, f"missing branch terminator {end}")
        part = part.split(end, 1)[0]
    return part


repo = Path(sys.argv[2]) if len(sys.argv) > 2 else Path.cwd()
root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("dxb/xnu")
pe = (root / "pexpert/arm/pe_fiq.c").read_text()
sleh = (root / "osfmk/arm64/sleh.c").read_text()
protos = (root / "pexpert/pexpert/arm/protos.h").read_text()
adt_builder = (repo / "tools/mkadt-lavender.py").read_text()
workflow = (repo / ".github/workflows/xnu.yml").read_text()

# The generated ADT is the source of truth for the PPI number. SDM660's
# architected virtual timer is PPI index 3, which maps to INTID 16 + 3 = 19.
match = re.search(r"^VTIMER_INTID\s*=\s*(\d+)", adt_builder, re.M)
require(match is not None, "mkadt-lavender.py lacks VTIMER_INTID")
vtimer = int(match.group(1))
require(vtimer == 19, f"unexpected SDM660 virtual-timer INTID: {vtimer}")
require("('virtual-timer-intid', u32(VTIMER_INTID))" in adt_builder,
        "generated /arm-io/gic does not publish virtual-timer-intid")

# The kernel reads the timer PPI from the same GIC node and rejects malformed
# or non-PPI values rather than quietly falling back to an unrelated INTID.
require("SECURITY_READ_ONLY_LATE(uint32_t) lavender_vtimer_intid = 19;" in pe,
        "Lavender virtual timer storage/default missing")
require('SecureDTGetProperty(entry, "virtual-timer-intid", &timer_prop, &timer_size)' in pe,
        "Lavender GIC path does not read the ADT timer INTID")
require("if (intid < 16 || intid >= 32)" in pe,
        "Lavender GIC path does not validate that timer INTID is a PPI")
require("lavender_vtimer_intid = intid;" in pe,
        "Lavender GIC path does not use the ADT-selected timer INTID")

# Restrict these checks to the Lavender #elif branch, preserving Sweet and
# generic ARM64 behavior. Lavender runs at non-secure EL1 under the shim.
ppi_anchor = pe.index("gicr_write32(GICR_ISENABLER0, (1U << sweet_vtimer_intid)")
ppi_lavender = pe.index("#elif defined(ARM64_BOARD_CONFIG_LAVENDER)", ppi_anchor)
ppi_end = pe.index("#else\n\t// Configure timers and legacy FIQ to be group 0", ppi_lavender)
fiq_branch = pe[ppi_lavender:ppi_end]
dist_anchor = pe.index("/* Non-secure view: ARE_NS is bit 4;")
dist_lavender = pe.rfind("#elif defined(ARM64_BOARD_CONFIG_LAVENDER)", 0, dist_anchor)
dist_end = pe.index("#else\n\t// Preserve the legacy Group-0 path for other generic board profiles.", dist_anchor)
dist_branch = pe[dist_lavender:dist_end]
bpr_marker = "#if defined(SWEET) || defined(ARM64_BOARD_CONFIG_LAVENDER)"
bpr_branch = extract_branch(pe, bpr_marker, "#else")
fiq_branch += "\n" + dist_branch + "\n" + bpr_branch

for token in (
    "igroupr = gicr_read32(GICR_IGROUPR0)",
    "igroupr |= (1U << lavender_vtimer_intid)",
    "gicr_write32(GICR_IGROUPR0, igroupr)",
    "GICR_IPRIORITYR0 + lavender_vtimer_intid",
    "gicr_write32(GICR_ISENABLER0, (1U << lavender_vtimer_intid))",
    "ICC_BPR1_EL1",
    "gicd_ctlr |= GICD_CTLR_ENABLEGRP1A;",
    "GICD_CTLR_ARE_NS",
    "GICD_CTLR_ENABLEGRP1",
    "GICD_CTLR_DS",
    "gicd_ctlr |= GICD_CTLR_ARE_NS;",
    "ICC_IGRPEN1_EL1",
    "GICD_CTLR_RWP",
):
    require(token in fiq_branch, f"Lavender GIC branch missing {token}")

for forbidden in (
    "GICD_CTLR_ENABLEGRP0",
    "ICC_IGRPEN0_EL1",
    "ICC_BPR0_EL1",
    "1 << 27",
    "SWEET_IPI_SGI",
):
    require(forbidden not in fiq_branch,
            f"Lavender GIC branch still contains incompatible Group-0/legacy setup: {forbidden}")

require("GICD_CTLR_DS" in fiq_branch and
        "single-state interrupt groups are active" in fiq_branch,
        "Lavender must distinguish the DS=1 single-security-state view")
require("cannot enable affinity routing while legacy Group-1 NS is active" in fiq_branch,
        "Lavender must refuse a DS=0 ARE_NS transition while legacy EnableGrp1NS is active")
require("GICD_CTLR_ENABLEGRP1A | GICD_CTLR_ENABLEGRP1" in fiq_branch,
        "Lavender DS=1 transition must require both Group 0 and Group 1 disabled")
require("GICD_CTLR.ARE_NS did not stick" in fiq_branch,
        "Lavender must verify the ARE_NS transition before enabling Group 1")
require(fiq_branch.index("gicd_ctlr |= GICD_CTLR_ARE_NS;") <
        fiq_branch.index("gicd_ctlr |= GICD_CTLR_ENABLEGRP1A;"),
        "Lavender must enable affinity routing before enabling Group 1")
require("non-secure Group-1 distributor enable did not stick" in fiq_branch,
        "Lavender must verify distributor Group-1 enable")
require("GICR_IGROUPR0 is Secure and RAZ/WI to Non-secure EL1" in fiq_branch,
        "Lavender must document the DS=0 firmware group-assignment precondition")
require("GICR_WAKER is Secure and reads" in " ".join(pe.split()) and
        "as RAZ/WI to Non-secure EL1" in " ".join(pe.split()),
        "Lavender must document the DS=0 redistributor-wake precondition")
waker_start = pe.index("LAVENDER_FIQ_TRACE(96, gicr_pe_base + GICR_WAKER")
waker_end = pe.index("LAVENDER_FIQ_TRACE(98", waker_start)
waker_code = pe[waker_start:waker_end]
require(waker_code.index("if (gicr_waker & GICR_WAKER_CHILDRENASLEEP)") <
        waker_code.index("gicr_write32(GICR_WAKER, gicr_waker)"),
        "Lavender WAKER write must remain conditional on ChildrenAsleep")
require("igroupr |= (1U << lavender_vtimer_intid)" in fiq_branch and
        "gicr_write32(GICR_IGROUPR0, igroupr)" in fiq_branch,
        "Lavender DS=1 path must configure only the selected timer PPI")
require("timed out waiting for GIC distributor RWP" in fiq_branch,
        "Lavender distributor write-pending wait is not bounded")
require('"ICC_PMR_EL1", 0xFF' in pe, "Lavender must set the CPU priority mask")
require("LAVENDER_FIQ_TRACE(104" in pe and "LAVENDER_FIQ_TRACE(105" in pe,
        "Lavender final GIC setup markers 104/105 are missing")
require("GICD_CTLR_ENABLEGRP1A | GICD_CTLR_ENABLEGRP1" not in fiq_branch,
        "non-secure Group-1 setup must not set the RES0 Group1 bit when ARE_NS is enabled")
require("Bit 0 is RES0 in this view" in pe,
        "non-secure GICD_CTLR bit aliasing is not documented")
require("GIC system-register interface did not enable" in pe,
        "Lavender must verify ICC_SRE_EL1.SRE")
require("split EOI mode remained enabled" in pe,
        "Lavender must verify combined EOI/deactivate mode")

# IRQ delivery and acknowledgement must match Group-1 configuration.
irq = sleh.split("sleh_irq(arm_saved_state_t *state)", 1)[1]
irq = irq.split("\nvoid\nsleh_fiq", 1)[0]
lavender_irq = extract_branch(
    irq,
    "#elif defined(ARM64_BOARD_CONFIG_LAVENDER)",
    "#else\n\tsleh_interrupt_handler_prologue(state, DBG_INTR_TYPE_OTHER);",
)
for token in (
    "ICC_IAR1_EL1",
    "lavender_vtimer_intid",
    "rtclock_intr(TRUE)",
    "ICC_EOIR1_EL1",
    "cdp->cpu_decrementer = -1;",
):
    require(token in lavender_irq, f"Lavender IRQ handler missing {token}")
for forbidden in ("ICC_IAR0_EL1", "ICC_EOIR0_EL1", "cpu_signal_handler()"):
    require(forbidden not in lavender_irq,
            f"unexpected Group-0 or SMP path in single-core Lavender IRQ handler: {forbidden}")
require("extern uint32_t lavender_vtimer_intid;" in protos,
        "pexpert/arm/protos.h does not declare the Lavender timer INTID")

# Keep the milestone single-core until secondary startup and IPI routing are proven.
require("cpus=1" in workflow,
        "package command line no longer constrains Lavender bring-up to one CPU")
require("event >= 47 && event <= 106" in
        ((repo / "tools/split-lavender-early.py").read_text()),
        "generated Lavender trace renderer does not include events 104-105")

print(f"PASS: Lavender timer PPI {vtimer} has a DS-aware Group-1NS route and firmware preconditions; single-core gate retained")
