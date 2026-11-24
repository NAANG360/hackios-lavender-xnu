# Host tests

`tests/run_tests.sh` compiles the **real patched SDHCI driver** against a mock MMIO/card backend. It caught a real bug in the 0192 patch before the kernel build: the new helper could not be named `sdhci_wait_card_ready` because the CMD6 helper already has that name. It was renamed `sdhci_wait_write_ready`.

## What is covered

- SDHCI register layout (SDCC v4 and v5), PIO reads/writes, FIFO/PRESENT_STATE fallbacks.
- CMD24/CMD25 write path, transfer-complete and the new `sdhci_wait_write_ready()`.
- Cache state (`CACHE_CTRL`), `sdhci_synchronize_cache()` and the 30 s cache barrier.
- Recovery paths (vendor reset, clock cycle, inhibit handling).
- GPT parser with the SYSTEM log fence, log ring packing/wrap-around, GIC wait, payload checks.

## Run

From the repository root:

```bash
bash tests/run_tests.sh
```

The relevant files are:

- `tests/test_sdhci_current.c` — the current-driver mock; includes the write path, lock behavior and the timeout test.
- `tests/prepare_kernel_mock.py` — rebuilds the mock from the patch series with `git apply`; the selected function list must include `sdhci_wait_write_ready`.
- `tests/test_log_ring.c` — `SANTONILOG1` header packing and the 4 MiB ring decoder.

The 0192 test asserts that a write whose card never becomes ready returns `ETIMEDOUT` after the **30 s** `CACHE_FLUSH_TIMEOUT_NS` bound (the mock advances simulated time). The 1 s `CMD6_BUSY_TIMEOUT_NS` variant was rejected on hardware: it stopped at `cc_fips_test`.

## Note

The suite came with the series from `ios-lavender` (commit `bc606b7`). The one change is that the HakiOS checksum check is skipped, because those third-party references for another phone are not carried here. If you change a driver, run the full suite before building the kernel; CI runs it first.

# Host tests for 0200 (USB) and 0201 (keys) — `tests/host/`

These tests live in this repository. They compile the **real patched** `bsd/dev/sweet_usb.c` and `bsd/dev/sweet_keys.c` with `-Wall -Wextra -Werror` against simulated SDM660 registers. The only change to the sources is that the ARM barrier asm is replaced by a `MOCK_BARRIER()` hook, which is where the fake hardware reacts. Stub headers stand in for the XNU kernel interfaces (`mock/xnu_mock.h`).

```bash
# 1. patched sources from xnu/patches/
bash tests/host/prepare-tree.sh /tmp/lavender-tree
# 2. build and run (clang; e.g. the shim's Docker image)
bash tests/host/run.sh /tmp/lavender-tree
```

| Test | Checks |
| --- | --- |
| `test_lavender_keys` | Arbiter v3 version gate; a duplicated PPID resolves to the apps-owned APID; observer page `0x9400000 + 0x8000*apid`; the PM660L GPIO7 (PPID `0x2c6`), not the PM660 one; power, volume-up and volume-down produce 6 correct HID events; only EXT_READL commands are written; second init is a no-op; a GPIO configured as output is withheld; a v5 arbiter is rejected |
| `test_lavender_usb_1` | `sweetusb=1` with everything powered: no GCC/DWC3/QSCRATCH/PHY register changes; the SM7150 addresses are never mapped; the probe runs once |
| `test_lavender_usb_2` | `sweetusb=3` with the USB off and the core in OTG: GDSC and the 7 branch clocks enabled, `GCTL.PrtCapDir` set to device without touching other bits, VBUS override in QSCRATCH, `RUN_STOP`, event buffer in our DMA page, `ue0` created and configured, event and live-log threads started |
| `test_lavender_usb_3` | `sweetusb=1` with the USB off: DWC3 and the PHY are never mapped and nothing is written |
| `test_lavender_usb_4` | `0210`, the state of the first lavender boot (USB on, QUSB2 `POWERDOWN 0x23`, PLL unlocked, TCSR clamp engaged) with `sweetusb=2`: the clamp is released, `POWERDOWN` becomes `0x22`, `PORT_TUNE1` is kept, the PLL locks before `RUN_STOP`; `sweetusbhold=1` stays in its status loop |
| `test_lavender_usb_7` | `0213` CDC-NCM driven like Windows: 86-byte config descriptor (02/0D/00, NCM functional, data 0A/00/01), product string, `GET_NTB_PARAMETERS`, `SET_NTB_FORMAT` 16 ok / 32 stalled, configuration, SPEED_CHANGE then NETWORK_CONNECTION, an OUT NTB with two datagrams, a malformed NTB dropped, IN NTB16 framing and the 512-byte pad, bus reset and reconfiguration, no mbuf leaks |
| `test_lavender_usb_6` | `0211` off (`sweetusbfix=0`) on a model where `SETEPCONFIG` only completes after a PHYSOFTRST with the PLL locked: reproduces the device log (`ep0 cmd 0x1 timeout`, no `RUN_STOP`). Scenarios 2/4 check the `0211` steps (GENERAL_CFG `0x9`, GUCTL1 bits 17/26, SUSPHY/ENBLSLPM/PHYSOFTRST clear, GCTL device + RAMCLKSEL 0, U1/U2 off) |
| `test_lavender_usb_4`/`_5` (0212) | Start from `UTMI_CTRL2 0x40` (`TEST_MUX_SEL`, as in every lavender log) and expect the mux released (`0x00`) |
| `test_lavender_usb_5` | `0210` with the PHY supplies off (PLL never locks): the wait is bounded (5 ms), the gadget still starts, `sweetusbhold=0` returns at once |
| topaz syntax check | `sweet_keys.c` still compiles without `ARM64_BOARD_CONFIG_LAVENDER` |

The simulated registers check the driver's logic and its addresses. They do not show that the hardware behaves the same way. The first device boot with `sweetusb=1` is the real check (see `docs/BOOT.md`).
