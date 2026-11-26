#!/usr/bin/env bash
set -euo pipefail
IPSW="$(realpath "${1:?usage: build-ios-root-linux.sh iPhone10,3_16.7.16.ipsw OUTDIR [SIZE_MIB] [SWEETGL_DYLIB]}")"
OUT="$(mkdir -p "${2:?output directory required}" && cd "$2" && pwd)"
SIZE_MIB="${3:-22208}"
SWEETGL_DYLIB="${4:-${SWEETGL_DYLIB:-}}"
LOGICAL_BYTES=$((SIZE_MIB * 1024 * 1024))
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXTRACT="$OUT/extracted"
RAW="$OUT/ios16-root.img"
SPARSE="$OUT/ios16-root.simg"
SPARSE_ZST="$OUT/ios16-root.simg.zst"
MNT="$OUT/root-mount"
mkdir -p "$EXTRACT/fs" "$EXTRACT/sys" "$EXTRACT/app"

find_one_dmg() {
  local dir="$1"
  mapfile -t files < <(find "$dir" -type f -iname '*.dmg' -print)
  if [ "${#files[@]}" -ne 1 ]; then
    echo "expected exactly one DMG under $dir, found ${#files[@]}" >&2
    find "$dir" -type f -maxdepth 4 -print >&2 || true
    exit 1
  fi
  printf '%s\n' "${files[0]}"
}

echo "Extracting iPhone10,3 iOS 16.7.16 filesystem images"
ipsw extract --dmg fs --device iPhone10,3 --output "$EXTRACT/fs" "$IPSW"
ipsw extract --dmg sys --device iPhone10,3 --output "$EXTRACT/sys" "$IPSW"
ipsw extract --dmg app --device iPhone10,3 --output "$EXTRACT/app" "$IPSW"
OS_DMG="$(find_one_dmg "$EXTRACT/fs")"
SYS_DMG="$(find_one_dmg "$EXTRACT/sys")"
APP_DMG="$(find_one_dmg "$EXTRACT/app")"
ls -lh "$OS_DMG" "$SYS_DMG" "$APP_DMG"

# Free the 6.17 GB restore archive before constructing the 16 GiB HFSX volume.
rm -f "$IPSW"
df -h "$OUT"

test ! -e "$RAW" || { echo "refusing to overwrite $RAW" >&2; exit 1; }
sudo bash "$ROOT/tools/mkiosroot.sh" "$OS_DMG" "$SYS_DMG" "$APP_DMG" "$RAW" "${SIZE_MIB}M"
rm -rf "$EXTRACT"

# Apply the tested first-boot/data-migration changes used by the
# reference lock-screen build, then cleanly verify the journaled HFSX volume.
sudo mkdir -p "$MNT"
sudo mount -t hfsplus -o loop,force "$RAW" "$MNT"
SWEETGL_SHA=""
SWEETGL_SIZE="0"
if [ -n "$SWEETGL_DYLIB" ]; then
  test -s "$SWEETGL_DYLIB" || { echo "SWEETGL_DYLIB missing or empty: $SWEETGL_DYLIB" >&2; sudo umount "$MNT"; exit 1; }
  sudo mkdir -p "$MNT/usr/lib"
  sudo install -m 0755 -o 0 -g 0 "$SWEETGL_DYLIB" "$MNT/usr/lib/sweetgl.dylib"
  SWEETGL_SHA="$(sha256sum "$SWEETGL_DYLIB" | awk '{print $1}')"
  SWEETGL_SIZE="$(stat -c%s "$SWEETGL_DYLIB")"
  echo "Installed /usr/lib/sweetgl.dylib ($SWEETGL_SIZE bytes, sha256=$SWEETGL_SHA)"
fi
sudo python3 "$ROOT/tools/sweet-root-tweaks.py" "$MNT"
sync
sudo umount "$MNT"
sudo fsck.hfsplus -f -n "$RAW"

