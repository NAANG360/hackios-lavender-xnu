#!/usr/bin/env python3
"""Convert the pinned 20H392 AppleLogo RGBA PNG to a compact XNU C mask."""
from __future__ import annotations
import argparse, hashlib, sys
from pathlib import Path
from PIL import Image
EXPECTED_W, EXPECTED_H = 194, 238
EXPECTED_PNG = "9c1af53fd36eb86021657bd8e94df4f54b1e407a74cbcfd37f60aff63c604bdb"
EXPECTED_IM4P = "93e6586cdd85ee2208469d8ca4052a27bb5465b2f142c52d0e00286bda7d50b8"
def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("png", type=Path); p.add_argument("output", type=Path)
    p.add_argument("--source-im4p-sha256", required=True)
    a = p.parse_args()
    try:
        raw = a.png.read_bytes()
        png_sha = hashlib.sha256(raw).hexdigest()
        if png_sha != EXPECTED_PNG: raise ValueError(f"unexpected AppleLogo PNG SHA-256: {png_sha}")
        if a.source_im4p_sha256 != EXPECTED_IM4P: raise ValueError("unexpected AppleLogo IM4P SHA-256")
        with Image.open(a.png) as image:
            rgba = image.convert("RGBA")
            if rgba.size != (EXPECTED_W, EXPECTED_H): raise ValueError(f"unexpected dimensions: {rgba.size}")
            mask = bytearray(EXPECTED_W * EXPECTED_H)
            for i, (r,g,b,alpha) in enumerate(rgba.getdata()):
                if alpha and not (r == g == b == alpha):
                    raise ValueError(f"non-premultiplied white-mask pixel at {i % EXPECTED_W},{i // EXPECTED_W}")
                mask[i] = alpha
        lines = [
            "/* Auto-generated from iPhone X iOS 16.7.16 (20H392) AppleLogo IM4P.",
            f" * Source IM4P SHA-256: {a.source_im4p_sha256}",
            f" * Decoded PNG SHA-256: {png_sha}",
            " * Regenerate with tools/prepare-lavender-applelogo.sh. */",
            "#pragma once", "#include <stdint.h>",
            f"#define LAVENDER_APPLELOGO_WIDTH {EXPECTED_W}u",
            f"#define LAVENDER_APPLELOGO_HEIGHT {EXPECTED_H}u",
            f"static const uint8_t lavender_applelogo_luma[{len(mask)}u] = {{",
        ]
        for off in range(0, len(mask), 16):
            lines.append("    " + ", ".join(f"0x{x:02x}" for x in mask[off:off+16]) + ",")
        lines += ["};", ""]
        a.output.parent.mkdir(parents=True, exist_ok=True)
        a.output.write_text("\n".join(lines), encoding="ascii")
        print(f"PASS: genuine 20H392 AppleLogo {EXPECTED_W}x{EXPECTED_H}; {len(mask)} antialiased pixels")
        print(f"PNG SHA-256: {png_sha}; IM4P SHA-256: {a.source_im4p_sha256}")
        return 0
    except (OSError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr); return 1
if __name__ == "__main__": raise SystemExit(main())
