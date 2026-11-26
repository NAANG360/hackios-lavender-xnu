#!/bin/sh
# Build sweetgl.dylib (Mesa freedreno a512 + GLES + EAGL shim) for iOS 16 arm64.
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$H/../..
M=$R/research/mesa
B=$R/research/mesa-ios
TARGETS=$(cd "$B" && ninja -t targets all | awk -F: '$1 ~ /^src\/(mesa|gallium|freedreno|compiler|mapi|util|c11)\/.*\.a$/ && $1 !~ /test|mock/ {print $1}' | sort -u)
test -n "$TARGETS" || { echo "no Mesa static archive targets found in $B" >&2; exit 1; }
(cd "$B" && ninja $TARGETS)
O=$H/out
mkdir -p "$O"
ARCHIVE_DIRS="src/mesa src/gallium src/freedreno src/compiler src/mapi src/util src/c11"
LIBS=""
for d in $ARCHIVE_DIRS; do
	if [ -d "$B/$d" ]; then
		for a in $(find "$B/$d" -name '*.a' | sort); do
			case "$a" in
				*test*|*mock*) ;;
				*) LIBS="$LIBS $a" ;;
			esac
		done
	fi
done
test -n "$LIBS" || { echo "no Mesa static archives built in $B" >&2; exit 1; }
nm -g $LIBS > "$O/bridge.nm"
CC="xcrun -sdk iphoneos clang -arch arm64 -miphoneos-version-min=16.0"
INC="-I$H/include -I$M/include -I$M/src -I$M/src/mesa -I$M/src/mapi -I$M/src/gallium/include
	-I$M/src/gallium/auxiliary -I$M/src/gallium/drivers -I$M/src/gallium/winsys
	-I$B/src -I$B/src/mesa -I$B/src/mapi"
python3 -I -c '
import json, shlex, sys
cc = json.load(open(sys.argv[1]))
e = [x for x in cc if x["file"].endswith("state_tracker/st_manager.c")]
if not e:
	e = [x for x in cc if x["file"].endswith("freedreno_screen.c")]
print("\n".join(shlex.quote(a) for a in shlex.split(e[0]["command"]) if a.startswith("-D")))
' "$B/compile_commands.json" > "$O/mesa-defines.rsp"
python3 -I "$H/gen-dispatch.py" "$H/ios-dispatch.txt" "$M/include" "$O/bridge.nm" "$O/dispatch.c"
for f in sweetgl sweet_drm eagl layerdump "$O/dispatch"; do
	case $f in /*) src=$f.c ;; *) src=$H/$f.c ;; esac
	$CC -O2 -Wall -fvisibility=hidden @"$O/mesa-defines.rsp" $INC -I"$H" -c "$src" -o "$O/$(basename "$f").o"
done
printf '_sweetgl_init\n_sweetgl_create\n_sweetgl_make_current\n_sweetgl_destroy\n_sweetgl_selftest\n' > "$O/exports.txt"
xcrun -sdk iphoneos clang++ -arch arm64 -miphoneos-version-min=16.0 -dynamiclib -dead_strip \
	-install_name /usr/lib/sweetgl.dylib -exported_symbols_list "$O/exports.txt" \
	-o "$O/sweetgl.dylib" "$O/sweetgl.o" "$O/sweet_drm.o" "$O/eagl.o" "$O/layerdump.o" "$O/dispatch.o" $LIBS \
	-framework IOKit -framework CoreFoundation -framework IOSurface -framework OpenGLES -framework CoreGraphics -lobjc -lc++ -lz
codesign -f -s - "$O/sweetgl.dylib"
echo "built $O/sweetgl.dylib"
