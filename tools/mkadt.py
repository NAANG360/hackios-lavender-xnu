#!/usr/bin/env python3
"""Build the minimal Apple Device Tree that the LAVENDER/WHYRED xnu needs to boot.

Only what the early boot path actually reads; MMIO addresses come from the
lavender (Redmi Note 7, SDM660) and whyred (Redmi Note 5, SDM636) live FDTs.
Usage:  mkadt.py OUT.adt [--board lavender|whyred] [--product-adt APPLE.adt] [--qemu] [--native]
"""
import hashlib, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import adt  # noqa: E402

# Dual-bank lavender 3GB+ span: lower [0x40000000, 0x85600000) + upper [0xA0400000, 0xE6000000).
# XNU patch 0711 keeps Qualcomm's reserved carve-out [0x85600000, 0xA0400000) unmapped & skipped.
PHYS_BASE = 0x40000000
MEM_SIZE = 0xa6000000  # 2656 MiB span -> 2226 MiB (~2.17 GiB) usable after 430 MiB carve-out hole
# /arm-io children are relative to this; it must not be 0 (serial_init bails)
SOC_BASE = 0x01000000

# interrupt-controller@17a00000 (redistributor-stride 0x20000, 8 cores)
GICD, GICD_SIZE = 0x17a00000, 0x10000
GICR, GICR_SIZE = 0x17b00000, 0x100000
# Memory-mapped architected timer frame in the matching Lavender FDT:
# /soc/timer@17920000, compatible arm,armv7-timer-mem, reg <17920000 1000>.
# Darwin 22's PE map gate requires a device_type=timer node even though its
# ARM64 timebase uses CNTV system registers rather than programming this frame.
TIMER_FRAME, TIMER_FRAME_SIZE = 0x17920000, 0x1000
# Continuous splash simplefb left lit by Qualcomm ABL at the same physical
# address on both boards.
FB = 0x9d400000

# Board facts. SDM660 (lavender) and SDM636 (whyred) are the same Kryo 260
# family: four Cortex-A73 Gold cores (MPIDR 0x100-0x103) with the 16 KiB
# translation granule, GICv3 at 0x17a00000/0x17b00000, SDCC v5 at 0x0c0c4000.
BOARDS = {
    'lavender': {
        'name': 'lavender', 'soc': 'sdm660',
        'panel_w': 1080, 'panel_h': 2340,
        'cpu_hz': 2200000000,
        'ecid': 0x4c4156454e444552,          # 'LAVENDER', synthetic
        'manifest': b'lavender boot manifest',
    },
    'whyred': {
        'name': 'whyred', 'soc': 'sdm636',
        'panel_w': 1080, 'panel_h': 2160,
        'cpu_hz': 1800000000,
        'ecid': 0x5748595245440001,          # 'WHYRED\0\1', synthetic
        'manifest': b'whyred boot manifest',
    },
}
DEFAULT_BOARD = 'lavender'
FB_W, FB_H = BOARDS[DEFAULT_BOARD]['panel_w'], BOARDS[DEFAULT_BOARD]['panel_h']
FB_STRIDE = FB_W * 4
FB_SIZE = FB_W * FB_H * 4
TIMEBASE = 19200000
I2C0, I2C0_SIZE = 0x0C175000, 0x600
I2C0_IRQ_SPEC = (0, 0x5F, 4)
I2C0_INPUT_HZ, I2C0_OUTPUT_HZ = 0x124F800, 0x61A80
TLMM, TLMM_SIZE = 0x03000000, 0xC00000
NVT_I2C_ADDRESS, NVT_IRQ_GPIO, NVT_RESET_GPIO = 0x62, 67, 66
VTIMER_INTID = 19
# QEMU virt (-M virt,gic-version=3 -cpu cortex-a76): GICD/GICR and the generic timer's CNTFRQ
QEMU_GICD, QEMU_GICR, QEMU_TIMEBASE = 0x08000000, 0x080a0000, 62500000
QEMU_VTIMER_INTID = 27

