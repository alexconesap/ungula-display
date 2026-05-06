// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_keypad.cpp
 * @brief Reusable numeric keypad component implementation
 */

#include "ui_keypad.h"

#include <string.h>

#include "ui_widgets.h"

using namespace ungula::display;

// ============================================================================
// INTERNAL STATE
// ============================================================================

static bool s_visible = false;
static int s_x = 0;
static int s_y = 0;
static char s_title[64];
static int s_value = 0;
static int s_min_value = 0;
static int s_max_value = 9999;
static keypad_callback_t s_callback = nullptr;
static void* s_user_data = nullptr;
static keypad_cancel_callback_t s_cancel_callback = nullptr;

// Password mode state
static bool s_password_mode = false;
static int s_max_digits = 0;
static char s_digit_buf[16];
static int s_digit_count = 0;
static bool s_anti_guess = false;
static constexpr int ANTI_GUESS_MAX_DIGITS = 10;
static constexpr int ANTI_GUESS_PIN_LEN = 4;

// Track which button is currently pressed (-1 = none)
static int s_pressed_btn = -1;

// Button definitions
// Layout: 3 columns x 4 rows for digits, plus action buttons
// Row 0: 1, 2, 3
// Row 1: 4, 5, 6
// Row 2: 7, 8, 9
// Row 3: CLR, 0, ENT
// Plus Cancel (X) button at top

struct KeypadButton {
        int x, y, w, h;
        const char* label;
        int action;  // 0-9 = digit, 10 = clear, 11 = enter, 12 = cancel
};

#define KEYPAD_BTN_COUNT 13

static KeypadButton s_buttons[KEYPAD_BTN_COUNT];

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void init_buttons() {
    int padding = 6;
    int start_x = s_x + padding + 2;
    int start_y = s_y + 110;  // Leave room for title (50) + display (50) + gap

    // Digit buttons 1-9
    const char* digit_labels[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        s_buttons[i].x = start_x + col * (KEYPAD_BTN_WIDTH + KEYPAD_BTN_GAP);
        s_buttons[i].y = start_y + row * (KEYPAD_BTN_HEIGHT + KEYPAD_BTN_GAP);
        s_buttons[i].w = KEYPAD_BTN_WIDTH;
        s_buttons[i].h = KEYPAD_BTN_HEIGHT;
        s_buttons[i].label = digit_labels[i];
        s_buttons[i].action = i + 1;  // 1-9
    }

    // Bottom row: CLR, 0, ENT
    int row3_y = start_y + 3 * (KEYPAD_BTN_HEIGHT + KEYPAD_BTN_GAP);

    // CLR button
    s_buttons[9].x = start_x;
    s_buttons[9].y = row3_y;
    s_buttons[9].w = KEYPAD_BTN_WIDTH;
    s_buttons[9].h = KEYPAD_BTN_HEIGHT;
    s_buttons[9].label = "CLR";
    s_buttons[9].action = 10;

    // 0 button
    s_buttons[10].x = start_x + (KEYPAD_BTN_WIDTH + KEYPAD_BTN_GAP);
    s_buttons[10].y = row3_y;
    s_buttons[10].w = KEYPAD_BTN_WIDTH;
    s_buttons[10].h = KEYPAD_BTN_HEIGHT;
    s_buttons[10].label = "0";
    s_buttons[10].action = 0;

    // OK button
    s_buttons[11].x = start_x + 2 * (KEYPAD_BTN_WIDTH + KEYPAD_BTN_GAP);
    s_buttons[11].y = row3_y;
    s_buttons[11].w = KEYPAD_BTN_WIDTH;
    s_buttons[11].h = KEYPAD_BTN_HEIGHT;
    s_buttons[11].label = "OK";
    s_buttons[11].action = 11;

    // Cancel (X) button at top right - aligned with title row
    s_buttons[12].x = s_x + KEYPAD_WIDTH - 52;
    s_buttons[12].y = s_y + 6;
    s_buttons[12].w = 46;
    s_buttons[12].h = 42;
    s_buttons[12].label = "X";
    s_buttons[12].action = 12;
}

static void draw_button(int idx, bool pressed) {
    KeypadButton& btn = s_buttons[idx];
    uint16_t bg_color;

    // Choose color based on button type and state
    if (pressed) {
        bg_color = UI_COLOR_BTN_PRESSED;
    } else if (btn.action == 11) {  // Enter
        bg_color = UI_COLOR_SUCCESS;
    } else if (btn.action == 12) {  // Cancel
        bg_color = UI_COLOR_DANGER;
    } else if (btn.action == 10) {  // Clear
        bg_color = UI_COLOR_WARNING;
    } else {
        bg_color = UI_COLOR_BTN_PRIMARY;
    }

    ui_draw_button(btn.x, btn.y, btn.w, btn.h, btn.label, bg_color, UI_COLOR_TEXT_PRIMARY,
                   UI_TEXT_SIZE_NORMAL, UI_RADIUS_NORMAL);
}

