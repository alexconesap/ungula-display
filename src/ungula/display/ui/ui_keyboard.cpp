// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_keyboard.cpp
 * @brief Reusable text keyboard component implementation
 */

#include "ui_keyboard.h"

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
static char s_text[KEYBOARD_MAX_TEXT + 1];
static int s_max_length = KEYBOARD_MAX_TEXT;
static keyboard_callback_t s_callback = nullptr;
static void* s_user_data = nullptr;
static bool s_password_mode = false;
static bool s_password_visible = false;  // Toggle for show/hide password
static bool s_shift_active = false;
static bool s_symbols_mode = false;

// Track which key is currently pressed (-1 = none)
static int s_pressed_key = -1;

// ============================================================================
// KEYBOARD LAYOUTS
// ============================================================================

// QWERTY layout - lowercase
static const char* s_layout_lower[] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};

// QWERTY layout - uppercase
static const char* s_layout_upper[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};

// Symbols layout
static const char* s_layout_symbols[] = {"!@#$%^&*()", "-_=+[]{}|\\", ";:'\"<>,.?/", "`~"};

// Special key IDs (negative values)
#define KEY_BACKSPACE -1
#define KEY_SHIFT -2
#define KEY_SYMBOLS -3
#define KEY_SPACE -4
#define KEY_ENTER -5
#define KEY_CANCEL -6
#define KEY_CLEAR -7
#define KEY_SHOW_HIDE -8

// Key structure
struct KeyInfo {
        int x, y, w, h;
        char ch;      // Character (0 if special key)
        int special;  // Special key ID (0 if regular char)
};

#define MAX_KEYS 60
static KeyInfo s_keys[MAX_KEYS];
static int s_key_count = 0;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void build_layout() {
    s_key_count = 0;

    const char** layout = s_symbols_mode   ? s_layout_symbols
                          : s_shift_active ? s_layout_upper
                                           : s_layout_lower;

    int padding = 6;  // Padding from dialog edges
    int start_x = s_x + padding + 4;
    int start_y = s_y + 95;  // Room for title (45) + text display (40) + gap

    // Row offsets for QWERTY stagger
    int row_offsets[] = {0, 0, 15, 35};

    // Build character keys
    for (int row = 0; row < 4; row++) {
        const char* row_keys = layout[row];
        int row_len = strlen(row_keys);
        int key_x = start_x + row_offsets[row];
        int key_y = start_y + row * (KEYBOARD_KEY_HEIGHT + KEYBOARD_KEY_GAP);

        for (int i = 0; i < row_len && s_key_count < MAX_KEYS; i++) {
            s_keys[s_key_count].x = key_x;
            s_keys[s_key_count].y = key_y;
            s_keys[s_key_count].w = KEYBOARD_KEY_WIDTH;
            s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
            s_keys[s_key_count].ch = row_keys[i];
            s_keys[s_key_count].special = 0;
            s_key_count++;

            key_x += KEYBOARD_KEY_WIDTH + KEYBOARD_KEY_GAP;
        }
    }

    // Special keys on bottom row
    int bottom_y = start_y + 4 * (KEYBOARD_KEY_HEIGHT + KEYBOARD_KEY_GAP);

    // Shift key
    s_keys[s_key_count].x = start_x;
    s_keys[s_key_count].y = bottom_y;
    s_keys[s_key_count].w = 60;
    s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
    s_keys[s_key_count].ch = 0;
    s_keys[s_key_count].special = KEY_SHIFT;
    s_key_count++;

    // Symbols key
    s_keys[s_key_count].x = start_x + 65;
    s_keys[s_key_count].y = bottom_y;
    s_keys[s_key_count].w = 60;
    s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
    s_keys[s_key_count].ch = 0;
    s_keys[s_key_count].special = KEY_SYMBOLS;
    s_key_count++;

    // Space bar
    s_keys[s_key_count].x = start_x + 130;
    s_keys[s_key_count].y = bottom_y;
    s_keys[s_key_count].w = 230;
    s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
    s_keys[s_key_count].ch = ' ';
    s_keys[s_key_count].special = KEY_SPACE;
    s_key_count++;

    // Clear (deletes last character)
    s_keys[s_key_count].x = start_x + 365;
    s_keys[s_key_count].y = bottom_y;
    s_keys[s_key_count].w = 80;
    s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
    s_keys[s_key_count].ch = 0;
    s_keys[s_key_count].special = KEY_CLEAR;
    s_key_count++;

    // Enter (OK)
    s_keys[s_key_count].x = start_x + 450;
    s_keys[s_key_count].y = bottom_y;
    s_keys[s_key_count].w = 90;
    s_keys[s_key_count].h = KEYBOARD_KEY_HEIGHT;
    s_keys[s_key_count].ch = 0;
    s_keys[s_key_count].special = KEY_ENTER;
    s_key_count++;

    // Cancel (X) at top right - aligned with title row
    s_keys[s_key_count].x = s_x + KEYBOARD_WIDTH - 46;
    s_keys[s_key_count].y = s_y + 6;
    s_keys[s_key_count].w = 40;
    s_keys[s_key_count].h = 38;
    s_keys[s_key_count].ch = 0;
    s_keys[s_key_count].special = KEY_CANCEL;
    s_key_count++;

    // Show/Hide password button (only in password mode) - aligned with text display row
    if (s_password_mode) {
        s_keys[s_key_count].x = s_x + KEYBOARD_WIDTH - 72;
        s_keys[s_key_count].y = s_y + 50;
        s_keys[s_key_count].w = 66;
        s_keys[s_key_count].h = 38;
        s_keys[s_key_count].ch = 0;
        s_keys[s_key_count].special = KEY_SHOW_HIDE;
        s_key_count++;
    }
}

