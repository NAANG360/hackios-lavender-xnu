#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else "dxb/xnu")
base=root/"iokit"/"Kernel"/"arm"
driver=base/"LavenderQUPI2C.cpp"; header=base/"LavenderQUPI2C.h"
files=root/"iokit"/"conf"/"files.arm64"; expert=base/"LavenderPlatformExpert.cpp"
for p in (driver,header,files,expert):
 if not p.is_file(): raise SystemExit(f"missing QUP integration file: {p}")
s=driver.read_text();h=header.read_text()
need=("LAVENDER_QUP_I2C_BASE","LAVENDER_TLMM_SOUTH_BASE","LAVENDER_TLMM_GPIO2_CTL","LAVENDER_TLMM_GPIO3_CTL","lavender_qup_tlmm_control","setQUPPins","mapTLMM","GCC_QUP1_I2C_CMD_RCGR","QUP_EN_VERSION_TWO_TAG","QUP_MX_READ_COUNT","QUP_MX_WRITE_COUNT","IOInterruptEventSource::interruptEventSource","IOLockSleepDeadline","QUP_I2C_NACK","QUP_I2C_ARB_LOST","QUP_I2C_BUS_ERROR","lavender_qup_build_output","lavender_qup_consume_input","lavender_qup_choose_mode","QUP_OUT_BLOCK_WRITE_REQ","lavender_qup_register_counts","mxInputCount","mxOutputCount","QUP_OUTPUT_MODE_SHIFT","QUP_MODE_BLOCK","waitForBlock","fTxBlockEvents","fRxBlockEvents","IOService::publishResource","LavenderQUPI2C::probe","IONameMatch","qcom,i2c-msm-v2","LAVENDER_QUP_I2C_ADDRESS")
for x in need:
 if x not in s and x not in h: raise SystemExit(f"missing QUP implementation marker: {x}")
adt_source=(Path(__file__).resolve().parents[1]/"tools"/"mkadt-lavender.py").read_text()
for token in ("I2C0, I2C0_SIZE = 0x0C175000, 0x600","I2C0_IRQ_SPEC = (0, 0x5F, 4)","('compatible', cstr('qcom,i2c-msm-v2'))","NVT_I2C_ADDRESS, NVT_IRQ_GPIO, NVT_RESET_GPIO = 0x62, 67, 66"):
 if token not in adt_source: raise SystemExit(f"generated ADT hardware contract changed: {token}")
if "iokit/Kernel/arm/LavenderQUPI2C.cpp optional iokitcpp" not in files.read_text(): raise SystemExit("QUP source is not compiled")
if "lavender_register_qup_i2c_driver();" not in expert.read_text(): raise SystemExit("QUP personality is not registered")
for x in ("platform_driver","i2c_adapter","devm_","readl_relaxed","writel_relaxed"):
 if x in s: raise SystemExit(f"Linux-only implementation leaked: {x}")
if "QUP_TAG2_START" not in h or "QUP_TAG2_DATA_READ_N_STOP" not in h: raise SystemExit("QUP-v2 tags missing")
if "LAVENDER_QUP_MODE_BLOCK" not in h or "lavender_qup_fifo_geometry" not in h: raise SystemExit("QUP FIFO/block geometry selection missing")
if "LavenderQUPI2CPinsConfigured" not in s: raise SystemExit("QUP TLMM pins are not marked configured")
if "LavenderI2CTransferFunction" not in s or "callPlatformFunction" not in s: raise SystemExit("QUP provider client call path is not published")
start=s[s.index("bool LavenderQUPI2C::start"):s.index("void LavenderQUPI2C::releaseResources")]
if not (start.find("mapTLMM()") < start.find("setQUPPins(true)") < start.find("enableClocks()")): raise SystemExit("QUP TLMM active state is not applied before controller clocks")
release=s[s.index("void LavenderQUPI2C::releaseResources"):s.index("void LavenderQUPI2C::stop")]
if "setQUPPins(false)" not in release or release.find("disableClocks()") > release.find("setQUPPins(false)"): raise SystemExit("QUP pins are not restored to sleep state during resource release")
if "fFirstTransferLogged" not in s or "first client transaction completed" not in s: raise SystemExit("QUP first-client-transfer runtime proof is missing")
if "OSPtr<" in s or ".get()" in s: raise SystemExit("non-XNU smart-pointer ownership pattern leaked into driver")
print("Lavender QUP-v2 IOKit source guard passed")
