#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
for source in LavenderRPMGLINK.cpp LavenderRPMGLINK.h; do
  test -s "$SCRIPT_DIR/support/$source"
  cp "$SCRIPT_DIR/support/$source" "iokit/Kernel/arm/$source"
done