# The iOS userland is the iPhone X (iPhone10,3 / D22AP) IPSW. MobileGestalt
# derives product type, device class, home-button-less UI, artwork etc. from
# the root identity and /product of the real D22AP device tree.
D22AP_ADT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../research/ipsw-x/d22ap.adt')


def d22ap_product(path=D22AP_ADT):
    """/product properties of the real D22AP tree, without the syscfg
    placeholders iBoot would fill in (flag bit set) and the node bookkeeping."""
    if not os.path.exists(path):
        print(f'warning: {path} missing, no /product node', file=sys.stderr)
        return None
    root, _ = adt.parse(open(path, 'rb').read())
    for c in root.children:
        if dict((k, v) for k, f, v in c.props).get('name') == b'product\0':
            return [(k, v) for k, f, v in c.props if not f and k not in ('name', 'AAPL,phandle')]
    raise SystemExit(f'{path}: no /product node')

u32 = lambda *v: struct.pack('<%dI' % len(v), *v)
u64 = lambda *v: struct.pack('<%dQ' % len(v), *v)
cstr = lambda s: s.encode() + b'\0'


def node(name, props=(), children=()):
    n = adt.Node()
    n.props = [('name', 0, cstr(name))] + [(k, 0, v) for k, v in props]
    n.children = list(children)
    return n


