// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_widgets.cpp
 * @brief Reusable UI widget drawing functions implementation
 */

#include "ui_widgets.h"

#include "display/gfx_bitmap.h"

// ============================================================================
// PANEL/CONTAINER WIDGETS
// ============================================================================

void ui_draw_panel(int pos_x, int pos_y, int width, int height, uint16_t fill_color,
                   uint16_t border_color, int radius) {
    gfx.fillRoundRect(pos_x, pos_y, width, height, radius, fill_color);
    if (border_color != 0) {
        gfx.drawRoundRect(pos_x, pos_y, width, height, radius, border_color);
    }
}

void ui_draw_content_frame() {
    gfx.fillRoundRect(UI_CONTENT_X + 1, UI_CONTENT_Y + 1, UI_CONTENT_WIDTH - 2,
                      UI_CONTENT_HEIGHT - 2, UI_RADIUS_NORMAL, UI_COLOR_BG_PANEL);
    gfx.drawRoundRect(UI_CONTENT_X, UI_CONTENT_Y, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT,
                      UI_RADIUS_NORMAL, UI_COLOR_BORDER);
}

// White 1px border around the entire screen
void ui_draw_box(int pos_x, int pos_y, int width, int height, uint16_t border_color,
                 int border_width) {
    for (int i = 0; i < border_width; i++) {
        gfx.drawRect(pos_x + i, pos_y + i, width - (2 * i), height - (2 * i), border_color);
    }
}

// Clear screen
void ui_clear(uint16_t background_color) {
    gfx.fillScreen(background_color);
}

// ============================================================================
// BUTTON WIDGETS
// ============================================================================

void ui_draw_button(int pos_x, int pos_y, int width, int height, const char* text,
                    uint16_t bg_color, uint16_t text_color, int text_size, int radius) {
    gfx.fillRoundRect(pos_x, pos_y, width, height, radius, bg_color);

    // Calculate text position (centered)
    gfx_set_font(text_size);
    gfx.setTextColor(text_color);

    int text_y = pos_y + (height - (text_size * 8)) / 2;
    gfx_drawCentreString(text, pos_x + width / 2, text_y);
}

void ui_draw_icon_button(int pos_x, int pos_y, int width, int height, const uint8_t* icon,
                         int icon_w, int icon_h, uint16_t bg_color, uint16_t icon_color,
                         int radius) {
    gfx.fillRoundRect(pos_x, pos_y, width, height, radius, bg_color);

    // Center the icon
    int icon_x = pos_x + (width - icon_w) / 2;
    int icon_y = pos_y + (height - icon_h) / 2;
    gfx_draw_bitmap(icon_x, icon_y, icon, icon_w, icon_h, icon_color);
}

// ============================================================================
// STATUS/INDICATOR WIDGETS
// ============================================================================

void ui_draw_led(int pos_x, int pos_y, bool is_on, int radius) {
    uint16_t color = is_on ? UI_COLOR_LED_ON : UI_COLOR_LED_OFF;
    gfx.fillCircle(pos_x, pos_y, radius, color);
}

void ui_draw_status_bar(int pos_x, int pos_y, int width, int height, const char* text,
                        uint16_t bg_color) {
    gfx.fillRoundRect(pos_x, pos_y, width, height, UI_RADIUS_NORMAL, bg_color);

    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx_drawCentreString(text, pos_x + width / 2, pos_y + (height - 16) / 2);
}

// ============================================================================
// VALUE DISPLAY WIDGETS
// ============================================================================

void ui_draw_value_box(int pos_x, int pos_y, int width, int height, const char* label,
                       const char* value, const char* unit, uint16_t border_color) {
    // Background
    gfx.fillRoundRect(pos_x, pos_y, width, height, UI_RADIUS_NORMAL, UI_COLOR_BG_DARK);
    gfx.drawRoundRect(pos_x, pos_y, width, height, UI_RADIUS_NORMAL, border_color);

    // Label (small, grey, left aligned)
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_SECONDARY);
    gfx_setCursor(pos_x + UI_PADDING_LARGE, pos_y + height / 2 - 8);
    gfx.print(label);

    // Value (larger, white)
    gfx_set_font(UI_TEXT_SIZE_LARGE);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx_setCursor(pos_x + width / 2, pos_y + height / 2 - 12);
    gfx.print(value);

    // Unit (small, grey)
    if (unit) {
        gfx_set_font(UI_TEXT_SIZE_NORMAL);
        gfx.setTextColor(UI_COLOR_TEXT_SECONDARY);
        gfx_setCursor(pos_x + width - 80, pos_y + height / 2 - 8);
        gfx.print(unit);
    }
}