static const char* get_key_label(int idx) {
    static char label[2] = {0, 0};

    if (s_keys[idx].ch != 0 && s_keys[idx].special != KEY_SPACE) {
        label[0] = s_keys[idx].ch;
        return label;
    }

    switch (s_keys[idx].special) {
        case KEY_SHIFT:
            return s_shift_active ? "ABC" : "abc";
        case KEY_SYMBOLS:
            return s_symbols_mode ? "abc" : "!@#";
        case KEY_SPACE:
            return "SPACE";
        case KEY_ENTER:
            return "OK";
        case KEY_CANCEL:
            return "X";
        case KEY_CLEAR:
            return "DEL";
        case KEY_SHOW_HIDE:
            return s_password_visible ? "Hide" : "Show";
        default:
            return "?";
    }
}

static uint16_t get_key_color(int idx, bool pressed) {
    if (pressed) {
        return UI_COLOR_BTN_PRESSED;
    }

    switch (s_keys[idx].special) {
        case KEY_ENTER:
            return UI_COLOR_SUCCESS;
        case KEY_CANCEL:
            return UI_COLOR_DANGER;
        case KEY_CLEAR:
            return UI_COLOR_WARNING;
        case KEY_SHIFT:
            return s_shift_active ? UI_COLOR_ACCENT : UI_COLOR_ACCENT_DIM;
        case KEY_SYMBOLS:
            return s_symbols_mode ? UI_COLOR_ACCENT : UI_COLOR_ACCENT_DIM;
        case KEY_SHOW_HIDE:
            return s_password_visible ? UI_COLOR_ACCENT : UI_COLOR_ACCENT_DIM;
        default:
            return UI_COLOR_BTN_PRIMARY;
    }
}

static void draw_key(int idx, bool pressed) {
    KeyInfo& key = s_keys[idx];
    uint16_t bg_color = get_key_color(idx, pressed);
    const char* label = get_key_label(idx);

    ui_draw_button(key.x, key.y, key.w, key.h, label, bg_color, UI_COLOR_TEXT_PRIMARY,
                   UI_TEXT_SIZE_NORMAL, UI_RADIUS_SMALL);
}

