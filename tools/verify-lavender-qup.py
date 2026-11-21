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
need=("LAVENDER_QUP_I2C_BASE","LAVENDER_TLMM_SOUTH_BASE","LAVENDER_TLMM_GPIO2_CTL","LAVENDER_TLMM_GPIO3_CTL","LAVENDER_TLMM_QUP1_SDA_CTL","LAVENDER_TLMM_QUP1_SCL_CTL","lavender_qup_tlmm_control","lavender_qup_tlmm_gpio_control","lavender_qup_block_read_words","setQUPPins","setQUPPinsGPIO","setGPIOOutputLow","releaseGPIO","waitGPIOHigh","recoverBus","mapTLMM","GCC_QUP1_I2C_CMD_RCGR","QUP_EN_VERSION_TWO_TAG","QUP_MX_READ_COUNT","QUP_MX_WRITE_COUNT","IOInterruptEventSource::interruptEventSource","IOLockSleepDeadline","QUP_I2C_NACK","QUP_I2C_ARB_LOST","QUP_I2C_BUS_ERROR","lavender_qup_build_output","lavender_qup_consume_input","lavender_qup_choose_mode","QUP_OUT_BLOCK_WRITE_REQ","lavender_qup_register_counts","mxInputCount","mxOutputCount","QUP_OUTPUT_MODE_SHIFT","QUP_MODE_BLOCK","waitForBlock","fTxBlockEvents","fRxBlockEvents","IOService::publishResource","LavenderQUPI2C::probe","IONameMatch","qcom,i2c-msm-v2","LAVENDER_QUP_I2C_ADDRESS")
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
if "lavender_qup_block_read_words(count,g.inputBlockBytes,&words)" not in s: raise SystemExit("QUP block-mode final partial input block is not drained to exact byte count")
transfer=s[s.index("IOReturn LavenderQUPI2C::transfer"):s.index('extern "C" void lavender_register_qup_i2c_driver')]
if not (transfer.find("LavenderQUPRegisterCounts counts={};") < transfer.find("goto done;")): raise SystemExit("QUP transfer cleanup jumps across initialized local state")
if "LavenderQUPI2CPinsConfigured" not in s: raise SystemExit("QUP TLMM pins are not marked configured")
if "LavenderI2CTransferFunction" not in s or "callPlatformFunction" not in s: raise SystemExit("QUP provider client call path is not published")
start=s[s.index("bool LavenderQUPI2C::start"):s.index("void LavenderQUPI2C::releaseResources")]
if not (start.find("mapTLMM()") < start.find("setQUPPins(true)") < start.find("enableClocks()")): raise SystemExit("QUP TLMM active state is not applied before controller clocks")
if start.find("IOService::publishResource") > start.find("registerService()"): raise SystemExit("QUP transfer resource must be published before clients match")
release=s[s.index("void LavenderQUPI2C::releaseResources"):s.index("void LavenderQUPI2C::stop")]
if "setQUPPins(false)" not in release or release.find("disableClocks()") > release.find("setQUPPins(false)"): raise SystemExit("QUP pins are not restored to sleep state during resource release")
if "provider matched qcom,i2c-msm-v2" not in s or "fFirstTransferLogged" not in s or "first client transaction completed" not in s: raise SystemExit("QUP match/first-client-transfer runtime instrumentation is missing")
if "bool LavenderQUPI2C::recoverBus" not in s or "recoverBus();" not in s: raise SystemExit("bounded QUP bus recovery is not called on transfer errors")
if "scl=LAVENDER_TLMM_QUP1_SCL_CTL,sda=LAVENDER_TLMM_QUP1_SDA_CTL" not in s: raise SystemExit("QUP1 recovery line mapping disagrees with DTS: GPIO3=SCL, GPIO2=SDA")
if "LAVENDER_TLMM_QUP1_SDA_CTL LAVENDER_TLMM_GPIO2_CTL" not in h or "LAVENDER_TLMM_QUP1_SCL_CTL LAVENDER_TLMM_GPIO3_CTL" not in h: raise SystemExit("QUP1 DTS line mapping is not represented in native GPIO aliases")
if "QUP_BUS_ACTIVE" not in s or "QUP_BUS_MASTER" not in s or "QUP_I2C_SDA" not in s: raise SystemExit("QUP recovery bus-state checks are missing")
if "fRecovering=attemptRecovery" not in s or "fIRQ->disable()" not in s or "fIRQ->enable()" not in s: raise SystemExit("QUP recovery is not isolated from interrupt handling")
if "setQUPPinsGPIO()" not in s or "setQUPPins(true)" not in s: raise SystemExit("QUP pin mux is not switched to GPIO and restored")
if "for(uint32_t pulse=0;ok&&pulse<9" not in s or "waitGPIOHigh" not in s: raise SystemExit("QUP recovery lacks bounded nine-clock/clock-stretch handling")
if "IOLog(\"LAVENDER QUP: bus-clear %s" not in s: raise SystemExit("QUP bus-clear result instrumentation is missing")
if "OSPtr<" in s or ".get()" in s: raise SystemExit("non-XNU smart-pointer ownership pattern leaked into driver")
print("Lavender QUP-v2 IOKit source guard passed")
