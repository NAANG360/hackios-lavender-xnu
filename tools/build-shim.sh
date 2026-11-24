#!/bin/bash
# Build a lavender shim with the known-good boot arguments.
#
# Usage:
#   tools/build-shim.sh /path/to/kernel.development.lavender
#
# Requires clang, ld.lld, llvm-objcopy/readelf/nm and python3. Uses this
# repository's shim/ and resources/lavender_dtb.bin; the product ADT
# (d22ap.adt from the iPhone X IPSW) is not distributed and must be supplied
# as shim/build.sh expects.
set -euo pipefail

BASE="$(cd "$(dirname "$0")/.." && pwd)"
KERNEL="${1:?path to kernel.development.lavender}"
[ -f "$KERNEL" ] && KERNEL="$(cd "$(dirname "$KERNEL")" && pwd)/$(basename "$KERNEL")"
BOARD="${BOARD:-lavender}"
ROOTDEV="${ROOTDEV:-emmcroot0}"

# The lock-screen boot command line. Adjust only one variable at a time.
EXTRA_CMDLINE="${EXTRA_CMDLINE:-sweetlog=1 emmcrw=1 rootrw=1 cs_enforcement_disable=1 unentitled_ios_sim_launch=1 launchd_unsecure_cache=1 wdt=-1 wdt_test=no_user_checkin sweetfb_noconsole=1 sweetfb_stayon=1 sweetfb_hz=30 cpus=4}"
KPRINTF_SPAM="${KPRINTF_SPAM-}"

# Calendar (patches 0216/0220): sweetrtc = the build time, the fallback when
# the PM660 RTC is unusable; SWEETRTCOFF = this phone's offset from the RTC
# raw count to Unix time, measured once in TWRP (docs/BOOT.md, "Clock").
EXTRA_CMDLINE="$EXTRA_CMDLINE sweetrtc=$(date +%s)"
if [ -n "${SWEETRTCOFF:-}" ]; then
	EXTRA_CMDLINE="$EXTRA_CMDLINE sweetrtcoff=$SWEETRTCOFF"
fi

[ -f "$KERNEL" ] || { echo "missing kernel: $KERNEL" >&2; exit 1; }
[ -f "$BASE/shim/build.sh" ] || { echo "missing $BASE/shim/build.sh" >&2; exit 1; }

cd "$BASE"
BOARD="$BOARD" \
ROOTDEV="$ROOTDEV" \
EXTRA_CMDLINE="$EXTRA_CMDLINE" \
KPRINTF_SPAM="$KPRINTF_SPAM" \
bash shim/build.sh "$KERNEL"

ls -l shim/out/shim-boot.img
sha256sum shim/out/shim-boot.img