void ui_draw_temp_display(int pos_x, int pos_y, const char* label, int temp) {
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_SECONDARY);
    gfx_setCursor(pos_x, pos_y);
    gfx.print(label);

    gfx_set_font(UI_TEXT_SIZE_LARGE);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);

    // Clear previous value area
    gfx.fillRect(pos_x + 10, pos_y + 35, 100, 40, UI_COLOR_BG_DARK);

    // Format value with padding
    char buf[16];
    if (temp <= 0) {
        snprintf(buf, sizeof(buf), " 0  F");
    } else if (temp < 10) {
        snprintf(buf, sizeof(buf), " %d  F", temp);
    } else if (temp < 100) {
        snprintf(buf, sizeof(buf), " %d F", temp);
    } else {
        snprintf(buf, sizeof(buf), "%d F", temp);
    }

    gfx_setCursor(pos_x + 10, pos_y + 45);
    gfx.print(buf);
}

// ============================================================================
// TEXT HELPERS
// ============================================================================

void ui_draw_text_centered(int pos_x, int pos_y, int width, const char* text, uint16_t color,
                           int size) {
    gfx_set_font(size);
    gfx.setTextColor(color);
    gfx_drawCentreString(text, pos_x + width / 2, pos_y);
}

void ui_draw_text(int pos_x, int pos_y, const char* text, uint16_t color, int size) {
    gfx_set_font(size);
    gfx.setTextColor(color);
    gfx_setCursor(pos_x, pos_y);
    gfx.print(text);
}

// ============================================================================
// TOUCH HELPERS
// ============================================================================

bool ui_touch_in_rect(int touch_x, int touch_y, int pos_x, int pos_y, int width, int height) {
    return (touch_x >= pos_x && touch_x < pos_x + width && touch_y >= pos_y &&
            touch_y < pos_y + height);
}

// ============================================================================
// MODAL HELPERS
// ============================================================================

void ui_draw_modal_backdrop() {
    // Draw a dithered dark overlay to create a strong dimming effect
    // Using black horizontal lines every 2 pixels for 50% coverage
    // This creates a darker effect than using UI_COLOR_BG_DARK

    // Draw black horizontal lines on even rows (50% coverage)
    for (int y = 0; y < UI_SCREEN_HEIGHT; y += 2) {
        gfx.drawFastHLine(0, y, UI_SCREEN_WIDTH, 0x0000);  // Black
    }
}

// ============================================================================
// CLOSE BUTTON
// ============================================================================

void ui_draw_close_button(int pos_x, int pos_y, int size, bool pressed) {
    // Background
    uint16_t bg_color = pressed ? UI_COLOR_BTN_PRESSED : UI_COLOR_DANGER;
    gfx.fillRoundRect(pos_x, pos_y, size, size, UI_RADIUS_SMALL, bg_color);

    // Draw X using two thick diagonal lines
    int margin = 10;
    int x1 = pos_x + margin;
    int y1 = pos_y + margin;
    int x2 = pos_x + size - margin;
    int y2 = pos_y + size - margin;

    for (int i = -1; i <= 1; i++) {
        gfx.drawLine(x1 + i, y1, x2 + i, y2, UI_COLOR_TEXT_PRIMARY);
        gfx.drawLine(x2 + i, y1, x1 + i, y2, UI_COLOR_TEXT_PRIMARY);
    }
}

bool ui_touch_on_close_button(int touch_x, int touch_y, int pos_x, int pos_y, int size) {
    return ui_touch_in_rect(touch_x, touch_y, pos_x, pos_y, size, size);
}
