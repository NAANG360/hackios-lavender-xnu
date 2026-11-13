#!/usr/bin/env python3
"""Host-only regression checks for the Lavender ADT; never boots or flashes."""
import importlib.util
import pathlib
import struct

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("mkadt_lavender", ROOT / "tools/mkadt-lavender.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

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
    assert base == 0xA0400000
    assert base + size == 0xEA848000
    assert base % 0x4000 == 0 and size % 0x4000 == 0
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
    assert tuple(ids) == mod.CPU_MPIDRS
    assert ids[0] == 0x000  # actual Build 4 FDT's cpu@0 is Linux cpu0
    assert b"qcom,sdm660" in props(parsed)["compatible"]
    assert b"xiaomi,lavender" in props(parsed)["compatible"]
    arm_io = child(parsed, "arm-io")
    i2c = child(arm_io, "i2c0")
    touch = child(i2c, "novatek-touchscreen")
    tp = props(touch)
    assert struct.unpack("<I", tp["reg"])[0] == 0x62
    assert struct.unpack("<I", tp["lavender,irq-gpio-number"])[0] == 67
    assert struct.unpack("<I", tp["lavender,reset-gpio-number"])[0] == 66
    emmc = props(child(arm_io, "emmc"))
    assert len(emmc["reg"]) == 48  # three 64-bit address/size windows
    assert b"qcom,sdhci-msm-cqe" in emmc["compatible"]

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

    print("PASS: Lavender ADT memory, framebuffer, CPU order, eMMC and NVT touch checks")

if __name__ == "__main__":
    run()