def build(seed=None, qemu=False, native=False, board=DEFAULT_BOARD, product_adt=None):
    """qemu=True: same tree, but with QEMU virt's GIC addresses and CNTFRQ."""
    cfg = BOARDS[board]
    fb_w, fb_h, fb_stride = cfg['panel_w'], cfg['panel_h'], cfg['panel_w'] * 4
    fb_size = fb_stride * fb_h
    phys_base, mem_size = PHYS_BASE, MEM_SIZE
    gicd, gicr, timebase = (QEMU_GICD, QEMU_GICR, QEMU_TIMEBASE) if qemu else (GICD, GICR, TIMEBASE)
    seed = seed if seed is not None else os.urandom(64)
    phandle = iter(range(1, 1000))
    ph = lambda: ('AAPL,phandle', u32(next(phandle)))

    chosen = node('chosen', [
        ph(),
        ('dram-base', u64(phys_base)),
        ('dram-size', u64(mem_size)),
        ('random-seed', seed),
        ('debug-enabled', u32(1)),
        ('firmware-version', cstr(cfg['name'] + '-shim')),
        ('system-firmware-version', cstr(cfg['name'] + '-shim')),
        ('boot-manifest-hash', hashlib.sha384(cfg['manifest']).digest()),
        ('crypto-hash-method', cstr('sha2-384')),
        ('display-scale', u32(3)),
        ('unique-chip-id', struct.pack('<Q', cfg['ecid'])),
        ('chip-id', u32(0x8015)),
        ('certificate-production-status', u32(0)),
        ('effective-production-status-ap', u32(0)),
        ('certificate-security-mode', u32(1)),
        ('effective-security-mode-sep', u32(1)),
    ], [node('memory-map', [ph()]),
        node('asmb', [ph(), ('lp-sip0', u64(0x6f)), ('lp-sip1', u32(1))])])

    # LAVENDER SMP: the four Cortex-A73 (Gold) cores, MPIDR 0x100-0x103:
    # the Kryo 260 Silver cores have no 16K granule, so the shim boots xnu
    # on Gold CPU 0x100 and parks the Silver cluster.
    cpu_nodes = []
    for core in range(1 if qemu else 4):
        phys_id = core if qemu else (0x100 | core)
        state = 'running' if phys_id == 0x100 else 'stopped'
        cpu_nodes.append(node(f'cpu{core}', [
            ph(),
            ('device_type', cstr('cpu')),
            ('compatible', cstr('qcom,kryo260') + cstr('qcom,kryo470') + cstr('ARM,v8')),
            ('reg', u32(phys_id)),
            ('cpu-id', u32(core)),
            ('cluster-id', u32(0)),
            ('cluster-type', u32(0)),
            ('die-cluster-id', u32(0)),
            ('cluster-core-id', u32(core)),
            ('l2-cache-id', u32(0)),
            ('l3-cache-id', u32(0)),
            ('state', cstr(state)),
            ('timebase-frequency', u32(timebase)),
            ('clock-frequency', u32(cfg['cpu_hz'])),
            ('bus-frequency', u32(100000000)),
            ('memory-frequency', u32(1866000000)),
            ('peripheral-frequency', u32(timebase)),
            ('fixed-frequency', u32(timebase)),
        ]))
    cpus = node('cpus', [ph(), ('#address-cells', u32(1)),
                         ('#size-cells', u32(0))], cpu_nodes)

    gic_phandle = next(phandle)
    gic = node('gic', [
        ('AAPL,phandle', u32(gic_phandle)),
        ('compatible', cstr('arm,gic-v3')),
        # pexpert/arm/pe_identify_machine.c::pe_arm_map_interrupt_controller
        # finds this exact value; an FDT-style empty boolean does not match.
        ('interrupt-controller', cstr('master')),
        ('#interrupt-cells', u32(3)),
        ('virtual-timer-intid', u32(QEMU_VTIMER_INTID if qemu else VTIMER_INTID)),
        ('reg', u64(gicd - SOC_BASE, GICD_SIZE, gicr - SOC_BASE, GICR_SIZE)),
    ])
    timer_nodes = [] if qemu else [node('timer', [
        ph(),
        ('device_type', cstr('timer')),
        ('compatible', cstr('arm,armv7-timer-mem')),
        ('reg', u64(TIMER_FRAME - SOC_BASE, TIMER_FRAME_SIZE)),
    ])]
    arm_io_children = [gic, *timer_nodes,
        node('wdt', [ph(), ('no-pmu', u32(1))]),
        node('wlan', [
            ph(),
            ('device_type', cstr('wlan')),
            ('compatible', cstr('qcom,wcn3990')),
        ]),
        node('disp0', [
            ph(),
            ('device_type', cstr('display-subsystem')),
            ('compatible', cstr('sweet,display')),
            ('canvas-width', u32(fb_w if native else 1125)),
            ('canvas-height', u32(fb_h if native else 2436)),
        ])]
    if board == 'lavender':
        arm_io_children.extend([
            node('tlmm', [
                ph(),
                ('device_type', cstr('gpio-controller')),
                ('compatible', cstr('qcom,sdm660-pinctrl')),
                ('reg', u64(TLMM - SOC_BASE, TLMM_SIZE)),
            ]),
            node('i2c0', [
                ph(),
                ('device_type', cstr('i2c-controller')),
                ('compatible', cstr('qcom,i2c-msm-v2')),
                ('reg', u64(I2C0 - SOC_BASE, I2C0_SIZE)),
                ('reg-names', cstr('qup_phys_addr')),
                ('interrupts', u32(*I2C0_IRQ_SPEC)),
                ('interrupt-parent', u32(gic_phandle)),
                ('interrupt-names', cstr('qup_irq')),
                ('qcom,clk-freq-in', u32(I2C0_INPUT_HZ)),
                ('qcom,clk-freq-out', u32(I2C0_OUTPUT_HZ)),
                ('clock-names', cstr('iface_clk') + cstr('core_clk')),
                ('status', cstr('ok')),
            ], [
                node('novatek-touchscreen', [
                    ph(),
                    ('device_type', cstr('touchscreen')),
                    ('compatible', cstr('novatek,NVT-ts')),
                    ('reg', u32(NVT_I2C_ADDRESS)),
                    ('lavender,irq-gpio-number', u32(NVT_IRQ_GPIO)),
                    ('lavender,reset-gpio-number', u32(NVT_RESET_GPIO)),
                ]),
            ]),
        ])
    arm_io = node('arm-io', [
        ph(),
        ('device_type', cstr('sweet-io')),
        ('compatible', cstr('sweet-io') + cstr('arm-io')),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
        ('ranges', u64(0, SOC_BASE, 0x100000000 - SOC_BASE)),
    ], arm_io_children)

    fbcon = node('sweet-fbcon', [
        ph(),
        ('reg', u64(FB, fb_size)),
        ('width', u32(fb_w)),
        ('height', u32(fb_h)),
        ('stride', u32(fb_stride)),
        ('depth', u32(32)),
    ])

    # Internal eMMC 5.1 controller (SDHC1) for bsd/dev/santoni_sdhci.c
    # At SDM660 MMIO 0x0c0c4000 (SDCC v5: native Host Controller at offset 0).
    sdhci = node('santoni-sdhci', [
        ph(),
        ('compatible', cstr('qcom,sdhci-msm-v5') + cstr('generic-sdhci')),
        ('device_type', cstr('sdhci')),
        ('reg', u64(0x0c0c4000, 0x1000)),
        ('hc-reg', u64(0x0c0c4000, 0x1000)),
        ('bus-width', u32(8)),
        ('non-removable', b''),
    ])

    defaults = node('defaults', [ph(), ('no-effaceable-storage', u32(1))])

    buttons = node('buttons', [
        ph(),
        ('button-names', cstr('volup') + cstr('voldown') + cstr('hold')),
        ('home-button-type', u32(2)),
        ('opposed-power-vol-buttons', u32(1)),
    ])

    if product_adt is not None and not os.path.isfile(product_adt):
        raise FileNotFoundError(f'product ADT does not exist: {product_adt}')
    product_props = d22ap_product(product_adt) if product_adt is not None else d22ap_product()
    if product_props:
        product_props = [(k, cstr('sRGB') if k == 'artwork-display-gamut' else v) for k, v in product_props]
    product = [node('product', [ph()] + product_props)] if product_props else []

    root_children = [chosen, defaults, cpus, arm_io, fbcon, buttons, sdhci]
    if board == 'lavender':
        root_children.append(node('soc', [
            ph(),
            ('compatible', cstr('simple-bus')),
            ('#address-cells', u32(2)),
            ('#size-cells', u32(2)),
            ('ranges', b''),
        ], [
            node('rpm-glink', [
                ph(),
                ('compatible', cstr('qcom,glink-rpm')),
                ('status', cstr('ok')),
            ]),
        ]))
    return node('device-tree', [
        ph(),
        ('compatible', cstr('D22AP') + cstr('iPhone10,3') + cstr('xiaomi,' + cfg['name']) + cstr('xiaomi,sweet') + cstr('qcom,' + cfg['soc']) + cstr('AppleARM')),
        ('model', cstr('iPhone10,3')),
        ('target-type', cstr('D22')),
        ('target-sub-type', cstr('D22AP')),
        ('manufacturer', cstr('Apple Inc.')),
        ('#address-cells', u32(2)),
        ('#size-cells', u32(2)),
    ], root_children + product)


