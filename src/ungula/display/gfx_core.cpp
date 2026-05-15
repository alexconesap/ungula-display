// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.
/**
 * @file gfx_core.cpp
 * @brief Core graphics driver implementation
 */

#include "gfx_core.h"

#include <cstdarg>
#include <cstdio>
#include <new>

namespace ungula::display
{

// Global display instance (configured in gfx_init, not in constructor)
LGFX gfx;

void gfx_init(const GfxConfig &config)
{
        gfx.configure(config);
        gfx.init();
        gfx.setRotation(config.rotation);
}

bool gfx_get_touch(int32_t *pos_x, int32_t *pos_y)
{
        return gfx.getTouch(pos_x, pos_y) != 0;
}

static gfx_font_hook_t s_font_hook = nullptr;
static int8_t s_font_y_offset = 0;

void gfx_set_font_hook(gfx_font_hook_t hook)
{
        s_font_hook = hook;
}

void gfx_set_font(int size)
{
        if (s_font_hook != nullptr) {
                s_font_hook(size);
        } else {
                gfx.setTextSize(size);
        }
}

void gfx_set_font_y_offset(int8_t offset)
{
        s_font_y_offset = offset;
}

int8_t gfx_get_font_y_offset()
{
        return s_font_y_offset;
}

void gfx_drawCentreString(const char *text, int pos_x, int pos_y)
{
        gfx.drawCentreString(text, pos_x, pos_y + s_font_y_offset);
}

void gfx_drawString(const char *text, int pos_x, int pos_y)
{
        gfx.drawString(text, pos_x, pos_y + s_font_y_offset);
}

void gfx_setCursor(int pos_x, int pos_y)
{
        gfx.setCursor(pos_x, pos_y + s_font_y_offset);
}

void gfx_print(const char *text)
{
        gfx.print(text);
}

void gfx_printf(const char *fmt, ...)
{
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        gfx.print(buf);
}

void gfx_fillScreen(uint16_t color)
{
        gfx.fillScreen(color);
}

void gfx_setTextColor(uint16_t color)
{
        gfx.setTextColor(color);
}

void gfx_setFont(const void *font_ptr)
{
        gfx.setFont(static_cast<const lgfx::IFont *>(font_ptr));
}

void gfx_setTextSize(float size)
{
        gfx.setTextSize(size);
}

void gfx_fillRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, uint16_t color)
{
        gfx.fillRect(pos_x, pos_y, width, height, color);
}

void gfx_fillRoundRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, int32_t radius,
                       uint16_t color)
{
        gfx.fillRoundRect(pos_x, pos_y, width, height, radius, color);
}

void gfx_drawRoundRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, int32_t radius,
                       uint16_t color)
{
        gfx.drawRoundRect(pos_x, pos_y, width, height, radius, color);
}

void gfx_fillCircle(int32_t pos_x, int32_t pos_y, int32_t radius, uint16_t color)
{
        gfx.fillCircle(pos_x, pos_y, radius, color);
}

void gfx_fillTriangle(int32_t pos_x0, int32_t pos_y0, int32_t pos_x1, int32_t pos_y1,
                      int32_t pos_x2, int32_t pos_y2, uint16_t color)
{
        gfx.fillTriangle(pos_x0, pos_y0, pos_x1, pos_y1, pos_x2, pos_y2, color);
}

// --- Font abstraction ---

static constexpr uint8_t MAX_CUSTOM_FONTS = 16;
// U8g2font has no default constructor, so use aligned raw storage + placement new
static uint8_t s_custom_font_storage[MAX_CUSTOM_FONTS][sizeof(lgfx::U8g2font)]
    __attribute__((aligned(alignof(lgfx::U8g2font))));
static uint8_t s_custom_font_count = 0;

const void *gfx_builtin_font(GfxFont font)
{
        switch (font) {
        case GfxFont::SANS_9PT:
                return &fonts::FreeSans9pt7b;
        case GfxFont::SANS_12PT:
                return &fonts::FreeSans12pt7b;
        case GfxFont::SANS_18PT:
                return &fonts::FreeSans18pt7b;
        case GfxFont::SANS_24PT:
                return &fonts::FreeSans24pt7b;
        case GfxFont::MONO_9PT:
                return &fonts::FreeMono9pt7b;
        case GfxFont::MONO_12PT:
                return &fonts::FreeMono12pt7b;
        case GfxFont::MONO_18PT:
                return &fonts::FreeMono18pt7b;
        case GfxFont::MONO_24PT:
                return &fonts::FreeMono24pt7b;
        case GfxFont::TINY:
                return &fonts::Font0;
        }
        __builtin_unreachable();
}

const void *gfx_register_u8g2_font(const uint8_t *font_data)
{
        if (s_custom_font_count >= MAX_CUSTOM_FONTS) {
                return nullptr;
        }
        auto *slot = new (s_custom_font_storage[s_custom_font_count]) lgfx::U8g2font(font_data);
        s_custom_font_count++;
        return slot;
}

} // namespace ungula::display
