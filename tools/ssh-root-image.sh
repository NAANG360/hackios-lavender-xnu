#!/bin/bash
# Install SSH over USB (tools/sweet-ssh.py) into the iOS root image and
# regenerate the sparse image for `fastboot flash userdata`. macOS only.
#
#   sudo bash tools/ssh-root-image.sh [IMG] [OUT.simg] [PUBKEY...]
#
# Defaults: ~/ios16-root.img -> ~/ios16-root-ssh.simg, keys from ~/lavender-keys/*.pub.
# SSH_OUT (default ~/bergamota/research/ssh-build/out) holds the dropbear/dash/
# toybox build of bergamota's tools/ssh/build.sh.
set -euo pipefail

USER_HOME="$HOME"
[ -n "${SUDO_USER:-}" ] && USER_HOME="/Users/$SUDO_USER"
HERE="$(cd "$(dirname "$0")" && pwd)"
IMG="${1:-$USER_HOME/ios16-root.img}"
OUT="${2:-$USER_HOME/ios16-root-ssh.simg}"
shift $(( $# > 2 ? 2 : $# ))
KEYS=("$@")
[ ${#KEYS[@]} -eq 0 ] && KEYS=("$USER_HOME"/lavender-keys/*.pub)
SSH_OUT="${SSH_OUT:-$USER_HOME/bergamota/research/ssh-build/out}"
IMG2SIMG="${IMG2SIMG:-$USER_HOME/bergamota/tools/img2simg.py}"
MNT=/tmp/iosroot-ssh

[ "$(id -u)" = 0 ] || { echo "run as root: sudo bash $0" >&2; exit 1; }
[ -f "$IMG" ] || { echo "missing image: $IMG" >&2; exit 1; }
[ -f "$IMG2SIMG" ] || { echo "missing $IMG2SIMG" >&2; exit 1; }
for k in "${KEYS[@]}"; do [ -f "$k" ] || { echo "missing public key: $k" >&2; exit 1; }; done

mkdir -p "$MNT"
mount | grep -q "on $MNT " && hdiutil detach "$MNT" >/dev/null 2>&1 || true
trap 'hdiutil detach "$MNT" >/dev/null 2>&1 || true' EXIT
echo "attaching $IMG read-write at $MNT"
hdiutil attach -readwrite -nobrowse -owners on -mountpoint "$MNT" "$IMG"

python3 "$HERE/sweet-ssh.py" "$MNT" "$SSH_OUT" "${KEYS[@]}"

sync
hdiutil detach "$MNT"
trap - EXIT

echo "regenerating sparse image: $OUT"
python3 "$IMG2SIMG" "$IMG" "$OUT"
shasum -a 256 "$OUT"
ls -lh "$IMG" "$OUT"
echo SSH_IMAGE_DONE
