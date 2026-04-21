#!/usr/bin/env python3
"""
Convert a PNG image to a C byte array for embedded displays.

Modes:
  mono   — 1-bit monochrome for drawBitmap() (default)
  color  — 2-bit indexed (3 colors + transparent) with render function

Usage:
    python3 pngToArray.py <image.png>
    python3 pngToArray.py <image.png> --mode color
    python3 pngToArray.py <image.png> --mode color --name my_logo
    python3 pngToArray.py <image.png> --mode mono --alpha 64

Output goes to stdout. Analysis/stats go to stderr.

Requires: pip install Pillow
"""

import sys
import argparse
from PIL import Image


# =============================================================================
# Helpers
# =============================================================================

def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def format_array(data, per_line=21):
    lines = []
    for i in range(0, len(data), per_line):
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in data[i : i + per_line]) + ",")
    return "\n".join(lines)


# =============================================================================
# Mono mode (1-bit)
# =============================================================================

def generate_mono(img, name, alpha_threshold):
    img_rgba = img.convert("RGBA")
    w, h = img_rgba.size
    pixels = img_rgba.load()

    data = []
    for y in range(h):
        for bx in range((w + 7) // 8):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w:
                    r, g, b, a = pixels[x, y]
                    if a > alpha_threshold:
                        byte |= 1 << (7 - bit)
            data.append(byte)

    print(f"// Source: {img.filename}", file=sys.stderr)
    print(f"// Mode: 1-bit monochrome, {w}x{h}, {len(data)} bytes", file=sys.stderr)

    print(f"// {name} {w}x{h} — 1-bit monochrome, {len(data)} bytes")
    print(f"static const uint8_t {name}_data[] PROGMEM = {{")
    print(format_array(data))
    print("};")
    print(f"#define {name.upper()}_W {w}")
    print(f"#define {name.upper()}_H {h}")


# =============================================================================
# Color mode (2-bit indexed)
# =============================================================================

def classify_pixel(r, g, b, a, alpha_threshold):
    if a <= alpha_threshold:
        return 0
    max_c = max(r, g, b)
    min_c = min(r, g, b)
    sat = max_c - min_c
    if max_c < 60:
        return 1  # dark/text
    elif b > r and b > g and sat > 20:
        return 2  # blue
    elif r > g and sat > 20:
        return 3  # purple/magenta
    else:
        return 1  # fallback to dark


def analyze_colors(pixels, w, h, alpha_threshold):
    groups = {"dark": [], "blue": [], "purple": []}
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            idx = classify_pixel(r, g, b, a, alpha_threshold)
            if idx == 1:
                groups["dark"].append((r, g, b))
            elif idx == 2:
                groups["blue"].append((r, g, b))
            elif idx == 3:
                groups["purple"].append((r, g, b))
    return groups


def avg_color(px_list):
    if not px_list:
        return (0, 0, 0)
    n = len(px_list)
    return (sum(p[0] for p in px_list) // n,
            sum(p[1] for p in px_list) // n,
            sum(p[2] for p in px_list) // n)


def generate_color(img, name, alpha_threshold):
    img_rgba = img.convert("RGBA")
    w, h = img_rgba.size
    pixels = img_rgba.load()

    groups = analyze_colors(pixels, w, h, alpha_threshold)
    n_colors = sum(1 for g in groups.values() if g)
    blue_avg = avg_color(groups["blue"])
    purple_avg = avg_color(groups["purple"])

    print(f"// Source: {img.filename}", file=sys.stderr)
    print(f"// Mode: 2-bit indexed, {w}x{h}, {n_colors} color group(s)", file=sys.stderr)
    if groups["dark"]:
        print(f"//   Text/dark: {len(groups['dark'])} px", file=sys.stderr)
    if groups["blue"]:
        print(f"//   Blue:      {len(groups['blue'])} px, avg RGB{blue_avg} -> 0x{rgb565(*blue_avg):04X}", file=sys.stderr)
    if groups["purple"]:
        print(f"//   Purple:    {len(groups['purple'])} px, avg RGB{purple_avg} -> 0x{rgb565(*purple_avg):04X}", file=sys.stderr)

    # Encode 2-bit
    data = []
    for y in range(h):
        for bx in range((w + 3) // 4):
            byte = 0
            for pix in range(4):
                x = bx * 4 + pix
                if x < w:
                    r, g, b, a = pixels[x, y]
                    idx = classify_pixel(r, g, b, a, alpha_threshold)
                    byte |= idx << (6 - pix * 2)
            data.append(byte)

    N = name.upper()

    print(f"// {name} {w}x{h} — 2-bit indexed, {n_colors} colors, {len(data)} bytes")
    print(f"// Palette: 0=transparent, 1=text, 2=blue, 3=purple")
    print(f"static const uint8_t {name}_data[] PROGMEM = {{")
    print(format_array(data, per_line=18))
    print("};")
    print(f"#define {N}_W {w}")
    print(f"#define {N}_H {h}")

    if groups["blue"]:
        c = rgb565(*blue_avg)
        print(f"#define {N}_COLOR_BLUE   0x{c:04X}  // RGB{blue_avg}")
    if groups["purple"]:
        c = rgb565(*purple_avg)
        print(f"#define {N}_COLOR_PURPLE 0x{c:04X}  // RGB{purple_avg}")

    # Usage example (uses gfx_draw_indexed_bitmap from lib_display)
    palette_entries = ["0", f"{N}_COLOR_TEXT"]
    palette_entries.append(f"{N}_COLOR_BLUE" if groups["blue"] else "0")
    palette_entries.append(f"{N}_COLOR_PURPLE" if groups["purple"] else "0")

    print()
    print(f"#ifndef {N}_COLOR_TEXT")
    print(f"#define {N}_COLOR_TEXT 0xFFFF  // white (override before including)")
    print(f"#endif")
    print()
    print(f"// Build the palette array and call gfx_draw_indexed_bitmap():")
    print(f"//   #include <display/gfx_core.h>")
    print(f"//   static const uint16_t {name}_palette[] = {{ {', '.join(palette_entries)} }};")
    print(f"//   gfx_draw_indexed_bitmap(x, y, {name}_data, {N}_W, {N}_H, {name}_palette);")


# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Convert PNG to C byte array for embedded displays."
    )
    parser.add_argument("image", help="Input PNG file")
    parser.add_argument(
        "--mode", choices=["mono", "color"], default="mono",
        help="Output mode: mono (1-bit) or color (2-bit indexed). Default: mono"
    )
    parser.add_argument(
        "--alpha", type=int, default=32,
        help="Alpha threshold (0-255). Pixels with alpha <= this are transparent. Default: 32"
    )
    parser.add_argument(
        "--name", default="logo",
        help="C identifier prefix for arrays and defines. Default: logo"
    )
    args = parser.parse_args()

    img = Image.open(args.image)

    if args.mode == "mono":
        generate_mono(img, args.name, args.alpha)
    else:
        generate_color(img, args.name, args.alpha)


if __name__ == "__main__":
    main()
