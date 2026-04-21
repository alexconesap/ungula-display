#!/usr/bin/env python3
"""
Generate a base64-encoded BMP logo for the web portal from the C bitmap array.

Reads the monochrome logo bitmap from app_screens.cpp (the same one rendered
on the embedded display), converts it to a 1-bit BMP with configurable
foreground/background colors, and outputs a base64 data URI ready to paste
into the web portal HTML.

The embedded logo is stored as a packed 1-bit array where 1 = foreground pixel.
This script wraps it in a proper BMP file header so browsers can render it.

Usage:
    python3 tools/generate_web_logo.py

    # Custom colors (hex RGB, no #):
    python3 tools/generate_web_logo.py --bg 16213e --fg ffffff

    # Save to file instead of printing:
    python3 tools/generate_web_logo.py --output /tmp/logo.txt

The output is a single line of base64 text. To use it in HTML:
    <img src="data:image/bmp;base64,OUTPUT_HERE" alt="Logo">

Notes for our own projects (not for the public library usage):
    - Logo dimensions (255x60) are read from the #define LOGO_W / LOGO_H
      in app_screens.cpp. If the logo changes size, this script adapts.
    - BMP rows are 4-byte aligned as required by the format.
    - The output is ~2.6 KB of base64, adding negligible flash usage.
"""

import argparse
import base64
import os
import re
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
APP_SCREENS = os.path.join(PROJECT_DIR, "ICB", "app_screens.cpp")


def parse_logo_bytes(source_path):
    """Extract the logo byte array from the C source file."""
    with open(source_path, "r") as f:
        content = f.read()

    start = content.find("logo[] PROGMEM = {")
    if start < 0:
        print(f"Error: 'logo[] PROGMEM' not found in {source_path}", file=sys.stderr)
        sys.exit(1)

    start += len("logo[] PROGMEM = {")
    end = content.find("};", start)
    hex_data = content[start:end]

    values = [int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", hex_data)]
    if not values:
        print("Error: no hex bytes found in logo array", file=sys.stderr)
        sys.exit(1)

    return values


def parse_logo_dimensions(source_path):
    """Read LOGO_W and LOGO_H from #define directives."""
    with open(source_path, "r") as f:
        content = f.read()

    w_match = re.search(r"#define\s+LOGO_W\s+(\d+)", content)
    h_match = re.search(r"#define\s+LOGO_H\s+(\d+)", content)

    if not w_match or not h_match:
        print("Error: LOGO_W or LOGO_H not found", file=sys.stderr)
        sys.exit(1)

    return int(w_match.group(1)), int(h_match.group(1))


def hex_to_bgr(hex_str):
    """Convert a hex RGB string (e.g., 'ff8800') to BGR bytes for BMP color table."""
    r = int(hex_str[0:2], 16)
    g = int(hex_str[2:4], 16)
    b = int(hex_str[4:6], 16)
    return struct.pack("<BBBB", b, g, r, 0x00)


def build_bmp(logo_bytes, width, height, bg_hex, fg_hex):
    """Build a 1-bit BMP file from the raw monochrome bitmap data.

    In the source bitmap, bit=1 means foreground pixel. In BMP 1-bit format,
    bit=0 maps to color table index 0 (background) and bit=1 to index 1
    (foreground), so no bit inversion is needed.

    BMP stores rows bottom-to-top, each row padded to a 4-byte boundary.
    """
    row_bytes_src = (width + 7) // 8
    row_bytes_bmp = ((width + 31) // 32) * 4

    # Build pixel data (bottom-to-top row order for BMP)
    pixel_data = bytearray()
    for y in range(height - 1, -1, -1):
        row_start = y * row_bytes_src
        row = logo_bytes[row_start : row_start + row_bytes_src]
        # Pad row to BMP 4-byte alignment
        row += [0x00] * (row_bytes_bmp - len(row))
        pixel_data.extend(row)

    # Color table: index 0 = background, index 1 = foreground
    color_table = hex_to_bgr(bg_hex) + hex_to_bgr(fg_hex)

    header_size = 14 + 40 + len(color_table)
    file_size = header_size + len(pixel_data)

    bmp = bytearray()

    # BMP file header (14 bytes)
    bmp += b"BM"
    bmp += struct.pack("<I", file_size)
    bmp += struct.pack("<HH", 0, 0)
    bmp += struct.pack("<I", header_size)

    # DIB header — BITMAPINFOHEADER (40 bytes)
    bmp += struct.pack("<I", 40)
    bmp += struct.pack("<i", width)
    bmp += struct.pack("<i", height)
    bmp += struct.pack("<HH", 1, 1)  # planes=1, bpp=1
    bmp += struct.pack("<I", 0)  # no compression
    bmp += struct.pack("<I", len(pixel_data))
    bmp += struct.pack("<ii", 2835, 2835)  # ~72 DPI
    bmp += struct.pack("<II", 2, 2)  # 2 colors used

    # Color table + pixel data
    bmp += color_table
    bmp += pixel_data

    return bytes(bmp)


def main():
    parser = argparse.ArgumentParser(
        description="Generate a base64 BMP logo for the web portal"
    )
    parser.add_argument(
        "--bg",
        default="16213e",
        help="Background color as hex RGB (default: 16213e, the portal header color)",
    )
    parser.add_argument(
        "--fg",
        default="ffffff",
        help="Foreground (logo) color as hex RGB (default: ffffff, white)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Write base64 to file instead of stdout",
    )
    parser.add_argument(
        "--source",
        default=APP_SCREENS,
        help=f"Path to app_screens.cpp (default: {APP_SCREENS})",
    )
    args = parser.parse_args()

    if len(args.bg) != 6 or len(args.fg) != 6:
        print("Error: colors must be 6-digit hex (e.g., ff8800)", file=sys.stderr)
        sys.exit(1)

    logo_bytes = parse_logo_bytes(args.source)
    width, height = parse_logo_dimensions(args.source)
    print(f"Logo: {width}x{height}, {len(logo_bytes)} bytes", file=sys.stderr)

    bmp_data = build_bmp(logo_bytes, width, height, args.bg, args.fg)
    b64 = base64.b64encode(bmp_data).decode("ascii")

    print(f"BMP: {len(bmp_data)} bytes, base64: {len(b64)} chars", file=sys.stderr)
    print(f"Background: #{args.bg}, Foreground: #{args.fg}", file=sys.stderr)

    if args.output:
        with open(args.output, "w") as f:
            f.write(b64)
        print(f"Saved to {args.output}", file=sys.stderr)
    else:
        print(b64)


if __name__ == "__main__":
    main()
