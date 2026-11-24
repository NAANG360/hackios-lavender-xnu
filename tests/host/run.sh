#!/bin/bash
# Host tests for patches 0200 (USB) and 0201 (keys) on LAVENDER.
#   hosttest/run.sh <dir with bsd/dev/sweet_usb.c and bsd/dev/sweet_keys.c>
# The driver sources are copied with their ARM barrier asm replaced by a
# MOCK_BARRIER() hook (the point where the fake hardware reacts); nothing
# else in them is changed.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
src="$(cd "${1:?source tree}" && pwd)"
out="${OUT_DIR:-$here/out}"
CC="${CC:-clang}"
mkdir -p "$out/mock" "$out/src"

for h in sys/param.h sys/systm.h kern/clock.h kern/thread.h kern/sched_prim.h \
	machine/machine_routines.h pexpert/pexpert.h arm/caches_internal.h vm/pmap.h \
	vm/vm_kern.h kern/locks.h sys/kpi_mbuf.h sys/kpi_socket.h sys/sockio.h net/if.h \
	net/if_types.h net/ethernet.h net/if_ether.h net/ether_if_module.h \
	net/kpi_interface.h net/kpi_protocol.h netinet/in.h; do
	mkdir -p "$out/mock/$(dirname "$h")"
	printf '#include "%s/mock/xnu_mock.h"\n' "$here" > "$out/mock/$h"
done

for f in sweet_usb.c sweet_keys.c; do
	sed -E 's/__asm__ volatile ?\(/MOCK_BARRIER(/' "$src/bsd/dev/$f" > "$out/src/$f"
	if grep -q '__asm__' "$out/src/$f"; then
		echo "unhandled asm in $f" >&2
		exit 1
	fi
done

FLAGS="-O1 -g -Wall -Wextra -Werror -Wno-unused-function -I$out/mock -I$here \
	-DARM64_BOARD_CONFIG_SWEET=1 -DARM64_BOARD_CONFIG_LAVENDER=1 -DLAVENDER=1"

echo "== LAVENDER keys (0201) =="
$CC $FLAGS -DKEYS_SOURCE="\"$out/src/sweet_keys.c\"" "$here/test_lavender_keys.c" -o "$out/test_lavender_keys"
"$out/test_lavender_keys"

echo "== LAVENDER USB (0200) =="
for s in 1 2 3 4 5 6 7; do
	$CC $FLAGS -DSCENARIO=$s -DUSB_SOURCE="\"$out/src/sweet_usb.c\"" \
		"$here/test_lavender_usb.c" -o "$out/test_lavender_usb_$s"
	"$out/test_lavender_usb_$s"
done

echo "== topaz build of sweet_keys.c still compiles (no LAVENDER) =="
$CC -fsyntax-only -Wall -Wextra -Werror -I"$out/mock" -DARM64_BOARD_CONFIG_SWEET=1 \
	-include "$here/mock/xnu_mock.h" -DMOCK_BARRIER\(...\)= \
	-include "$here/topaz_stubs.h" "$out/src/sweet_keys.c"
echo "all host tests passed"
