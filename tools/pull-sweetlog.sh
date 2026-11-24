#!/sbin/sh
# Dump the SANTONILOG1 area from the SYSTEM partition (patches 0143/0178).
# Run inside TWRP: sh /tmp/pull-sweetlog.sh
# Then pull /tmp/sweetlog.bin and decode it with tools/sweet-log.py.
set -e

OUT=/tmp/sweetlog.bin
DEV=/dev/block/bootdevice/by-name/system
[ -e "$DEV" ] || DEV=/dev/block/by-name/system

dd if="$DEV" of="$OUT" bs=512 count=8193
sync
ls -l "$OUT"
echo "DONE. Pull /tmp/sweetlog.bin to the PC."
