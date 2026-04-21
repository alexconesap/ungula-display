// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file gfx_bitmap.cpp
 * @brief Bitmap drawing functions implementation
 */

#include "gfx_bitmap.h"

#include <pgmspace.h>

#include "gfx_core.h"

void gfx_draw_bitmap(int pos_x, int pos_y, const uint8_t* bitmap, int width, int height,
                     uint16_t color) {
    gfx.drawBitmap(pos_x, pos_y, bitmap, width, height, color);
}

void gfx_draw_bitmap_scaled(int32_t pos_x, int32_t pos_y, const uint8_t* bitmap, int srcW, int srcH,
                            int dstW, int dstH, uint16_t color) {
    int srcBytesPerRow = (srcW + 7) / 8;
    gfx.startWrite();
    for (int dy = 0; dy < dstH; dy++) {
        int sy = dy * srcH / dstH;
        int rowOffset = sy * srcBytesPerRow;
        for (int dx = 0; dx < dstW; dx++) {
            int sx = dx * srcW / dstW;
            if (pgm_read_byte(&bitmap[rowOffset + (sx / 8)]) & (0x80 >> (sx % 8))) {
                gfx.writePixel(pos_x + dx, pos_y + dy, color);
            }
        }
    }
    gfx.endWrite();
}

void gfx_draw_indexed_bitmap(int pos_x, int pos_y, const uint8_t* data, int width, int height,
                             const uint16_t palette[4]) {
    int bytesPerRow = (width + 3) / 4;
    for (int row = 0; row < height; row++) {
        for (int bx = 0; bx < bytesPerRow; bx++) {
            uint8_t packed = pgm_read_byte(&data[(row * bytesPerRow) + bx]);
            if (packed == 0) {
                continue;
            }
            for (int pva = 0; pva < 4; pva++) {
                uint8_t idx = (packed >> (6 - (pva * 2))) & 0x03;
                if (idx != 0) {
                    gfx.drawPixel(pos_x + (bx * 4) + pva, pos_y + row, palette[idx]);
                }
            }
        }
    }
}
