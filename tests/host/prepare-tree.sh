#!/bin/bash
# Rebuild the patched driver sources the host tests compile:
#   tests/host/prepare-tree.sh [out-dir]
#
# Applies this repository's full series (xnu/patches/*.patch, in order) with
# git apply, but only for bsd/dev/sweet_* and bsd/dev/santoni_sdhci.c. No XNU
# checkout is needed. CR line endings (a Windows checkout without the
# .gitattributes rules) are stripped first; git apply would reject them.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
out="${1:-$here/out/tree}"

if [ -e "$out/bsd" ]; then
	echo "$out already holds a tree; pass a new directory" >&2
	exit 1
fi
mkdir -p "$out"
series="$(mktemp)"
trap 'rm -f "$series"' EXIT
for p in "$repo"/xnu/patches/*.patch; do
	# C locale: BSD tr (macOS) rejects patch bytes that are not valid UTF-8.
	LC_ALL=C tr -d '\r' < "$p" >> "$series"
done
if [ ! -d "$out/.git" ]; then
	git -C "$out" init -q
	git -C "$out" config core.autocrlf false
fi
git -C "$out" apply --include='bsd/dev/sweet_*' --include='bsd/dev/santoni_sdhci.c' \
	--whitespace=nowarn "$series"
echo "patched sources in $out/bsd/dev"
