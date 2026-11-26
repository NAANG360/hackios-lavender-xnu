#!/bin/sh
# Build sweetgl.dylib (Mesa freedreno a512 + GLES + EAGL shim) for iOS 16 arm64.
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$H/../..
M=$R/research/mesa
B=$R/research/mesa-ios
LIBS="src/mesa/libmesa.a src/gallium/auxiliary/libgallium.a
	src/gallium/drivers/freedreno/libfreedreno.a src/gallium/winsys/freedreno/drm/libfreedrenowinsys.a
	src/freedreno/drm/libfreedreno_drm.a src/freedreno/ir3/libfreedreno_ir3.a
	src/freedreno/ir2/libfreedreno_ir2.a src/freedreno/fdl/libfreedreno_layout.a
	src/freedreno/common/libfreedreno_common.a src/freedreno/isa/libir3encode.a
	src/freedreno/isa/libir3decode.a src/compiler/isaspec/libisaspec.a
	src/freedreno/perfcntrs/libfreedreno_perfcntrs.a
	src/compiler/glsl/libglsl.a src/compiler/glsl/glcpp/libglcpp.a src/compiler/nir/libnir.a
	src/compiler/spirv/libvtn.a src/compiler/libcompiler.a
	src/mapi/shared-glapi/libglapi.a
	src/util/libxmlconfig.a src/util/libmesa_util.a src/util/libmesa_util_simd.a src/util/blake3/libblake3.a
	src/c11/impl/libmesa_util_c11.a"
(cd "$B" && ninja $LIBS)
O=$H/out
mkdir -p "$O"
nm -g "$B/src/mapi/shared-glapi/libglapi.a" > "$O/bridge.nm"
CC="xcrun -sdk iphoneos clang -arch arm64 -miphoneos-version-min=16.0"
INC="-I$H/include -I$M/include -I$M/src -I$M/src/mesa -I$M/src/mapi -I$M/src/gallium/include
	-I$M/src/gallium/auxiliary -I$M/src/gallium/drivers -I$M/src/gallium/winsys
	-I$B/src -I$B/src/mesa -I$B/src/mapi"
python3 -I -c '
import json, shlex, sys
cc = json.load(open(sys.argv[1]))
e = [x for x in cc if x["file"].endswith("state_tracker/st_manager.c")][0]
print("\n".join(shlex.quote(a) for a in shlex.split(e["command"]) if a.startswith("-D")))
' "$B/compile_commands.json" > "$O/mesa-defines.rsp"
python3 -I "$H/gen-dispatch.py" "$H/ios-dispatch.txt" "$M/include" "$O/bridge.nm" "$O/dispatch.c"
for f in sweetgl sweet_drm eagl layerdump "$O/dispatch"; do
	case $f in /*) src=$f.c ;; *) src=$H/$f.c ;; esac
	$CC -O2 -Wall -fvisibility=hidden @"$O/mesa-defines.rsp" $INC -I"$H" -c "$src" -o "$O/$(basename "$f").o"
done
printf '_sweetgl_init\n_sweetgl_create\n_sweetgl_make_current\n_sweetgl_destroy\n_sweetgl_selftest\n' > "$O/exports.txt"
xcrun -sdk iphoneos clang++ -arch arm64 -miphoneos-version-min=16.0 -dynamiclib -dead_strip \
	-install_name /usr/lib/sweetgl.dylib -exported_symbols_list "$O/exports.txt" \
	-o "$O/sweetgl.dylib" "$O/sweetgl.o" "$O/sweet_drm.o" "$O/eagl.o" "$O/layerdump.o" "$O/dispatch.o" $(for l in $LIBS; do echo "$B/$l"; done) \
	-framework IOKit -framework CoreFoundation -framework IOSurface -framework OpenGLES -framework CoreGraphics -lobjc -lc++ -lz
codesign -f -s - "$O/sweetgl.dylib"
echo "built $O/sweetgl.dylib"
