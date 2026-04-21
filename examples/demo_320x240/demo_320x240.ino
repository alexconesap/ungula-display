// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

// UngulaDisplay Demo — 320x240 (ESP32 + ILI9341 SPI display)
//
// This example shows how to use the library's UI widgets on a smaller
// SPI-based display. Since gfx_core.h targets the 7" RGB display,
// this sketch defines its own LGFX class for the ILI9341 and provides
// the `gfx` global that the widget functions expect.
//
// Hardware: ESP32 DevKit + 2.4" ILI9341 SPI TFT (320x240)
// Board:    esp32:esp32:esp32
// Defines:  -DEMBEDDED_UI
//
// Pin wiring (adjust to your board):
//   TFT_MOSI  = GPIO 23
//   TFT_SCLK  = GPIO 18
//   TFT_CS    = GPIO 15
//   TFT_DC    = GPIO  2
//   TFT_RST   = GPIO  4
//   TFT_BL    = GPIO 32

#define EMBEDDED_UI
#define LGFX_USE_V1
#include <ungula_display.h>

#include <LovyanGFX.hpp>

// Screen resolution
static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;

// ---- LGFX configuration for ILI9341 SPI ----
class LGFX : public lgfx::LGFX_Device {
   public:
    lgfx::Panel_ILI9341 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

    LGFX() {
        // SPI bus
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = -1;
            cfg.pin_dc = 2;
            _bus.config(cfg);
        }
        _panel.setBus(&_bus);

        // Panel
        {
            auto cfg = _panel.config();
            cfg.pin_cs = 15;
            cfg.pin_rst = 4;
            cfg.panel_width = SCREEN_W;
            cfg.panel_height = SCREEN_H;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.readable = false;
            _panel.config(cfg);
        }

        // Backlight
        {
            auto cfg = _light.config();
            cfg.pin_bl = 32;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }

        setPanel(&_panel);
    }
};

// The global `gfx` instance that ui_widgets.cpp references
LGFX gfx;

// Include the UI components we need (after gfx is defined)
// We include individual headers rather than ungula_display.h because
// gfx_core.h targets the 7" display and we've provided our own LGFX above.
#include "ui/ui_theme.h"

// ---- Adapted layout for 320x240 ----
static constexpr int BTN_W = 100;
static constexpr int BTN_H = 40;
static constexpr int BTN_GAP = 20;
static constexpr int BTN_Y = (SCREEN_H / 2) + 10;
static constexpr int START_X = (SCREEN_W / 2) - BTN_W - (BTN_GAP / 2);
static constexpr int STOP_X = (SCREEN_W / 2) + (BTN_GAP / 2);
static constexpr int TITLE_Y = 20;

// Minimal widget functions adapted for this example.
// In a full project you would refactor gfx_core.h to support multiple
// display configurations and use the library widgets directly.

void drawButton(int x, int y, int w, int h, const char* text, uint16_t bgColor) {
    gfx.fillRoundRect(x, y, w, h, 5, bgColor);
    gfx.setTextSize(2);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    int textY = y + (h - 16) / 2;
    gfx.drawCentreString(text, x + w / 2, textY);
}

bool touchInRect(int tx, int ty, int x, int y, int w, int h) {
    return (tx >= x && tx < x + w && ty >= y && ty < y + h);
}

void drawScreen() {
    gfx.fillScreen(UI_COLOR_BG_DARK);

    // White 1px border
    gfx.drawRect(0, 0, SCREEN_W, SCREEN_H, UI_COLOR_TEXT_PRIMARY);

    // Title
    gfx.setTextSize(2);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx.drawCentreString("UNGULA UI LIBRARY TEST", SCREEN_W / 2, TITLE_Y);

    // Buttons
    drawButton(START_X, BTN_Y, BTN_W, BTN_H, "START", UI_COLOR_BTN_SUCCESS);
    drawButton(STOP_X, BTN_Y, BTN_W, BTN_H, "STOP", UI_COLOR_BTN_DANGER);
}

void setup() {
    Serial.begin(115200);

    gfx.init();
    gfx.setRotation(1);  // Landscape
    gfx.setBrightness(200);

    drawScreen();
    Serial.println("UngulaDisplay demo ready (320x240)");
}

void loop() {
    int32_t tx, ty;
    if (!gfx.getTouch(&tx, &ty)) return;

    if (touchInRect(tx, ty, START_X, BTN_Y, BTN_W, BTN_H)) {
        drawButton(START_X, BTN_Y, BTN_W, BTN_H, "START", UI_COLOR_BTN_PRESSED);
        Serial.println("START pressed");
        delay(200);
        drawButton(START_X, BTN_Y, BTN_W, BTN_H, "START", UI_COLOR_BTN_SUCCESS);
    }

    if (touchInRect(tx, ty, STOP_X, BTN_Y, BTN_W, BTN_H)) {
        drawButton(STOP_X, BTN_Y, BTN_W, BTN_H, "STOP", UI_COLOR_BTN_PRESSED);
        Serial.println("STOP pressed");
        delay(200);
        drawButton(STOP_X, BTN_Y, BTN_W, BTN_H, "STOP", UI_COLOR_BTN_DANGER);
    }

    delay(50);
}
