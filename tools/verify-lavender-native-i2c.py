#!/usr/bin/env python3
from pathlib import Path
import argparse
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
SUPPORT=ROOT/"xnu/patches/support"
qup=(SUPPORT/"LavenderQUPI2C.cpp").read_text()
quph=(SUPPORT/"LavenderQUPI2C.h").read_text()
rpm=(SUPPORT/"LavenderRPMGLINK.cpp").read_text()
rpmh=(SUPPORT/"LavenderRPMGLINK.h").read_text()
stage=(ROOT/"xnu/patches/0600-lavender-native-i2c.sh").read_text()
adt_source=(ROOT/"tools/mkadt.py").read_text()
for token in ("LAVENDER_QUP_I2C_BASE","qcom,i2c-msm-v2","QUP_EN_VERSION_TWO_TAG",
              "lavender_qup_build_output","lavender_qup_consume_input","lavender_qup_choose_mode",
              "QUP_MODE_BLOCK","waitBusIdle(100)","recoverBus()","lavender_rpm_vote_vdd_dig_low_svs",
              'PE_parse_boot_argn("lavenderqup"'):
    if token not in qup and token not in quph: raise SystemExit(f"missing QUP contract: {token}")
for token in ("LAVENDER_RPM_MSG_RAM_BASE","LAVENDER_RPM_APCS_BASE","rpm_requests",
              "LAVENDER_RPM_LEVEL_LOW_SVS","lavender_rpm_parse_toc","lavender_rpm_decode_v0_ack",
              'PE_parse_boot_argn("lavenderrpm"'):
    if token not in rpm and token not in rpmh: raise SystemExit(f"missing RPM contract: {token}")
for token in ("SweetPlatformExpert.cpp","files.arm64","LavenderQUPI2C.cpp",
              "LavenderRPMGLINK.cpp","lavender_register_rpm_glink_driver"):
    if token not in stage: raise SystemExit(f"missing stage contract: {token}")
for token in ("I2C0, I2C0_SIZE = 0x0C175000, 0x600","I2C0_IRQ_SPEC = (0, 0x5F, 4)",
              "qcom,i2c-msm-v2","qcom,glink-rpm"):
    if token not in adt_source: raise SystemExit(f"missing ADT contract: {token}")
sys.path.insert(0,str(ROOT/"tools"))
import mkadt
def walk(node,path="/"):
    yield path,node
    for child in node.children:
        name=next((v[:-1].decode("utf-8","ignore") for k,_f,v in child.props if k=="name" and v.endswith(b"\0")),"unknown")
        cp=(path.rstrip("/")+"/"+name) if path!="/" else "/"+name
        yield from walk(child,cp)
nodes=dict(walk(mkadt.build(board="lavender")))
i2c=nodes.get("/arm-io/i2c0")
if i2c is None: raise SystemExit("ADT lacks /arm-io/i2c0")
props={k:v for k,_f,v in i2c.props}
if b"qcom,i2c-msm-v2\0" not in props.get("compatible",b""): raise SystemExit("QUP compatible mismatch")
reg=props.get("reg")
if reg is None or len(reg)!=16: raise SystemExit("QUP reg tuple malformed")
base,size=struct.unpack("<QQ",reg)
if base+0x01000000!=0x0C175000 or size<0x600: raise SystemExit(f"QUP reg mismatch {base:#x}/{size:#x}")
if "/soc/rpm-glink" not in nodes: raise SystemExit("ADT lacks /soc/rpm-glink")
rp={k:v for k,_f,v in nodes["/soc/rpm-glink"].props}
if b"qcom,glink-rpm\0" not in rp.get("compatible",b""): raise SystemExit("RPM compatible mismatch")
parser=argparse.ArgumentParser()
parser.add_argument("--xnu-root")
args=parser.parse_args()
if args.xnu_root:
    xr=Path(args.xnu_root)
    for rel in ("iokit/Kernel/arm/LavenderQUPI2C.cpp",
                "iokit/Kernel/arm/LavenderQUPI2C.h",
                "iokit/Kernel/arm/LavenderRPMGLINK.cpp",
                "iokit/Kernel/arm/LavenderRPMGLINK.h"):
        if not (xr/rel).is_file():
            raise SystemExit(f"staged XNU source missing: {rel}")
    f=(xr/"iokit/conf/files.arm64").read_text()
    if "\\n" in f: raise SystemExit("files.arm64 contains literal escaped newlines")
    e=(xr/"iokit/Kernel/arm/SweetPlatformExpert.cpp").read_text()
    for token in ("LavenderQUPI2C.cpp optional iokitcpp",
                  "LavenderRPMGLINK.cpp optional iokitcpp"):
        if f.count(token) != 1: raise SystemExit(f"files.arm64 entry missing or duplicated: {token}")
    for token in ("lavender_register_rpm_glink_driver();",
                  "lavender_register_qup_i2c_driver();",
                  'childFromPath("rpm-glink"'):
        if token not in e: raise SystemExit(f"SweetPlatformExpert missing: {token}")
print("Lavender native I2C/RPM ADT and source guards passed")
