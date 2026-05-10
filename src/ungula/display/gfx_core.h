// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file gfx_core.h
 * @brief Core graphics driver for ESP32-S3 RGB display with GT911 touch
 *
 * Configurable display driver built on LovyanGFX. Hardware parameters
 * (resolution, pins, timing) are set via GfxConfig passed to gfx_init().
 * Defaults match the Waveshare ESP32-S3-Touch-LCD-7 (800x480).
 */

#pragma once
#define LGFX_USE_V1
#include <driver/i2c.h>

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

#include "gfx_config.h"

namespace ungula::display
{

    /**
     * @brief LGFX display driver for ESP32-S3 RGB displays with GT911 touch
     *
     * Hardware configuration is deferred to configure() which must be called
     * before init(). Use gfx_init() which handles both steps.
     */
    class LGFX : public lgfx::LGFX_Device {
    public:
        lgfx::Bus_RGB _bus_instance;
        lgfx::Panel_RGB _panel_instance;
        lgfx::Light_PWM _light_instance;
        lgfx::Touch_GT911 _touch_instance;

        LGFX() = default;

        /**
             * @brief Apply hardware configuration from a GfxConfig struct
             * Must be called before init().
             */
        void configure(const GfxConfig &hwc)
        {
            // Panel
            {
                auto cfg = _panel_instance.config();
                cfg.memory_width = hwc.width;
                cfg.memory_height = hwc.height;
                cfg.panel_width = hwc.width;
                cfg.panel_height = hwc.height;
                cfg.offset_x = 0;
                cfg.offset_y = 0;
                _panel_instance.config(cfg);
            }

            // RGB bus (16-bit: 5B + 6G + 5R)
            {
                auto cfg = _bus_instance.config();
                cfg.panel = &_panel_instance;

                cfg.pin_d0 = hwc.pin_b0;
                cfg.pin_d1 = hwc.pin_b1;
                cfg.pin_d2 = hwc.pin_b2;
                cfg.pin_d3 = hwc.pin_b3;
                cfg.pin_d4 = hwc.pin_b4;

                cfg.pin_d5 = hwc.pin_g0;
                cfg.pin_d6 = hwc.pin_g1;
                cfg.pin_d7 = hwc.pin_g2;
                cfg.pin_d8 = hwc.pin_g3;
                cfg.pin_d9 = hwc.pin_g4;
                cfg.pin_d10 = hwc.pin_g5;

                cfg.pin_d11 = hwc.pin_r0;
                cfg.pin_d12 = hwc.pin_r1;
                cfg.pin_d13 = hwc.pin_r2;
                cfg.pin_d14 = hwc.pin_r3;
                cfg.pin_d15 = hwc.pin_r4;

                cfg.pin_henable = hwc.pin_henable;
                cfg.pin_vsync = hwc.pin_vsync;
                cfg.pin_hsync = hwc.pin_hsync;
                cfg.pin_pclk = hwc.pin_pclk;
                cfg.freq_write = hwc.freq_write;

                cfg.hsync_polarity = hwc.hsync_polarity;
                cfg.hsync_front_porch = hwc.hsync_front_porch;
                cfg.hsync_pulse_width = hwc.hsync_pulse_width;
                cfg.hsync_back_porch = hwc.hsync_back_porch;

                cfg.vsync_polarity = hwc.vsync_polarity;
                cfg.vsync_front_porch = hwc.vsync_front_porch;
                cfg.vsync_pulse_width = hwc.vsync_pulse_width;
                cfg.vsync_back_porch = hwc.vsync_back_porch;

                cfg.pclk_active_neg = hwc.pclk_active_neg;
                cfg.de_idle_high = hwc.de_idle_high;
                cfg.pclk_idle_high = hwc.pclk_idle_high;

                _bus_instance.config(cfg);
            }
            _panel_instance.setBus(&_bus_instance);

            // Touch (GT911)
            {
                auto cfg = _touch_instance.config();
                cfg.x_min = 0;
                cfg.x_max = hwc.width - 1;
                cfg.y_min = 0;
                cfg.y_max = hwc.height - 1;
                cfg.pin_int = hwc.touch_int;
                cfg.pin_rst = hwc.touch_rst;
                cfg.bus_shared = false;
                cfg.offset_rotation = 0;
                cfg.i2c_port = static_cast<i2c_port_t>(hwc.touch_i2c_port);
                cfg.pin_sda = hwc.touch_sda;
                cfg.pin_scl = hwc.touch_scl;
                cfg.freq = hwc.touch_freq;
                cfg.i2c_addr = hwc.touch_addr;
                _touch_instance.config(cfg);
                _panel_instance.setTouch(&_touch_instance);
            }
            setPanel(&_panel_instance);
        }
    };