static void draw_text_display() {
    int padding = 6;
    int disp_x = s_x + padding;
    int disp_y = s_y + 50;  // Below title row (45px)
    int disp_w =
            s_password_mode ? (KEYBOARD_WIDTH - padding * 2 - 72) : (KEYBOARD_WIDTH - padding * 2);
    int disp_h = 38;

    // Background
    gfx.fillRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_BG_DARK);
    gfx.drawRoundRect(disp_x, disp_y, disp_w, disp_h, UI_RADIUS_SMALL, UI_COLOR_BORDER);

    // Text
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx_setCursor(disp_x + 10, disp_y + 10);

    // Show asterisks only if password mode AND not visible
    if (s_password_mode && !s_password_visible) {
        int len = strlen(s_text);
        for (int i = 0; i < len; i++) {
            gfx.print("*");
        }
    } else {
        gfx.print(s_text);
    }

    // Cursor — measure actual rendered text width for correct positioning
    int text_w;
    if (s_password_mode && !s_password_visible) {
        text_w = strlen(s_text) * gfx.textWidth("*");
    } else {
        text_w = gfx.textWidth(s_text);
    }
    int cursor_x = disp_x + 10 + text_w;
    if (cursor_x < disp_x + disp_w - 10) {
        gfx.fillRect(cursor_x, disp_y + 8, 2, disp_h - 16, UI_COLOR_TEXT_PRIMARY);
    }
}

static int find_key_at(int touch_x, int touch_y) {
    for (int i = 0; i < s_key_count; i++) {
        if (ui_touch_in_rect(touch_x, touch_y, s_keys[i].x, s_keys[i].y, s_keys[i].w,
                             s_keys[i].h)) {
            return i;
        }
    }
    return -1;
}

