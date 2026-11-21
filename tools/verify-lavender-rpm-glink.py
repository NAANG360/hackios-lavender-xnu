#!/usr/bin/env python3
"""Guard the native SDM660 RPM/GLINK rpm_requests bring-up path."""
from pathlib import Path
import sys


def require(condition, message):
    if not condition:
        raise SystemExit("FAIL: " + message)


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {Path(sys.argv[0]).name} XNU_SOURCE_ROOT")
    root = Path(sys.argv[1])
    driver = (root / "iokit/Kernel/arm/LavenderRPMGLINK.cpp").read_text()
    header = (root / "iokit/Kernel/arm/LavenderRPMGLINK.h").read_text()
    qup = (root / "iokit/Kernel/arm/LavenderQUPI2C.cpp").read_text()
    expert = (root / "iokit/Kernel/arm/LavenderPlatformExpert.cpp").read_text()
    files = (root / "iokit/conf/files.arm64").read_text()

    for token in (
        "LAVENDER_RPM_MSG_RAM_BASE 0x00778000ULL",
        "LAVENDER_RPM_MSG_RAM_SIZE 0x00007000U",
        "LAVENDER_RPM_TOC_MAGIC 0x67727430U",
        "LAVENDER_RPM_RX_FIFO_ID 0x72326170U",
        "LAVENDER_RPM_TX_FIFO_ID 0x61703272U",
        'LAVENDER_RPM_CHANNEL "rpm_requests"',
        "LAVENDER_RPM_APCS_BASE 0x17911000ULL",
        "LAVENDER_RPM_APCS_DOORBELL 0x008U",
        "LAVENDER_RPM_IRQ_SPI 168U",
        "LAVENDER_RPM_RESOURCE_RWXC 0x78637772U",
        "LAVENDER_RPM_RESOURCE_ID_VDD_DIG 0U",
        "LAVENDER_RPM_KVP_VLVL 0x6c766c76U",
        "LAVENDER_RPM_LEVEL_LOW_SVS 64U",
        "LAVENDER_RPM_V0_REQUEST_SIZE 40U",
        "lavender_rpm_parse_toc",
        "lavender_rpm_encode_v0_vdd_dig_vote",
        "lavender_rpm_decode_v0_ack",
        "lavender_rpm_encode_glink_tx_data",
    ):
        require(token in header, "RPM protocol header missing " + token)

    for token in (
        "LavenderRPMGLINK::pumpRX",
        "LAVENDER_GLINK_CMD_VERSION_ACK",
        "LAVENDER_GLINK_CMD_OPEN",
        "LAVENDER_GLINK_CMD_OPEN_ACK",
        "LAVENDER_GLINK_CMD_TX_DATA",
        "LAVENDER_GLINK_CMD_TX_DATA_CONT",
        "LAVENDER_RPM_CHANNEL_LENGTH",
        "waitForChannel",
        "requestVddDigLowSVS",
        "lavender_register_rpm_glink_driver",
        "rpmWrite32(fAPCS,LAVENDER_RPM_APCS_DOORBELL",
        "LAVENDER_RPM_IRQ_SPI",
        "polled",
    ):
        require(token in driver, "native RPM/GLINK driver missing " + token)

    require("lavender_rpm_vote_vdd_dig_low_svs" in qup,
            "QUP driver does not request the native RPM voltage vote")
    require('extern "C" bool lavender_rpm_vote_vdd_dig_low_svs(void)' in driver,
            "RPM implementation does not export the voltage-vote function with C linkage")
    require('extern "C" bool lavender_rpm_vote_vdd_dig_low_svs(void);' in qup,
            "QUP declaration does not use matching C linkage")
    require(driver.count('extern "C" bool lavender_rpm_vote_vdd_dig_low_svs(void)') == 1,
            "RPM voltage-vote definition is missing or duplicated")
    start = qup.index("bool LavenderQUPI2C::start")
    start_body = qup[start:qup.index("\n}", start)]
    vote = start_body.index("lavender_rpm_vote_vdd_dig_low_svs")
    clocks = start_body.index("enableClocks()")
    require(vote < clocks, "QUP enables its GCC clocks before the acknowledged LOW_SVS vote")
    require("refusing clock enable without acknowledged vdd_dig LOW_SVS vote" in qup,
            "QUP does not fail closed when the RPM vote is unavailable")

    rpm_register = expert.index("lavender_register_rpm_glink_driver();")
    qup_register = expert.index("lavender_register_qup_i2c_driver();")
    require(rpm_register < qup_register,
            "RPM/GLINK personality must be registered before the QUP personality")
    require('childFromPath("soc", gIODTPlane)' in expert and
            'childFromPath("rpm-glink", gIODTPlane)' in expert and
            "LavenderRPMGLINK.cpp" in files,
            "qcom,glink-rpm provider nub is not exposed/compiled")

    require('extern "C" void lavender_register_rpm_glink_driver(void);' in expert,
            "PlatformExpert lacks the RPM personality C-linkage declaration")
    require('#include <libkern/c++/OSArray.h>' in expert and
            "createNubs(this, rpmNubs)" in expert,
            "PlatformExpert rpm-glink nub publication is incomplete")
    require(expert.count("lavender_register_rpm_glink_driver();") == 1,
            "PlatformExpert RPM driver registration is missing or duplicated")
    require(files.count("iokit/Kernel/arm/LavenderRPMGLINK.cpp optional iokitcpp") == 1,
            "RPM translation unit source-list row is missing or duplicated")
    require("if (haveRx) return false;" in header and
            "if (haveTx) return false;" in header and
            "found.rx.tailOffset<txEnd && found.tx.tailOffset<rxEnd" in header,
            "RPM TOC duplicate/overlap fail-closed checks are missing")

    print("PASS: native Lavender rpm_requests GLINK transport, RPM-SMD v0 LOW_SVS request, and QUP power-vote gate")


if __name__ == "__main__":
    main()
