#!/usr/bin/env python3
"""Host-only regression checks for the Lavender ADT; never boots or flashes."""
import importlib.util
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("mkadt_lavender", ROOT / "tools/mkadt-lavender.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

payload_spec = importlib.util.spec_from_file_location("mkpayload", ROOT / "tools/mkpayload.py")
payload_mod = importlib.util.module_from_spec(payload_spec)
payload_spec.loader.exec_module(payload_mod)

def props(node):
    return {name: value for name, _, value in node.props}

def child(node, name):
    return next(c for c in node.children if c.name == name)

def run():
    root = mod.build(seed=bytes(range(64)), product_tree="/nonexistent/lavender-product.adt")
    blob = mod.adt.serialize(root)
    mod.check(blob)
    parsed, end = mod.adt.parse(blob)
    assert end == len(blob)
    chosen = props(child(parsed, "chosen"))
    base = struct.unpack("<Q", chosen["dram-base"])[0]
    size = struct.unpack("<Q", chosen["dram-size"])[0]
    assert base == 0xA2000000
    assert base + size == 0xEA848000
    assert base % mod.BOOTSTRAP_L2_BLOCK_SIZE == 0
    assert size % 0x4000 == 0
    # The first 28 MiB of System RAM are intentionally excluded so every
    # KVA L2 block maps to the same physical offset used by the shim loader.
    assert base - mod.RAM_START == 0x01C00000
    virt_base = 0xFFFFFFE006000000
    payload_mod.validate_lavender_bootstrap_alignment(virt_base, base)
    try:
        payload_mod.validate_lavender_bootstrap_alignment(virt_base, 0xA0400000)
    except ValueError:
        pass
    else:
        raise AssertionError("misaligned Lavender phys_base was accepted")
    pram = props(child(parsed, "pram"))
    panic_base, panic_size = struct.unpack("<QQ", pram["reg"])
    assert (panic_base, panic_size) == (base + size, 0x100000)
    fb = props(child(parsed, "lavender-fbcon"))
    fb_base, fb_size = struct.unpack("<QQ", fb["reg"])
    width = struct.unpack("<I", fb["width"])[0]
    height = struct.unpack("<I", fb["height"])[0]
    stride = struct.unpack("<I", fb["stride"])[0]
    assert (width, height, stride) == (1080, 2340, 4320)
    assert stride * height <= fb_size
    assert fb_base + fb_size <= base
    cpu = child(parsed, "cpus")
    assert len(cpu.children) == 8
    ids = [struct.unpack("<I", props(n)["reg"])[0] for n in cpu.children]
    assert len(set(ids)) == 8
    cluster_types = [struct.unpack("<I", props(n)["cluster-type"])[0] for n in cpu.children]
    assert cluster_types == [ord("E")] * 4 + [ord("P")] * 4
    assert tuple(ids) == mod.CPU_MPIDRS
    assert ids[0] == 0x000  # actual Build 4 FDT's cpu@0 is Linux cpu0
    assert b"qcom,sdm660" in props(parsed)["compatible"]
    assert b"xiaomi,lavender" in props(parsed)["compatible"]

    # The IOKit platform expert must match an actual compatible on this generated root.
    iokit_patch = (ROOT / "xnu/patches/0126-lavender-iokit-bringup.patch").read_text()
    assert "+    \"     'IONameMatch'     = 'xiaomi,lavender';\"" in iokit_patch
    assert "+    \"     'IONameMatch'     = 'qcom,sdm660-mtp';\"" not in iokit_patch
    arm_io = child(parsed, "arm-io")
    i2c = child(arm_io, "i2c0")
    touch = child(i2c, "novatek-touchscreen")
    tp = props(touch)
    assert struct.unpack("<I", tp["reg"])[0] == 0x62
    assert struct.unpack("<I", tp["lavender,irq-gpio-number"])[0] == 67
    assert struct.unpack("<I", tp["lavender,reset-gpio-number"])[0] == 66
    emmc = props(child(arm_io, "emmc"))
    expected_emmc = tuple(value for address, size in mod.EMMC_REGIONS
                          for value in (address - mod.SOC_BASE, size))
    assert struct.unpack("<6Q", emmc["reg"]) == expected_emmc
    assert emmc["reg-names"] == mod.cstr("hc_mem") + mod.cstr("cqhci_mem") + mod.cstr("cqhci_ice")
    assert struct.unpack("<6I", emmc["interrupts"]) == mod.EMMC_IRQ_SPEC
    assert emmc["interrupt-names"] == mod.cstr("hc_irq") + mod.cstr("pwr_irq")
    assert emmc["clock-names"] == mod.cstr("iface_clk") + mod.cstr("core_clk") + mod.cstr("ice_core_clk")
    assert struct.unpack("<I", emmc["bus-width"])[0] == 8
    assert emmc["non-removable"] == b""
    assert emmc["status"] == mod.cstr("ok")
    assert b"qcom,sdhci-msm-v5" in emmc["compatible"] and b"qcom,sdhci-msm-cqe" in emmc["compatible"]

    # Optional A73-only first-boot profile: boot MPIDR first, then only its cluster.
    a73_root = mod.build(seed=bytes(range(64)), product_tree="/nonexistent/lavender-product.adt",
                         boot_mpidr=0x100, cpu_count=4)
    a73_blob = mod.adt.serialize(a73_root)
    mod.check(a73_blob, boot_mpidr=0x100, cpu_count=4)
    a73_cpu_nodes = child(a73_root, "cpus").children
    a73_ids = [struct.unpack("<I", props(n)["reg"])[0] for n in a73_cpu_nodes]
    assert a73_ids == [0x100, 0x101, 0x102, 0x103]
    assert [props(n)["state"] for n in a73_cpu_nodes] == [mod.cstr("running"), mod.cstr("stopped"), mod.cstr("stopped"), mod.cstr("stopped")]
    assert [struct.unpack("<I", props(n)["cluster-id"])[0] for n in a73_cpu_nodes] == [1, 1, 1, 1]
    assert [struct.unpack("<I", props(n)["cluster-type"])[0] for n in a73_cpu_nodes] == [ord("P")] * 4

    # A small image fits after the kernel inside the one contiguous XNU aperture.
    kernel_end_va = 0xFFFFFFE008020000
    kernel_phys_end, ramdisk_end = payload_mod.validate_ramdisk_range(
        0xFFFFFFE006000000, kernel_end_va, base, size, 0x100000
    )
    assert kernel_phys_end == 0xA4020000
    assert ramdisk_end == 0xA4120000

    # mkiosroot.sh defaults to a 16 GiB logical HFS+ image. Reject it before
    # mkpayload reads/allocates the file: Lavender's current safe aperture has
    # only about 1.10 GiB left after the kernel image.
    try:
        payload_mod.validate_ramdisk_range(
            0xFFFFFFE006000000, kernel_end_va, base, size, 16 << 30
        )
    except ValueError as exc:
        assert "RAMDisk does not fit" in str(exc)
    else:
        raise AssertionError("oversized Lavender RAMDisk was accepted")

    print("PASS: Lavender ADT memory, framebuffer, CPU order, eMMC, NVT touch and bounded RAMDisk checks")

if __name__ == "__main__":
    run()