static void process_key(int idx) {
    if (s_keys[idx].special == 0 || s_keys[idx].special == KEY_SPACE) {
        // Regular character or space
        int len = strlen(s_text);
        if (len < s_max_length) {
            s_text[len] = s_keys[idx].ch;
            s_text[len + 1] = '\0';
            draw_text_display();
        }
    } else {
        switch (s_keys[idx].special) {
            case KEY_CLEAR: {
                int len = strlen(s_text);
                if (len > 0) {
                    s_text[len - 1] = '\0';
                    draw_text_display();
                }
            } break;

            case KEY_SHIFT:
                s_shift_active = !s_shift_active;
                if (s_shift_active)
                    s_symbols_mode = false;
                build_layout();
                keyboard_draw();
                break;

            case KEY_SYMBOLS:
                s_symbols_mode = !s_symbols_mode;
                if (s_symbols_mode)
                    s_shift_active = false;
                build_layout();
                keyboard_draw();
                break;

            case KEY_ENTER:
                // Hide keyboard BEFORE callback so caller can redraw cleanly
                keyboard_hide();
                if (s_callback) {
                    s_callback(s_text, s_user_data);
                }
                break;

            case KEY_CANCEL:
                keyboard_hide();
                break;

            case KEY_SHOW_HIDE:
                s_password_visible = !s_password_visible;
                draw_text_display();
                // Redraw the show/hide button to update label
                for (int i = 0; i < s_key_count; i++) {
                    if (s_keys[i].special == KEY_SHOW_HIDE) {
                        draw_key(i, false);
                        break;
                    }
                }
                break;
        }
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void keyboard_show(int pos_x, int pos_y, const char* title, const char* initial_text,
                   int max_length, keyboard_callback_t callback, void* user_data,
                   bool password_mode) {
    s_x = pos_x;
    s_y = pos_y;
    s_max_length = (max_length > KEYBOARD_MAX_TEXT) ? KEYBOARD_MAX_TEXT : max_length;
    s_callback = callback;
    s_user_data = user_data;
    s_password_mode = password_mode;
    s_password_visible = !password_mode;  // If password mode, start hidden; otherwise visible
    s_shift_active = false;
    s_symbols_mode = false;
    s_pressed_key = -1;

    // Store title
    if (title) {
        strncpy(s_title, title, sizeof(s_title) - 1);
        s_title[sizeof(s_title) - 1] = '\0';
    } else {
        s_title[0] = '\0';
    }

    // Initialize text
    if (initial_text) {
        strncpy(s_text, initial_text, s_max_length);
        s_text[s_max_length] = '\0';
    } else {
        s_text[0] = '\0';
    }

    s_visible = true;
    build_layout();
    keyboard_draw();
}

void keyboard_hide() {
    s_visible = false;
}

bool keyboard_is_visible() {
    return s_visible;
}

bool keyboard_handle_touch(int touch_x, int touch_y, bool pressed) {
    if (!s_visible) {
        return false;
    }

    // Handle touch release first - process even if coordinates are outside bounds
    // (touch coordinates may be invalid on release)
    if (!pressed) {
        if (s_pressed_key >= 0) {
            draw_key(s_pressed_key, false);
            process_key(s_pressed_key);
            s_pressed_key = -1;
            return true;
        }
        return false;
    }

    // Check if touch is within keyboard bounds for press events
    if (!ui_touch_in_rect(touch_x, touch_y, s_x, s_y, KEYBOARD_WIDTH, KEYBOARD_HEIGHT)) {
        return false;
    }

    int key = find_key_at(touch_x, touch_y);
    if (key >= 0 && key != s_pressed_key) {
        if (s_pressed_key >= 0) {
            draw_key(s_pressed_key, false);
        }
        s_pressed_key = key;
        draw_key(s_pressed_key, true);
    }

    return true;
}

void keyboard_draw() {
    if (!s_visible) {
        return;
    }

    // Draw darkened backdrop over the screen
    ui_draw_modal_backdrop();

    // Draw modal shadow/outline effect (darker outer border)
    gfx.drawRoundRect(s_x - 3, s_y - 3, KEYBOARD_WIDTH + 6, KEYBOARD_HEIGHT + 6,
                      UI_RADIUS_NORMAL + 3, 0x0000);
    gfx.drawRoundRect(s_x - 2, s_y - 2, KEYBOARD_WIDTH + 4, KEYBOARD_HEIGHT + 4,
                      UI_RADIUS_NORMAL + 2, 0x0000);
    gfx.drawRoundRect(s_x - 1, s_y - 1, KEYBOARD_WIDTH + 2, KEYBOARD_HEIGHT + 2,
                      UI_RADIUS_NORMAL + 1, 0x0000);

    // Background
    gfx.fillRoundRect(s_x, s_y, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, UI_RADIUS_NORMAL,
                      UI_COLOR_BG_PANEL);

    // Visible border (thick bright accent for modal effect)
    gfx.drawRoundRect(s_x, s_y, KEYBOARD_WIDTH, KEYBOARD_HEIGHT, UI_RADIUS_NORMAL, UI_COLOR_ACCENT);
    gfx.drawRoundRect(s_x + 1, s_y + 1, KEYBOARD_WIDTH - 2, KEYBOARD_HEIGHT - 2,
                      UI_RADIUS_NORMAL - 1, UI_COLOR_ACCENT);
    gfx.drawRoundRect(s_x + 2, s_y + 2, KEYBOARD_WIDTH - 4, KEYBOARD_HEIGHT - 4,
                      UI_RADIUS_NORMAL - 2, UI_COLOR_BORDER);

    // Title - use custom title if provided, otherwise default
    // Title row is 45px tall, text centered vertically
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    gfx.setTextColor(UI_COLOR_TEXT_SECONDARY);
    gfx_setCursor(s_x + 10, s_y + 16);
    if (s_title[0] != '\0') {
        gfx.print(s_title);
    } else {
        gfx.print(s_password_mode ? "Enter Password" : "Enter Text");
    }

    // Text display
    draw_text_display();

    // All keys
    for (int i = 0; i < s_key_count; i++) {
        draw_key(i, i == s_pressed_key);
    }
}

const char* keyboard_get_text() {
    return s_text;
}