if __name__ == '__main__':
    args = sys.argv[1:]
    board = DEFAULT_BOARD
    product_adt = None
    if '--board' in args:
        at = args.index('--board')
        if at + 1 >= len(args):
            print("mkadt.py: --board needs lavender or whyred", file=sys.stderr)
            sys.exit(2)
        board = args[at + 1].lower()
        del args[at:at + 2]
    if board not in BOARDS:
        print("mkadt.py: unknown board %r (want %s)" % (board, '/'.join(BOARDS)),
              file=sys.stderr)
        sys.exit(2)
    if '--product-adt' in args:
        at = args.index('--product-adt')
        if at + 1 >= len(args):
            print('mkadt.py: --product-adt needs a decoded Apple ADT path', file=sys.stderr)
            sys.exit(2)
        product_adt = args[at + 1]
        del args[at:at + 2]
    if not args:
        print("Usage: mkadt.py OUT.adt [--board lavender|whyred] [--dump] [--qemu] [--native]",
              file=sys.stderr)
        sys.exit(1)
    root = build(qemu='--qemu' in args, native='--native' in args, board=board, product_adt=product_adt)
    blob = adt.serialize(root)
    out_path = args[0]
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, 'wb') as f:
        f.write(blob)
    print(f"Serialized ADT ({len(blob)} bytes) -> {out_path}")
