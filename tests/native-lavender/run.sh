#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="${OUT_DIR:-$(mktemp -d)}"
mkdir -p "$OUT"
CXX="${CXX:-c++}"
"$CXX" -std=c++17 -Wall -Wextra -Werror -I"$ROOT/xnu/patches/support" \
  "$ROOT/tests/native-lavender/test_lavender_qup_protocol.cpp" -o "$OUT/test_lavender_qup_protocol"
"$OUT/test_lavender_qup_protocol"
"$CXX" -std=c++17 -Wall -Wextra -Werror -I"$ROOT/xnu/patches/support" \
  "$ROOT/tests/native-lavender/test_lavender_rpm_glink.cpp" -o "$OUT/test_lavender_rpm_glink"
"$OUT/test_lavender_rpm_glink"
python3 "$ROOT/tools/verify-lavender-native-i2c.py"
echo "Lavender native QUP/RPM protocol tests passed"
