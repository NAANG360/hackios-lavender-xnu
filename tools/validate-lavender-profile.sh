#!/bin/bash
set -euo pipefail

machine="${1:-LAVENDER}"
patch="xnu/patches/0082-lavender-machine-profile.patch"

if [[ "$machine" != "LAVENDER" ]]; then
  echo "Machine profile validation skipped for $machine"
  exit 0
fi

test -s "$patch"
grep -q 'SUPPORTED_ARM64_MACHINE_CONFIGS = .*LAVENDER' "$patch"
grep -q 'MACHINE_FLAGS_ARM64_LAVENDER' "$patch"
grep -q 'ARM64_BOARD_CONFIG_LAVENDER' "$patch"
grep -q '#define MAX_CPUS[[:space:]]*8' "$patch"
grep -q '#define MAX_CPU_CLUSTERS[[:space:]]*2' "$patch"
grep -q '#define HAS_GIC_V3[[:space:]]*1' "$patch"
grep -q '__ARM_16K_PG__' "$patch"

if grep -Eiq 'SWEET_VTIMER_INTID|sweet_touch_init|sweet_keys_init|sweet_ufs' "$patch"; then
  echo "ERROR: SWEET-specific runtime code leaked into the Lavender profile" >&2
  exit 1
fi

echo "Lavender profile preflight passed (compile profile only; not boot-ready)."