    /// Global display instance
    extern LGFX gfx;

    /**
     * @brief Initialize the display with the given hardware configuration
     * @param config Hardware config (default: Waveshare 7" 800x480)
     */
    void gfx_init(const GfxConfig &config = GfxConfig::waveshare7inch());

    /**
     * @brief Get touch coordinates if screen is being touched
     * @param pos_x Pointer to store X coordinate
     * @param pos_y Pointer to store Y coordinate
     * @return true if touch detected, false otherwise
     */
    bool gfx_get_touch(int32_t *pos_x, int32_t *pos_y);

    /// Font-size hook type: maps logical size (1-4) to an actual font/size call
    typedef void (*gfx_font_hook_t)(int size);

    /**
     * @brief Register a project-specific font mapper
     *
     * When set, all library widgets will call this hook instead of gfx.setTextSize().
     * Pass nullptr to revert to the default setTextSize() behavior.
     */
    void gfx_set_font_hook(gfx_font_hook_t hook);

    /**
     * @brief Set the font for a given logical size (1-4)
     *
     * If a font hook is registered, calls it. Otherwise falls back to gfx.setTextSize().
     * All library widgets use this function instead of calling setTextSize() directly.
     */
    void gfx_set_font(int size);

    /**
     * @brief Set a vertical pixel offset applied to all text rendering
     *
     * Used to compensate for font baseline differences between typefaces.
     * Positive values shift text down, negative values shift text up.
     */
    void gfx_set_font_y_offset(int8_t offset);

    /// Get the current font Y offset
    int8_t gfx_get_font_y_offset();

    // ============================================================================
    // TEXT WRAPPERS — apply font Y offset automatically
    // Use these instead of gfx.drawCentreString / gfx.setCursor / gfx.drawString
    // ============================================================================

    /// Draw text centered at (pos_x, pos_y) with automatic vertical offset
    void gfx_drawCentreString(const char *text, int pos_x, int pos_y);

    /// Draw text at (pos_x, pos_y) with automatic vertical offset
    void gfx_drawString(const char *text, int pos_x, int pos_y);

    /// Set cursor at (pos_x, pos_y) with automatic vertical offset
    void gfx_setCursor(int pos_x, int pos_y);

    /// Print text at current cursor position (no offset applied — cursor already adjusted)
    void gfx_print(const char *text);

    /// Print formatted text at current cursor position
    void gfx_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    // ============================================================================
    // GRAPHICS WRAPPERS — thin passthroughs to isolate LovyanGFX from app code
    // ============================================================================

    void gfx_fillScreen(uint16_t color);
    void gfx_setTextColor(uint16_t color);
    void gfx_setFont(const void *font_ptr);
    void gfx_setTextSize(float size);

    // ============================================================================
    // FONT ABSTRACTION — hides LovyanGFX font types from application code
    // ============================================================================

    /// Built-in font identifiers (maps to LovyanGFX FreeSans/FreeMono fonts)
    enum class GfxFont : uint8_t {
        SANS_9PT, // FreeSans9pt7b
        SANS_12PT, // FreeSans12pt7b
        SANS_18PT, // FreeSans18pt7b
        SANS_24PT, // FreeSans24pt7b
        MONO_9PT, // FreeMono9pt7b
        MONO_12PT, // FreeMono12pt7b
        MONO_18PT, // FreeMono18pt7b
        MONO_24PT, // FreeMono24pt7b
        TINY, // Font0 (built-in small pixel font)
    };

    /// Get a built-in font pointer by identifier
    const void *gfx_builtin_font(GfxFont font);

    /// Register a U8g2 font from a binary data array (PROGMEM).
    /// Returns an opaque font pointer usable with gfx_setFont().
    /// The pointer is valid for the lifetime of the program (static storage).
    /// Max 16 custom fonts.
    const void *gfx_register_u8g2_font(const uint8_t *font_data);

    void gfx_fillRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, uint16_t color);
    void gfx_fillRoundRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, int32_t radius, uint16_t color);
    void gfx_drawRoundRect(int32_t pos_x, int32_t pos_y, int32_t width, int32_t height, int32_t radius, uint16_t color);
    void gfx_fillCircle(int32_t pos_x, int32_t pos_y, int32_t radius, uint16_t color);
    void gfx_fillTriangle(int32_t pos_x0, int32_t pos_y0, int32_t pos_x1, int32_t pos_y1, int32_t pos_x2,
                          int32_t pos_y2, uint16_t color);

} // namespace ungula::display