# Android sparse format is the flashable representation for the userdata
# partition; validate logical size matches SIZE_MIB.
img2simg "$RAW" "$SPARSE" 4096
simg_dump "$SPARSE" > "$OUT/ios16-root.sparse.txt"
python3 - "$SPARSE" "$LOGICAL_BYTES" <<'PY'
import os, struct, sys
path, expected_logical = sys.argv[1], int(sys.argv[2])
with open(path, "rb") as f:
    h = f.read(28)
    if len(h) != 28: raise SystemExit("short Android sparse header")
    magic, major, minor, fh, ch, blk, total, chunk_count, checksum = struct.unpack("<IHHHHIIII", h)
    if magic != 0xed26ff3a or major != 1 or fh < 28 or ch < 12 or blk != 4096:
        raise SystemExit("invalid Android sparse image header")
    if total * blk != expected_logical:
        raise SystemExit(f"wrong logical userdata image size: {total * blk} != {expected_logical}")
    f.seek(0, os.SEEK_END)
    size = f.tell()
    f.seek(fh)
    blocks = 0
    for i in range(chunk_count):
        head = f.read(ch)
        if len(head) != ch: raise SystemExit(f"truncated sparse chunk header {i}")
        typ, reserved, nblocks, tsize = struct.unpack("<HHII", head[:12])
        if typ == 0xCAC1: expected = ch + nblocks * blk; blocks += nblocks
        elif typ == 0xCAC2: expected = ch + 4; blocks += nblocks
        elif typ == 0xCAC3: expected = ch; blocks += nblocks
        elif typ == 0xCAC4: expected = ch + 4
        else: raise SystemExit(f"unknown sparse chunk type {typ:#x}")
        if tsize != expected: raise SystemExit(f"bad sparse chunk {i} size {tsize}, expected {expected}")
        f.seek(tsize - ch, 1)
    if blocks != total: raise SystemExit(f"sparse block count mismatch {blocks} != {total}")
    if f.tell() != size: raise SystemExit(f"sparse file length mismatch {f.tell()} != {size}")
print(f"verified Android sparse HFSX image: {size} bytes; logical size {total * blk} bytes")
PY

RAW_SHA="$(sha256sum "$RAW" | awk '{print $1}')"
SPARSE_SHA="$(sha256sum "$SPARSE" | awk '{print $1}')"
SPARSE_SIZE="$(stat -c%s "$SPARSE")"
rm -f "$RAW"

# Compress the sparse image with zstd for practical downloads over slower networks.
echo "Compressing ios16-root.simg with zstd..."
zstd -T0 -12 "$SPARSE" -o "$SPARSE_ZST"
zstd -t "$SPARSE_ZST"
ZST_SHA="$(sha256sum "$SPARSE_ZST" | awk '{print $1}')"
ZST_SIZE="$(stat -c%s "$SPARSE_ZST")"
rm -f "$SPARSE"

python3 - "$OUT/root-manifest.json" "$RAW_SHA" "$SPARSE_SHA" "$SPARSE_SIZE" "$ZST_SHA" "$ZST_SIZE" "$LOGICAL_BYTES" "$SWEETGL_SHA" "$SWEETGL_SIZE" <<'PY'
import json, sys
path, raw_sha, sparse_sha, sparse_size, zst_sha, zst_size, logical_bytes, sweetgl_sha, sweetgl_size = sys.argv[1:]
m = {
    "schema": 2,
    "device": "Xiaomi Redmi Note 7 / 7S",
    "codename": "lavender",
    "userland": "iOS 16.7.16 (20H392), iPhone10,3",
    "filesystem": "HFSX (case-sensitive, journaled HFS+)",
    "logical_size_bytes": int(logical_bytes),
    "partition_target": "userdata",
    "xnu_root_device": "emmcroot0",
    "image": "ios16-root.simg.zst",
    "uncompressed_image": "ios16-root.simg",
    "image_format": "zstd-compressed Android sparse image v1",
    "compressed_size_bytes": int(zst_size),
    "compressed_sha256": zst_sha,
    "sparse_size_bytes": int(sparse_size),
    "sparse_sha256": sparse_sha,
    "raw_hfsx_sha256": raw_sha,
    "source_ipsw_sha256": "ece1194f7e233e9109c5790a8d23a4564a10a254e15ec5b09978a343265568fb",
    "first_boot_tweaks": "sweet-root-tweaks.py (reference lock-screen profile)",
    "sweetgl_bundled": bool(sweetgl_sha),
    "sweetgl_sha256": sweetgl_sha or None,
    "sweetgl_size_bytes": int(sweetgl_size) if sweetgl_sha else 0,
}
with open(path, "w") as f:
    json.dump(m, f, indent=2)
    f.write("\n")
PY
{
  printf '%s  ios16-root.simg.zst\n' "$ZST_SHA"
  printf '# uncompressed sparse SHA-256: %s  ios16-root.simg\n' "$SPARSE_SHA"
} > "$OUT/ROOT-SHA256SUMS"
echo "iOS userdata root image ready: $ZST_SIZE bytes compressed ($SPARSE_SIZE bytes sparse, $LOGICAL_BYTES bytes logical)"
