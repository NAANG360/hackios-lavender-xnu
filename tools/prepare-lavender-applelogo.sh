#!/usr/bin/env bash
# Fetch the exact AppleLogo resource from iPhone X iOS 16.7.16 and generate a
# temporary C header in the XNU build tree. No firmware blob is committed to Git.
set -euo pipefail
OUT_HEADER="\${1:?usage: prepare-lavender-applelogo.sh <xnu-header-output-path>}"
ROOT="$(cd "$(dirname "\${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d "\${TMPDIR:-/tmp}/lavender-applelogo.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT
VERSION="3.1.748"
TOOL_ASSET="ipsw_\${VERSION}_macOS_x86_64.tar.gz"
TOOL_BASE="https://github.com/blacktop/ipsw/releases/download/v\${VERSION}"
IPSW_URL="https://updates.cdn-apple.com/2025FallFCS/fullrestores/122-77006/1158E502-D76D-4226-AA06-ED5E30CD1111/iPhone10,3,iPhone10,6_16.7.16_20H392_Restore.ipsw"
EXPECTED_IM4P_SHA="93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8"
mkdir -p "$TMP/bin" "$TMP/source" "$TMP/decoded"
curl -fL --retry 4 --retry-delay 3 "$TOOL_BASE/checksums.txt" -o "$TMP/checksums.txt"
curl -fL --retry 4 --retry-delay 3 "$TOOL_BASE/$TOOL_ASSET" -o "$TMP/$TOOL_ASSET"
EXPECTED_TOOL_SHA="$(awk -v name="$TOOL_ASSET" '$2 == name {print $1}' "$TMP/checksums.txt")"
test -n "$EXPECTED_TOOL_SHA"
printf '%s  %s\n' "$EXPECTED_TOOL_SHA" "$TMP/$TOOL_ASSET" | shasum -a 256 -c -
tar -xzf "$TMP/$TOOL_ASSET" -C "$TMP/bin"
chmod +x "$TMP/bin/ipsw"
# Range-extract only the AppleLogo IM4P; remotezip validates the member CRC.
python3 "$ROOT/tools/remotezip.py" "$IPSW_URL" applelogo "$TMP/source"
IM4P="$(find "$TMP/source" -type f -iname 'applelogo*.im4p' -print -quit)"
test -n "$IM4P"
ACTUAL_IM4P_SHA="$(shasum -a 256 "$IM4P" | awk '{print $1}')"
test "$ACTUAL_IM4P_SHA" = "$EXPECTED_IM4P_SHA"
"$TMP/bin/ipsw" fw ibootim "$IM4P" --output "$TMP/decoded"
PNG="$(find "$TMP/decoded" -type f -iname '*.png' -print -quit)"
test -n "$PNG"
python3 -m venv "$TMP/venv"
"$TMP/venv/bin/pip" install --quiet --disable-pip-version-check Pillow==11.3.0
"$TMP/venv/bin/python" "$ROOT/tools/pack_lavender_applelogo.py" "$PNG" "$OUT_HEADER" \
    --source-im4p-sha256 "$ACTUAL_IM4P_SHA"
