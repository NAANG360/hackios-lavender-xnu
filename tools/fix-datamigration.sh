#!/bin/bash
# Mark the iOS 16.7.16 root image as already data-migrated, so datamigrator
# does not run on every boot. Mirrors the whyred v10 fix (2026-10-17).
#
# Usage (on macOS):  sudo bash fix-datamigration.sh [RAW_IMAGE] [SPARSE_OUT]
# Defaults:          ~/ios16-root.img -> ~/ios16-root-dm.simg
set -euo pipefail

USER_HOME="${HOME}"
if [ -n "${SUDO_USER:-}" ]; then
	USER_HOME="/Users/${SUDO_USER}"
fi
IMG="${1:-$USER_HOME/ios16-root.img}"
OUT="${2:-$USER_HOME/ios16-root-dm.simg}"
MNT=/tmp/iosroot-dm

if [ "$(id -u)" != "0" ]; then
	echo "run as root: sudo bash $0" >&2
	exit 1
fi
if [ ! -f "$IMG" ]; then
	echo "missing image: $IMG" >&2
	exit 1
fi
if [ ! -f "$USER_HOME/bergamota/tools/img2simg.py" ]; then
	echo "missing $USER_HOME/bergamota/tools/img2simg.py" >&2
	exit 1
fi

mkdir -p "$MNT"
if mount | grep -q "on $MNT "; then
	hdiutil detach "$MNT" >/dev/null 2>&1 || true
fi
trap 'hdiutil detach "$MNT" >/dev/null 2>&1 || true' EXIT

echo "attaching $IMG read-write at $MNT"
hdiutil attach -readwrite -nobrowse -mountpoint "$MNT" "$IMG"

if [ -d "$MNT/private/var" ]; then
	P="$MNT/private/var/mobile/Library/Preferences"
else
	P="$MNT/var/mobile/Library/Preferences"
fi
mkdir -p "$P"
chown 501:501 "$P" 2>/dev/null || true

if [ -f "$P/com.apple.DataMigration.plist" ]; then
	echo "existing plist:"
	plutil -p "$P/com.apple.DataMigration.plist" || true
fi

cat > "$P/com.apple.DataMigration.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>LastSystemVersion</key>
	<string>20H392</string>
</dict>
</plist>
PLIST

chown 501:501 "$P/com.apple.DataMigration.plist"
chmod 600 "$P/com.apple.DataMigration.plist"
plutil -lint "$P/com.apple.DataMigration.plist"
ls -l "$P/com.apple.DataMigration.plist"
plutil -p "$P/com.apple.DataMigration.plist"

sync
hdiutil detach "$MNT"
trap - EXIT

echo "regenerating sparse image: $OUT"
python3 "$USER_HOME/bergamota/tools/img2simg.py" "$IMG" "$OUT"
shasum -a 256 "$IMG" "$OUT"
ls -lh "$IMG" "$OUT"
echo DATAMIGRATION_FIX_DONE
