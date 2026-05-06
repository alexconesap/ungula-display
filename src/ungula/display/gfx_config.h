// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file gfx_config.h
 * @brief Hardware configuration for the display driver
 *
 * Projects provide a GfxConfig struct to gfx_init() to configure
 * the display hardware. If no config is passed, the defaults match
 * the Waveshare ESP32-S3-Touch-LCD-7 (800x480 RGB, GT911 touch).
 */

#ifndef GFX_CONFIG_H
#define GFX_CONFIG_H
#include <cstdint>

namespace ungula::display {

    struct GfxConfig {
            // -- Display resolution --
            int width = 800;
            int height = 480;

            // -- RGB bus data pins (16-bit: 5 blue + 6 green + 5 red) --
            int8_t pin_b0 = 14;
            int8_t pin_b1 = 38;
            int8_t pin_b2 = 18;
            int8_t pin_b3 = 17;
            int8_t pin_b4 = 10;

            int8_t pin_g0 = 39;
            int8_t pin_g1 = 0;
            int8_t pin_g2 = 45;
            int8_t pin_g3 = 48;
            int8_t pin_g4 = 47;
            int8_t pin_g5 = 21;

            int8_t pin_r0 = 1;
            int8_t pin_r1 = 2;
            int8_t pin_r2 = 42;
            int8_t pin_r3 = 41;
            int8_t pin_r4 = 40;

            // -- Control signals --
            int8_t pin_henable = 5;
            int8_t pin_vsync = 3;
            int8_t pin_hsync = 46;
            int8_t pin_pclk = 7;
            uint32_t freq_write = 14000000;  // 14 MHz pixel clock

            // -- Display timing --
            uint8_t hsync_polarity = 0;
            uint8_t hsync_front_porch = 20;
            uint8_t hsync_pulse_width = 10;
            uint8_t hsync_back_porch = 10;

            uint8_t vsync_polarity = 0;
            uint8_t vsync_front_porch = 10;
            uint8_t vsync_pulse_width = 10;
            uint8_t vsync_back_porch = 10;

            uint8_t pclk_active_neg = 0;
            uint8_t de_idle_high = 0;
            uint8_t pclk_idle_high = 0;

            // -- Touch (GT911 via I2C) --
            int8_t touch_sda = 8;
            int8_t touch_scl = 9;
            int8_t touch_int = 4;
            int8_t touch_rst = -1;
            uint8_t touch_i2c_port = 1;  // I2C_NUM_1 (separate from expander)
            uint32_t touch_freq = 400000;
            uint8_t touch_addr = 0x14;

            // -- Display rotation (0-3) --
            uint8_t rotation = 2;

            // Factory: Waveshare ESP32-S3-Touch-LCD-7 (800x480)
            static GfxConfig waveshare7inch() {
                return GfxConfig{};
            }
    };

}  // namespace ungula::display

#endif  // GFX_CONFIG_H
