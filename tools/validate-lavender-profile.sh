#!/bin/bash
set -euo pipefail
machine="${1:-LAVENDER}"
patch="xnu/patches/0082-lavender-machine-profile.patch"
cache_patch="xnu/patches/0083-lavender-cache-config.patch"
expert_patch="xnu/patches/0087-lavender-linker-and-platform-expert.patch"
config_patch="xnu/patches/0088-lavender-config-groups.patch"
sampler_patch="xnu/patches/0089-lavender-exclude-sweet-sampler.patch"
monitor_patch="xnu/patches/0090-lavender-no-apple-monitor.patch"
table_patch="xnu/patches/0091-lavender-config-table-placement.patch"
if [[ "$machine" != "LAVENDER" ]]; then
  echo "Machine profile validation skipped for $machine"
  exit 0
fi
test -s "$patch"
test -s "$cache_patch"
test -s "$expert_patch"
test -s "$config_patch"
test -s "$sampler_patch"
test -s "$monitor_patch"
test -s "$table_patch"
grep -q 'SUPPORTED_ARM64_MACHINE_CONFIGS = .*LAVENDER' "$patch"
grep -q 'MACHINE_FLAGS_ARM64_LAVENDER' "$patch"
grep -q 'ARM64_BOARD_CONFIG_LAVENDER' "$patch"
grep -q '#define MAX_CPUS[[:space:]]*8' "$patch"
grep -q '#define MAX_CPU_CLUSTERS[[:space:]]*2' "$patch"
grep -q '#define ARM_ARCH_TIMER' "$patch"
grep -q '#define __ARM_AMP__' "$patch"
grep -q '#define HAS_GIC_V3[[:space:]]*1' "$patch"
grep -q '__ARM_16K_PG__' "$patch"
grep -q '#elif defined (ARM64_BOARD_CONFIG_LAVENDER)' "$cache_patch"
grep -q '#define MMU_I_CLINE 6' "$cache_patch"
grep -q '#define MMU_CLINE[[:space:]]*6' "$cache_patch"
grep -q 'LavenderPlatformExpert' "$expert_patch"
grep -q 'IONameMatch.*qcom,sdm660-mtp' "$expert_patch"
grep -q 'nos_arm_asm' "$config_patch"
grep -q 'nos_arm_pmap' "$config_patch"
grep -q 'ARM64_BOARD_CONFIG_SWEET' "$sampler_patch"
grep -q 'ARM64_BOARD_CONFIG_LAVENDER' "$monitor_patch"
for required_patch in \
  xnu/patches/0082-lavender-machine-profile.patch \
  xnu/patches/0083-lavender-cache-config.patch \
  xnu/patches/0085-lavender-gic-register-definitions.patch \
  "$expert_patch" "$config_patch" "$sampler_patch" "$monitor_patch" "$table_patch"; do
  if ! git apply --numstat "$required_patch" >/dev/null; then
    echo "Malformed Lavender patch: $required_patch" >&2
    exit 1
  fi
done
if grep -Eiq 'SWEET_VTIMER_INTID|sweet_touch_init|sweet_keys_init|sweet_ufs' "$patch"; then
  echo "ERROR: SWEET-specific runtime code leaked into the Lavender profile" >&2
  exit 1
fi
echo "Lavender profile preflight passed (compile profile only; not boot-ready)."
