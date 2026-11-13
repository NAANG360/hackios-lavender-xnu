#!/bin/bash
# Build the ARM64 shim. --compile-only avoids packaging a boot.img when the
# stock device DTB is not present; this is the CI validation mode.
set -euo pipefail
cd "$(dirname "$0")"
TARGET="${TARGET:-sweet}"
if [[ "$TARGET" == "lavender" ]]; then
  OS_VERSION="${OS_VERSION:-15.0.0}"
  OS_PATCH_LEVEL="${OS_PATCH_LEVEL:-2025-03}"
else
  OS_VERSION="${OS_VERSION:-17.0.0}"
  OS_PATCH_LEVEL="${OS_PATCH_LEVEL:-2035-06}"
fi
COMPILE_ONLY=0
PAYLOAD=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --target) TARGET="$2"; shift 2 ;;
    --compile-only) COMPILE_ONLY=1; shift ;;
    *)
      if [[ -n "$PAYLOAD" ]]; then echo "unexpected argument: $1" >&2; exit 2; fi
      PAYLOAD="$1"; shift ;;
  esac
done
case "$TARGET" in
  sweet) SOURCES=(start.S shim.c console.c mmu.c fdt.c loader.c ufs.c) ;;
  lavender) SOURCES=(start.S shim.c console.c mmu.c fdt.c loader.c lavender_guardian.c) ;;
  *) echo "unsupported shim target: $TARGET" >&2; exit 2 ;;
esac
CC=aarch64-linux-gnu-gcc
OBJCOPY=aarch64-linux-gnu-objcopy
CFLAGS=(-O2 -ffreestanding -nostdlib -fno-pic -mgeneral-regs-only -mstrict-align -fno-stack-protector -Wall -Wextra)
HOST_CFLAGS=(-O1 -Wall)
if [[ "$TARGET" == "lavender" ]]; then
  CFLAGS+=(-DTARGET_LAVENDER=1)
  HOST_CFLAGS+=(-DTARGET_LAVENDER=1)
fi
mkdir -p out
OBJECTS=()
for f in "${SOURCES[@]}"; do
  "$CC" "${CFLAGS[@]}" -c "$f" -o "out/${f%.*}.o"
  OBJECTS+=("out/${f%.*}.o")
done
# The image runs wherever ABL puts it with no relocation pass: only the
# header's image_size (start.o, linked at 0) may be an absolute address.
reloc_fail=0
for obj in "${OBJECTS[@]}"; do
  # start.S deliberately stores _end as an ABS64 image-size field in its
  # header; all C objects must remain position-independent.
  [[ "$obj" == "out/start.o" ]] && continue
  bad=$(aarch64-linux-gnu-readelf -r "$obj" | grep ABS64 || true)
  if [[ -n "$bad" ]]; then
    echo "error: absolute relocation in C object $obj" >&2
    echo "$bad" >&2
    reloc_fail=1
  fi
done
[[ "$reloc_fail" == 0 ]] || exit 1
"$CC" "${CFLAGS[@]}" -static -T shim.ld -Wl,--build-id=none -Wl,--no-warn-rwx-segments -Wl,-N   "${OBJECTS[@]}" -o out/shim.elf
"$OBJCOPY" -O binary out/shim.elf out/Image
# ABL copies the file; pad to image_size so .bss/stacks exist as zeroed bytes.
size_hex=$(aarch64-linux-gnu-nm out/shim.elf | awk '$3=="_end"{print $1}')
[[ -n "$size_hex" ]] || { echo "could not resolve _end in shim.elf" >&2; exit 1; }
size=$((16#$size_hex))
truncate -s "$size" out/Image
# Host render of the console layout.
gcc "${HOST_CFLAGS[@]}" -o out/host_test console.c host_test.c
./out/host_test
if [[ "$COMPILE_ONLY" == 1 ]]; then
  echo "${TARGET} shim compile-only succeeded; no boot.img was packaged."
  ls -l out/Image out/shim.elf out/host.ppm
  exit 0
fi
# Ramdisk: an XNU payload (tools/mkpayload.py) if given as $1, else an empty
# newc cpio (ABL dislikes a zero-size ramdisk on some builds) -> diagnostics.
if [[ -n "$PAYLOAD" ]]; then
  cp "$PAYLOAD" out/ramdisk.bin
else
  (cd out && rm -rf rd && mkdir rd && cd rd && find . | cpio -o -H newc --quiet | gzip -9 > ../ramdisk.bin)
fi
DTB_PATH="${DTB_PATH:-../dtb/boot_unpacked/dtb}"
[[ -f "$DTB_PATH" ]] || { echo "missing DTB: $DTB_PATH (set DTB_PATH to a stock Lavender DTB)" >&2; exit 1; }
# Place the DTB after the complete ramdisk, rounded up to a 2 MiB
# boundary. The previous fixed 0x1f00000 address overlapped the 15.2 MiB
# Lavender payload ramdisk loaded at 0x01000000.
RAMDISK_ADDR=$((0x1000000))
RAMDISK_SIZE=$(stat -c %s out/ramdisk.bin)
RAMDISK_END=$((RAMDISK_ADDR + RAMDISK_SIZE))
DTB_ADDR=$(((RAMDISK_END + 0x1fffff) & ~0x1fffff))
printf 'RAMDISK: [%#x, %#x) (%s bytes)\n' "$RAMDISK_ADDR" "$RAMDISK_END" "$RAMDISK_SIZE"
printf 'DTB:     [%#x, %#x)\n' "$DTB_ADDR" "$((DTB_ADDR + $(stat -c %s "$DTB_PATH")))"
mkbootimg --header_version 2 --kernel out/Image --ramdisk out/ramdisk.bin \
  --dtb "$DTB_PATH" --base 0 --kernel_offset 0x8000 --ramdisk_offset "$RAMDISK_ADDR" \
  --tags_offset 0x100 --dtb_offset "$DTB_ADDR" --pagesize 4096 \
  --os_version "$OS_VERSION" --os_patch_level "$OS_PATCH_LEVEL" \
  --cmdline "console=null" -o out/shim-boot.img
ls -l out/Image out/shim-boot.img
