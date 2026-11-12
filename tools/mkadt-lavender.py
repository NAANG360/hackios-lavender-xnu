#!/usr/bin/env python3
"""Build a conservative Apple Device Tree for XNU on Redmi Note 7 (lavender).

This is a separate Lavender profile. It deliberately does not import SWEET's
UFS node, SM7150 GIC redistributor address, 1080x2400 display, or Kryo 470 CPU
topology. It uses one contiguous, reserved-range-free RAM aperture for the
first XNU bring-up; the remaining banks are not advertised to XNU yet.
"""
import argparse
import hashlib
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import adt  # noqa: E402

# From the live /proc/iomem report:
#   a0400000-feabffff System RAM
#   ea948000-fe7fffff reserved
#   fe87c000-feabffff reserved (split subranges)
# Keep XNU's contiguous physBase/memSize below the first reserved carve-out.
PHYS_BASE = 0xA0400000
SAFE_RAM_END = 0xEA948000
MEM_SIZE_TOTAL = SAFE_RAM_END - PHYS_BASE
PANIC_LOG_SIZE = 0x100000

# Public mainline SDM660/Lavender DT references; verify against live vendor DT.
SOC_BASE = 0x01000000
GICD, GICD_SIZE = 0x17A00000, 0x10000
GICR, GICR_SIZE = 0x17B00000, 0x100000
GICR_PE_SIZE = 0x20000
VTIMER_INTID = 19  # live /proc/interrupts: mpm-gic 19 Level arch_timer
TIMEBASE = 19200000

# Public sdm660-xiaomi-lavender.dts simple-framebuffer handoff.
FB, FB_SIZE = 0x9D400000, 0x23FF000
FB_W, FB_H, FB_STRIDE = 1080, 2340, 1080 * 4

# Live boot device: Qualcomm SDHCI v5 eMMC at c0c4000.sdhci.
EMMC, EMMC_SIZE = 0x0C0C4000, 0x1000

# Mainline sdm630.dtsi CPU map: two clusters, four Aff0 values each.
# The boot CPU is represented by logical CPU 0 (MPIDR 0x100) in this ADT.
CPU_MPIDRS = (0x100, 0x101, 0x102, 0x103, 0x000, 0x001, 0x002, 0x003)

BOOT_MANIFEST_HASH = hashlib.sha384(b'lavender boot manifest').digest()
LAVENDER_ECID = 0x4C4156454E000001  # synthetic stable identity; not device data
D22AP_ADT = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         '../research/ipsw-x/d22ap.adt')


def d22ap_product(path=D22AP_ADT):
    """Optionally reuse the real iPhone10,3 /product node for the iOS userland."""
    if not os.path.exists(path):
        print(f'warning: {path} missing; omitting /product node', file=sys.stderr)
        return None
    root, _ = adt.parse(open(path, 'rb').read())
    for child in root.children:
        props = {k: v for k, _, v in child.props}
        if props.get('name') == b'product' + bytes([0]):
            return [(k, v) for k, flags, v in child.props
                    if not flags and k not in ('name', 'AAPL,phandle')]
    raise SystemExit(f'{path}: no /product node')


u32 = lambda *v: struct.pack('<%dI' % len(v), *v)
u64 = lambda *v: struct.pack('<%dQ' % len(v), *v)
cstr = lambda s: s.encode() + bytes([0])


def node(name, props=(), children=()):
    n = adt.Node()
    n.props = [('name', 0, cstr(name))] + [(k, 0, v) for k, v in props]
    n.children = list(children)
    return n