static void draw_display() {
    // Value display area - spans full width with padding
    int padding = 6;
    int disp_x = s_x + padding;
    int disp_y = s_y + 55;                    // Below title row (50px)
    int disp_w = KEYPAD_WIDTH - padding * 2;  // Full width with padding
    int disp_h = 48;

    // Background
    gfx.fillRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_BG_DARK);
    gfx.drawRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_BORDER);

    // Value text
    char buf[16];
    if (s_password_mode) {
        // Show asterisks for each entered digit
        for (int i = 0; i < s_digit_count && i < (int)sizeof(buf) - 1; i++) {
            buf[i] = '*';
        }
        buf[s_digit_count] = '\0';
    } else {
        snprintf(buf, sizeof(buf), "%d", s_value);
    }

    gfx_set_font(UI_TEXT_SIZE_LARGE);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);

    // Right-align the value using actual rendered width
    int text_w = gfx.textWidth(buf);
    gfx_setCursor(disp_x + disp_w - text_w - 15, disp_y + 10);
    gfx.print(buf);
}

static int find_button_at(int touch_x, int touch_y) {
    for (int i = 0; i < KEYPAD_BTN_COUNT; i++) {
        if (ui_touch_in_rect(touch_x, touch_y, s_buttons[i].x, s_buttons[i].y, s_buttons[i].w,
                             s_buttons[i].h)) {
            return i;
        }
    }
    return -1;
}

