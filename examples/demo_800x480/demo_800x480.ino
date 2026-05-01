// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

// UngulaDisplay Demo — 800x480 (Waveshare ESP32-S3-Touch-LCD-7)
//
// This example initializes the 7" display, draws a white border around
// the screen, a centered title, and START/STOP buttons. Touching a
// button shows a brief pressed state.
//
// Hardware: Waveshare ESP32-S3-Touch-LCD-7
// Board:    esp32:esp32:esp32s3 (with OPI PSRAM, 8MB Flash)
// Defines:  -DEMBEDDED_UI

#include <ungula_display.h>

// Button layout (centered on screen)
static constexpr int BTN_W = 200;
static constexpr int BTN_H = 70;
static constexpr int BTN_GAP = 40;
static constexpr int BTN_Y = (UI_SCREEN_HEIGHT / 2) + 10;
static constexpr int START_X = (UI_SCREEN_WIDTH / 2) - BTN_W - (BTN_GAP / 2);
static constexpr int STOP_X = (UI_SCREEN_WIDTH / 2) + (BTN_GAP / 2);

// Title position
static constexpr int TITLE_Y = 40;

int border_size = 0;

void drawScreen() {
    // Clear screen
    ui_clear(UI_COLOR_BG_DARK);

    // White 1px border around the entire screen
    ui_draw_box(0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, UI_COLOR_TEXT_PRIMARY, border_size);

    // Title centered at top
    ui_draw_text_centered(0, TITLE_Y, UI_SCREEN_WIDTH, "UNGULA UI LIBRARY TEST",
                          UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_LARGE);

    // START button (green)
    ui_draw_button(START_X, BTN_Y, BTN_W, BTN_H, "+", UI_COLOR_BTN_SUCCESS, UI_COLOR_TEXT_PRIMARY,
                   UI_TEXT_SIZE_LARGE);

    // STOP button (red)
    ui_draw_button(STOP_X, BTN_Y, BTN_W, BTN_H, "-", UI_COLOR_BTN_DANGER, UI_COLOR_TEXT_PRIMARY,
                   UI_TEXT_SIZE_LARGE);
}

void setup() {
    Serial.begin(115200);

    // Use default hardware config (Waveshare 7" 800x480)
    GfxConfig hw = GfxConfig::waveshare7inch();

    // Initialize IO expander (backlight, reset)
    ExpanderInit(hw.expander_sda, hw.expander_scl);

    // Initialize display
    gfx_init(hw);

    drawScreen();
    Serial.println("UngulaDisplay demo ready (800x480)");
}

void loop() {
    int32_t tx, ty;
    if (!gfx_get_touch(&tx, &ty))
        return;

    // Check START button
    if (ui_touch_in_rect(tx, ty, START_X, BTN_Y, BTN_W, BTN_H)) {
        ui_draw_button(START_X, BTN_Y, BTN_W, BTN_H, "+", UI_COLOR_BTN_PRESSED,
                       UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_LARGE);
        border_size += 5;
        delay(200);
    }

    // Check STOP button
    if (ui_touch_in_rect(tx, ty, STOP_X, BTN_Y, BTN_W, BTN_H)) {
        ui_draw_button(STOP_X, BTN_Y, BTN_W, BTN_H, "-", UI_COLOR_BTN_PRESSED,
                       UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_LARGE);
        border_size -= 5;
        delay(200);
    }

    drawScreen();
    delay(50);  // Touch debounce
}