def build(seed=None, product_tree=D22AP_ADT):
    phys_base = PHYS_BASE
    mem_size = MEM_SIZE_TOTAL
    panic_base = phys_base + mem_size - PANIC_LOG_SIZE
    mem_size -= PANIC_LOG_SIZE
    seed = seed if seed is not None else os.urandom(64)
    phandle = iter(range(1, 1000))
    ph = lambda: ('AAPL,phandle', u32(next(phandle)))

    chosen = node('chosen', [
        ph(),
        ('dram-base', u64(phys_base)),
        ('dram-size', u64(mem_size)),
        ('random-seed', seed),
        ('debug-enabled', u32(1)),
        ('embedded-panic-log-size', u32(PANIC_LOG_SIZE)),
        ('firmware-version', cstr('lavender-xnu-shim')),
        ('system-firmware-version', cstr('lavender-xnu-shim')),
        ('boot-manifest-hash', BOOT_MANIFEST_HASH),
        ('crypto-hash-method', cstr('sha2-384')),
        ('display-scale', u32(3)),
        ('unique-chip-id', u64(LAVENDER_ECID)),
        ('chip-id', u32(0x8015)),  # iPhone10,3 userland compatibility identity
        ('certificate-production-status', u32(0)),
        ('effective-production-status-ap', u32(0)),
        ('certificate-security-mode', u32(1)),
        ('effective-security-mode-sep', u32(1)),
    ], [
        node('memory-map', [ph()]),
        node('asmb', [ph(), ('lp-sip0', u64(0x6f)), ('lp-sip1', u32(1))]),
    ])

    cpu_nodes = []
    for logical, mpidr in enumerate(CPU_MPIDRS):
        cluster = (mpidr >> 8) & 0xff
        core = mpidr & 0xff
        cpu_nodes.append(node(f'cpu{logical}', [
            ph(),
            ('device_type', cstr('cpu')),
            ('compatible', cstr('qcom,kryo260') + cstr('ARM,v8')),
            ('reg', u32(mpidr)),
            ('cpu-id', u32(logical)),
            ('logical-cpu-id', u32(logical)),
            ('cluster-id', u32(cluster)),
            ('die-cluster-id', u32(0)),
            ('cluster-core-id', u32(core)),
            ('l2-cache-id', u32(cluster)),
            ('state', cstr('running' if logical == 0 else 'stopped')),
            ('timebase-frequency', u32(TIMEBASE)),
        ]))
    cpus = node('cpus', [ph(), ('#address-cells', u32(1)),
                         ('#size-cells', u32(0))], cpu_nodes)

    gic = node('gic', [
        ph(),
        ('compatible', cstr('arm,gic-v3')),
        ('interrupt-controller', b''),
        ('virtual-timer-intid', u32(VTIMER_INTID)),
        ('reg', u64(GICD - SOC_BASE, GICD_SIZE, GICR - SOC_BASE, GICR_SIZE)),
    ])
    arm_io = node('arm-io', [
        ph(),
        ('device_type', cstr('lavender-io')),
        ('compatible', cstr('lavender-io') + cstr('qcom,sdm660') + cstr('arm-io')),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
        ('ranges', u64(0, SOC_BASE, 0x100000000 - SOC_BASE)),
    ], [
        gic,
        node('wdt', [ph(), ('no-pmu', u32(1))]),
        node('disp0', [
            ph(),
            ('device_type', cstr('display-subsystem')),
            ('compatible', cstr('lavender,display')),
            ('canvas-width', u32(FB_W)),
            ('canvas-height', u32(FB_H)),
        ]),
        node('emmc', [
            ph(),
            ('device_type', cstr('block')),
            ('compatible', cstr('qcom,sdm630-sdhci') + cstr('qcom,sdhci-msm-v5')),
            # arm-io is translated from SOC_BASE; this is the child-relative offset.
            ('reg', u64(EMMC - SOC_BASE, EMMC_SIZE)),
            ('reg-names', cstr('hc_mem')),
            ('bus-width', u32(8)),
            ('non-removable', b''),
        ]),
    ])

    fbcon = node('lavender-fbcon', [
        ph(),
        ('reg', u64(FB, FB_SIZE)),
        ('width', u32(FB_W)),
        ('height', u32(FB_H)),
        ('stride', u32(FB_STRIDE)),
        ('format', cstr('a8r8g8b8')),
    ])
    defaults = node('defaults', [ph(), ('no-effaceable-storage', u32(1))])
    buttons = node('buttons', [
        ph(),
        ('button-names', cstr('volup') + cstr('voldown') + cstr('hold')),
        ('home-button-type', u32(2)),
        ('opposed-power-vol-buttons', u32(1)),
    ])
    pram = node('pram', [ph(), ('reg', u64(panic_base, PANIC_LOG_SIZE))])

    product_props = d22ap_product(product_tree)
    product = [node('product', [ph()] + product_props)] if product_props else []

    return node('device-tree', [
        ph(),
        ('compatible', cstr('D22AP') + cstr('iPhone10,3') +
                      cstr('xiaomi,lavender') + cstr('qcom,sdm660') + cstr('AppleARM')),
        ('model', cstr('iPhone10,3')),
        ('target-type', cstr('D22')),
        ('target-sub-type', cstr('D22AP')),
        ('manufacturer', cstr('Apple Inc.')),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
    ], [chosen, defaults, cpus, arm_io, fbcon, buttons, pram] + product)


def check(blob):
    root, end = adt.parse(blob)
    assert end == len(blob)
    chosen = next(c for c in root.children if c.name == 'chosen')
    get = lambda name: next(v for n, _, v in chosen.props if n == name)
    base = struct.unpack('<Q', get('dram-base'))[0]
    size = struct.unpack('<Q', get('dram-size'))[0]
    assert base == PHYS_BASE
    assert base + size == SAFE_RAM_END - PANIC_LOG_SIZE
    assert base + size == PHYS_BASE + MEM_SIZE_TOTAL - PANIC_LOG_SIZE
    cpus = next(c for c in root.children if c.name == 'cpus')
    regs = [struct.unpack('<I', next(v for n, _, v in c.props if n == 'reg'))[0]
            for c in cpus.children]
    assert tuple(regs) == CPU_MPIDRS
    arm_io = next(c for c in root.children if c.name == 'arm-io')
    gic = next(c for c in arm_io.children if c.name == 'gic')
    reg = struct.unpack('<4Q', next(v for n, _, v in gic.props if n == 'reg'))
    assert reg == (GICD - SOC_BASE, GICD_SIZE, GICR - SOC_BASE, GICR_SIZE)
    fb = next(c for c in root.children if c.name == 'lavender-fbcon')
    fb_reg = struct.unpack('<2Q', next(v for n, _, v in fb.props if n == 'reg'))
    assert fb_reg == (FB, FB_SIZE)
    assert FB + FB_SIZE <= 0xA0000000
    emmc = next(c for c in arm_io.children if c.name == 'emmc')
    emmc_reg = struct.unpack('<2Q', next(v for n, _, v in emmc.props if n == 'reg'))
    assert emmc_reg == (EMMC - SOC_BASE, EMMC_SIZE)
    assert b'qcom,sdhci-msm-v5' in next(v for n, _, v in emmc.props if n == 'compatible')
    return base, size, regs, fb_reg


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--product-tree', default=D22AP_ADT)
    ap.add_argument('--dump', action='store_true')
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    root = build(product_tree=args.product_tree)
    blob = adt.serialize(root)
    if args.check:
        base, size, cpus, fb = check(blob)
        print(f'PASS: RAM 0x{base:x}+0x{size:x}; CPUs {[hex(x) for x in cpus]}; framebuffer {hex(fb[0])}+{hex(fb[1])}')
    open(args.out, 'wb').write(blob)
    if args.dump:
        adt.dump(adt.parse(blob)[0])
    print(f'{args.out}: {len(blob)} bytes', file=sys.stderr)