static void process_action(int action) {
    if (action >= 0 && action <= 9) {
        if (s_password_mode) {
            // Password mode: collect individual digits up to max_digits
            if (s_digit_count < s_max_digits) {
                s_digit_buf[s_digit_count++] = '0' + action;
                s_digit_buf[s_digit_count] = '\0';
            }
        } else {
            // Normal mode: accumulate integer value
            int new_value = s_value * 10 + action;
            if (new_value <= s_max_value) {
                s_value = new_value;
            }
        }
        draw_display();
    } else if (action == 10) {
        // Clear - remove last digit
        if (s_password_mode) {
            if (s_digit_count > 0) {
                s_digit_buf[--s_digit_count] = '\0';
            }
        } else {
            s_value = s_value / 10;
        }
        draw_display();
    } else if (action == 11) {
        // Enter
        if (s_password_mode) {
            int required = s_anti_guess ? ANTI_GUESS_PIN_LEN : s_max_digits;
            if (s_digit_count >= required) {
                // Convert digits to int — in anti-guess mode use only the last 4
                int val = 0;
                int start = s_anti_guess ? (s_digit_count - ANTI_GUESS_PIN_LEN) : 0;
                int end = s_anti_guess ? s_digit_count : s_digit_count;
                for (int i = start; i < end; i++) {
                    val = val * 10 + (s_digit_buf[i] - '0');
                }
                keypad_hide();
                if (s_callback) {
                    s_callback(val, s_user_data);
                }
            } else {
                // Not enough digits - flash red border
                int padding = 6;
                int disp_x = s_x + padding;
                int disp_y = s_y + 55;
                int disp_w = KEYPAD_WIDTH - padding * 2;
                int disp_h = 48;
                gfx.drawRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_DANGER);
            }
        } else {
            // Normal mode: validate range
            if (s_value >= s_min_value && s_value <= s_max_value) {
                keypad_hide();
                if (s_callback) {
                    s_callback(s_value, s_user_data);
                }
            } else {
                int padding = 6;
                int disp_x = s_x + padding;
                int disp_y = s_y + 55;
                int disp_w = KEYPAD_WIDTH - padding * 2;
                int disp_h = 48;
                gfx.drawRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_DANGER);
            }
        }
    } else if (action == 12) {
        // Cancel - hide before callback so the callback can redraw the screen
        keypad_hide();
        if (s_cancel_callback) {
            s_cancel_callback(s_user_data);
        }
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void keypad_show(int pos_x, int pos_y, const char* title, int initial_value, int min_value,
                 int max_value, keypad_callback_t callback, void* user_data,
                 keypad_cancel_callback_t cancel_callback, bool password_mode, int max_digits,
                 bool anti_guess) {
    s_x = pos_x;
    s_y = pos_y;
    s_value = initial_value;
    s_min_value = min_value;
    s_max_value = max_value;
    s_callback = callback;
    s_user_data = user_data;
    s_cancel_callback = cancel_callback;
    s_password_mode = password_mode;
    s_anti_guess = anti_guess;

    if (anti_guess) {
        // Anti-guess mode: allow up to 10 digits, extract last 4 on confirm
        s_max_digits = ANTI_GUESS_MAX_DIGITS;
    } else {
        s_max_digits = (max_digits > 0 && max_digits < (int)sizeof(s_digit_buf)) ? max_digits : 0;
    }
    s_digit_count = 0;
    s_digit_buf[0] = '\0';
    s_pressed_btn = -1;

    // Store title
    if (title) {
        strncpy(s_title, title, sizeof(s_title) - 1);
        s_title[sizeof(s_title) - 1] = '\0';
    } else {
        s_title[0] = '\0';
    }

    s_visible = true;

    init_buttons();
    keypad_draw();
}

void keypad_hide() {
    s_visible = false;
    // Note: Caller should redraw the underlying screen
}

bool keypad_is_visible() {
    return s_visible;
}

bool keypad_handle_touch(int touch_x, int touch_y, bool pressed) {
    if (!s_visible) {
        return false;
    }

    // Handle touch release first - process even if coordinates are outside bounds
    // (touch coordinates may be invalid on release)
    if (!pressed) {
        if (s_pressed_btn >= 0) {
            // Process the action
            int action = s_buttons[s_pressed_btn].action;
            draw_button(s_pressed_btn, false);  // Un-highlight
            s_pressed_btn = -1;
            process_action(action);
            return true;
        }
        return false;
    }

    // Check if touch is within keypad bounds for press events
    if (!ui_touch_in_rect(touch_x, touch_y, s_x, s_y, KEYPAD_WIDTH, KEYPAD_HEIGHT)) {
        return false;  // Touch outside keypad
    }

    // Find which button was pressed
    int btn = find_button_at(touch_x, touch_y);
    if (btn >= 0 && btn != s_pressed_btn) {
        // New button pressed - highlight it
        if (s_pressed_btn >= 0) {
            draw_button(s_pressed_btn, false);  // Un-highlight old
        }
        s_pressed_btn = btn;
        draw_button(s_pressed_btn, true);  // Highlight new
    }

    return true;  // Touch was handled
}

void keypad_draw() {
    if (!s_visible) {
        return;
    }

    // Draw darkened backdrop over the screen
    ui_draw_modal_backdrop();

    // Draw modal shadow/outline effect (darker outer border)
    gfx.drawRoundRect(s_x - 3, s_y - 3, KEYPAD_WIDTH + 6, KEYPAD_HEIGHT + 6, UI_RADIUS_NORMAL + 3,
                      0x0000);
    gfx.drawRoundRect(s_x - 2, s_y - 2, KEYPAD_WIDTH + 4, KEYPAD_HEIGHT + 4, UI_RADIUS_NORMAL + 2,
                      0x0000);
    gfx.drawRoundRect(s_x - 1, s_y - 1, KEYPAD_WIDTH + 2, KEYPAD_HEIGHT + 2, UI_RADIUS_NORMAL + 1,
                      0x0000);

    // Draw keypad background
    gfx.fillRoundRect(s_x, s_y, KEYPAD_WIDTH, KEYPAD_HEIGHT, UI_RADIUS_NORMAL, UI_COLOR_BG_PANEL);

    // Visible border (thick bright accent for modal effect)
    gfx.drawRoundRect(s_x, s_y, KEYPAD_WIDTH, KEYPAD_HEIGHT, UI_RADIUS_NORMAL, UI_COLOR_ACCENT);
    gfx.drawRoundRect(s_x + 1, s_y + 1, KEYPAD_WIDTH - 2, KEYPAD_HEIGHT - 2, UI_RADIUS_NORMAL - 1,
                      UI_COLOR_ACCENT);
    gfx.drawRoundRect(s_x + 2, s_y + 2, KEYPAD_WIDTH - 4, KEYPAD_HEIGHT - 4, UI_RADIUS_NORMAL - 2,
                      UI_COLOR_BORDER);

    // Title - use custom title if provided, otherwise default
    // Title row is 50px tall, text centered vertically
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_SECONDARY);
    gfx_setCursor(s_x + 10, s_y + 18);
    if (s_title[0] != '\0') {
        gfx.print(s_title);
    } else {
        gfx.print("Enter Value");
    }

    // Draw display area
    draw_display();

    // Draw all buttons
    for (int i = 0; i < KEYPAD_BTN_COUNT; i++) {
        draw_button(i, i == s_pressed_btn);
    }
}

int keypad_get_value() {
    return s_value;
}

const char* keypad_get_raw_digits() {
    return s_digit_buf;
}
