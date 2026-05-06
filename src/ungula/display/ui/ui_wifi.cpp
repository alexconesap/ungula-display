// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_wifi.cpp
 * @brief Reusable WiFi network selector component implementation
 */

#include "ui_wifi.h"

#include <stdio.h>
#include <string.h>

#include "ui_keyboard.h"
#include "ui_widgets.h"

using namespace ungula::display;

// ============================================================================
// STRING HOOK
// ============================================================================

static const char* const s_default_strings[WIFI_STR_COUNT] = {
        "WiFi Networks",         // WIFI_STR_TITLE
        "Scanning...",           // WIFI_STR_SCANNING
        "No networks found",     // WIFI_STR_NO_NETWORKS
        "Push refresh to scan",  // WIFI_STR_PUSH_SCAN
        "Refresh Networks",      // WIFI_STR_BTN_SCAN
        "CONNECT",               // WIFI_STR_BTN_CONNECT
        "Password for: %s",      // WIFI_STR_PASSWORD_FOR
        "Enable Internet",       // WIFI_STR_ENABLE
};

static wifi_string_hook_t s_string_hook = nullptr;

void wifi_set_string_hook(wifi_string_hook_t hook) {
    s_string_hook = hook;
}

static const char* wstr(WifiStringId id) {
    if (s_string_hook)
        return s_string_hook(id);
    return s_default_strings[id];
}

// ============================================================================
// INTERNAL STATE
// ============================================================================

static bool s_visible = false;
static bool s_enabled = true;  // checkbox state — controls body visibility
static int s_x = 0;
static int s_y = 0;
static wifi_connect_callback_t s_connect_callback = nullptr;
static wifi_scan_callback_t s_scan_callback = nullptr;
static wifi_enable_callback_t s_enable_callback = nullptr;
static void* s_user_data = nullptr;

// Network list
static WiFiNetworkInfo s_networks[WIFI_MAX_NETWORKS];
static int s_network_count = 0;
static int s_selected_idx = -1;
static int s_scroll_offset = 0;

// State
static bool s_scanning = false;
static bool s_has_scanned = false;
static char s_status_msg[64] = {0};
static bool s_status_is_error = false;
static bool s_entering_password = false;
static char s_password[33] = {0};
static char s_connected_ssid[33] = {0};

// Button tracking
static int s_pressed_btn = -1;

// Layout constants (adjusted for 780x390 panel)
#define WIFI_LIST_X (s_x + 10)
#define WIFI_LIST_Y (s_y + 92)
#define WIFI_LIST_W (WIFI_SELECTOR_WIDTH - 20)
#define WIFI_LIST_H 224
#define WIFI_ITEM_H 37
#define WIFI_VISIBLE_ITEMS 6

// Button layout (centered)
#define WIFI_BTN_W 230
#define WIFI_BTN_GAP 20
#define WIFI_BTN_START_X (s_x + ((WIFI_SELECTOR_WIDTH - (2 * WIFI_BTN_W) - WIFI_BTN_GAP) / 2))

// Close button (top-right of panel)
#define WIFI_CLOSE_SIZE 40
#define WIFI_CLOSE_MARGIN 10
#define WIFI_CLOSE_X (s_x + WIFI_SELECTOR_WIDTH - WIFI_CLOSE_SIZE - WIFI_CLOSE_MARGIN)
#define WIFI_CLOSE_Y (s_y + WIFI_CLOSE_MARGIN)

// Scroll buttons aligned with close button
#define WIFI_SCROLL_BTN_X WIFI_CLOSE_X
#define WIFI_SCROLL_BTN_W WIFI_CLOSE_SIZE

// Checkbox layout (below title, left side)
#define WIFI_CB_X (s_x + 15)
#define WIFI_CB_Y (s_y + 60)
#define WIFI_CB_SIZE 24
#define WIFI_CB_LABEL_X (WIFI_CB_X + WIFI_CB_SIZE + 8)
#define WIFI_CB_TOUCH_W 250
#define WIFI_CB_TOUCH_H 30

// Status line (right of checkbox, same row)
#define WIFI_STATUS_Y (s_y + 64)
#define WIFI_STATUS_H 14

// Buttons
#define BTN_SCAN 0
#define BTN_CONNECT 1
#define BTN_CANCEL 2
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_ENABLE_CB 5

// ============================================================================
// SIGNAL STRENGTH ICONS (simple bars)
// ============================================================================

static void draw_signal_bars(int pos_x, int pos_y, int rssi) {
    // Convert RSSI to 0-4 bars
    int bars;
    if (rssi >= -50) {
        bars = 4;
    } else if (rssi >= -60) {
        bars = 3;
    } else if (rssi >= -70) {
        bars = 2;
    } else if (rssi >= -80) {
        bars = 1;
    } else {
        bars = 0;
    }

    int bar_w = 4;
    int bar_gap = 2;
    int heights[] = {6, 10, 14, 18};

    for (int i = 0; i < 4; i++) {
        uint16_t color = (i < bars) ? UI_COLOR_SUCCESS : UI_COLOR_ACCENT_DIM;
        int bar_h = heights[i];
        int bar_y = pos_y + 18 - bar_h;
        gfx_fillRect(pos_x + (i * (bar_w + bar_gap)), bar_y, bar_w, bar_h, color);
    }
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static bool is_connected_network(int idx) {
    return s_connected_ssid[0] != '\0' && strcmp(s_networks[idx].ssid, s_connected_ssid) == 0;
}

static void draw_network_item(int idx, bool selected) {
    if (idx < 0 || idx >= s_network_count) {
        return;
    }

    int display_idx = idx - s_scroll_offset;
    if (display_idx < 0 || display_idx >= WIFI_VISIBLE_ITEMS) {
        return;
    }

    int item_x = WIFI_LIST_X + 1;
    int item_y = WIFI_LIST_Y + 1 + (display_idx * WIFI_ITEM_H);
    int item_w = WIFI_LIST_W - 50 - 2;
    bool connected = is_connected_network(idx);

    // Background — connected network gets a subtle green tint
    uint16_t bg_color = selected ? UI_COLOR_ACCENT : (connected ? 0x0320 : UI_COLOR_BG_DARK);
    gfx_fillRoundRect(item_x, item_y, item_w, WIFI_ITEM_H - 3, UI_RADIUS_SMALL, bg_color);

    // Lock icon for secured networks
    if (s_networks[idx].secure) {
        gfx_set_font(UI_TEXT_SIZE_SMALL);
        gfx_setTextColor(UI_COLOR_TEXT_SECONDARY);
        gfx_setCursor(item_x + 5, item_y + 14);
        gfx_print("*");
    }

    // SSID
    gfx_set_font(UI_TEXT_SIZE_NORMAL);
    uint16_t text_color = selected ? UI_COLOR_BG_DARK : UI_COLOR_TEXT_PRIMARY;
    gfx_setTextColor(text_color);
    gfx_setCursor(item_x + 20, item_y + 11);
    gfx_print(s_networks[idx].ssid);

    // Connected indicator (small check mark after SSID)
    if (connected && !selected) {
        gfx_setTextColor(UI_COLOR_SUCCESS);
        gfx_print(" ok");
    }

    // Signal strength bars
    draw_signal_bars(item_x + item_w - 35, item_y + 11, s_networks[idx].rssi);
}

static void draw_network_list() {
    // List background
    gfx_fillRoundRect(WIFI_LIST_X, WIFI_LIST_Y, WIFI_LIST_W - 50, WIFI_LIST_H, UI_RADIUS_NORMAL,
                      UI_COLOR_BG_PANEL);
    gfx_drawRoundRect(WIFI_LIST_X, WIFI_LIST_Y, WIFI_LIST_W - 50, WIFI_LIST_H, UI_RADIUS_NORMAL,
                      UI_COLOR_BORDER_DIM);

    if (s_scanning) {
        // Show scanning message
        gfx_set_font(UI_TEXT_SIZE_NORMAL);
        gfx_setTextColor(UI_COLOR_TEXT_SECONDARY);
        gfx_drawCentreString(wstr(WIFI_STR_SCANNING), WIFI_LIST_X + ((WIFI_LIST_W - 50) / 2),
                             WIFI_LIST_Y + (WIFI_LIST_H / 2) - 8);
    } else if (s_network_count == 0) {
        // Show "push refresh" on first load, "no networks found" after a scan returned empty
        auto msg = s_has_scanned ? WIFI_STR_NO_NETWORKS : WIFI_STR_PUSH_SCAN;
        gfx_set_font(UI_TEXT_SIZE_NORMAL);
        gfx_setTextColor(UI_COLOR_TEXT_SECONDARY);
        gfx_drawCentreString(wstr(msg), WIFI_LIST_X + ((WIFI_LIST_W - 50) / 2),
                             WIFI_LIST_Y + (WIFI_LIST_H / 2) - 8);
    } else {
        // Draw visible items
        for (int i = s_scroll_offset;
             i < s_network_count && i < s_scroll_offset + WIFI_VISIBLE_ITEMS; i++) {
            draw_network_item(i, i == s_selected_idx);
        }
    }

    // Scroll buttons (aligned with X button)
    bool can_scroll_up = s_scroll_offset > 0;
    bool can_scroll_down = s_scroll_offset + WIFI_VISIBLE_ITEMS < s_network_count;

    ui_draw_button(WIFI_SCROLL_BTN_X, WIFI_LIST_Y, WIFI_SCROLL_BTN_W, WIFI_SCROLL_BTN_W, "^",
                   can_scroll_up ? UI_COLOR_BTN_PRIMARY : UI_COLOR_ACCENT_DIM,
                   UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_NORMAL, UI_RADIUS_SMALL);

    ui_draw_button(WIFI_SCROLL_BTN_X, WIFI_LIST_Y + WIFI_LIST_H - WIFI_SCROLL_BTN_W,
                   WIFI_SCROLL_BTN_W, WIFI_SCROLL_BTN_W, "v",
                   can_scroll_down ? UI_COLOR_BTN_PRIMARY : UI_COLOR_ACCENT_DIM,
                   UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_NORMAL, UI_RADIUS_SMALL);
}

static void draw_button_with_state(int btn_id) {
    int btn_y = s_y + WIFI_SELECTOR_HEIGHT - 55;
    int btn_h = 45;
    bool pressed = (s_pressed_btn == btn_id);

    switch (btn_id) {
        case BTN_SCAN: {
            uint16_t color = pressed ? UI_COLOR_BTN_PRESSED
                                     : (s_scanning ? UI_COLOR_ACCENT_DIM : UI_COLOR_BTN_PRIMARY);
            ui_draw_button(WIFI_BTN_START_X, btn_y, WIFI_BTN_W, btn_h, wstr(WIFI_STR_BTN_SCAN),
                           color, UI_COLOR_TEXT_PRIMARY, UI_TEXT_SIZE_NORMAL, UI_RADIUS_NORMAL);
            break;
        }
        case BTN_CONNECT: {
            bool can_connect = (s_selected_idx >= 0 && !s_scanning);
            uint16_t color = pressed ? UI_COLOR_BTN_PRESSED
                                     : (can_connect ? UI_COLOR_SUCCESS : UI_COLOR_ACCENT_DIM);
            ui_draw_button(WIFI_BTN_START_X + WIFI_BTN_W + WIFI_BTN_GAP, btn_y, WIFI_BTN_W, btn_h,
                           wstr(WIFI_STR_BTN_CONNECT), color, UI_COLOR_TEXT_PRIMARY,
                           UI_TEXT_SIZE_NORMAL, UI_RADIUS_NORMAL);
            break;
        }
        case BTN_CANCEL: {
            ui_draw_close_button(WIFI_CLOSE_X, WIFI_CLOSE_Y, WIFI_CLOSE_SIZE, pressed);
            break;
        }
        default:
            break;
    }
}

static void draw_buttons() {
    draw_button_with_state(BTN_SCAN);
    draw_button_with_state(BTN_CONNECT);
    draw_button_with_state(BTN_CANCEL);
}

static void draw_enable_checkbox() {
    // Clear checkbox area
    gfx_fillRect(WIFI_CB_X - 2, WIFI_CB_Y - 2, WIFI_CB_TOUCH_W + 4, WIFI_CB_TOUCH_H,
                 UI_COLOR_BG_PANEL);

    // Checkbox border
    gfx_drawRoundRect(WIFI_CB_X, WIFI_CB_Y, WIFI_CB_SIZE, WIFI_CB_SIZE, 4, UI_COLOR_TEXT_PRIMARY);

    // Fill if checked
    if (s_enabled) {
        gfx_fillRoundRect(WIFI_CB_X + 3, WIFI_CB_Y + 3, WIFI_CB_SIZE - 6, WIFI_CB_SIZE - 6, 2,
                          UI_COLOR_ACCENT);
    }

    // Label
    gfx_set_font(UI_TEXT_SIZE_SMALL);
    uint16_t labelColor = s_enabled ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_SECONDARY;
    gfx_setTextColor(labelColor);
    gfx_setCursor(WIFI_CB_LABEL_X, WIFI_CB_Y + 5);
    gfx_print(wstr(WIFI_STR_ENABLE));
}

static void draw_status_line() {
    // Clear the status area (right portion of the checkbox row)
    int statusX = s_x + WIFI_CB_TOUCH_W + 30;
    int statusW = WIFI_SELECTOR_WIDTH - WIFI_CB_TOUCH_W - 80;
    gfx_fillRect(statusX, WIFI_STATUS_Y, statusW, WIFI_STATUS_H, UI_COLOR_BG_PANEL);

    const char* text = nullptr;
    uint16_t color = UI_COLOR_TEXT_SECONDARY;

    if (!s_enabled) {
        // No status when disabled
    } else if (s_connected_ssid[0] != '\0') {
        snprintf(s_status_msg, sizeof(s_status_msg), "Connected to %s", s_connected_ssid);
        text = s_status_msg;
        color = UI_COLOR_SUCCESS;
    } else {
        text = "Not connected";
    }

    if (text) {
        gfx_set_font(UI_TEXT_SIZE_SMALL);
        gfx_setTextColor(color);
        gfx_setCursor(statusX, WIFI_STATUS_Y);
        gfx_print(text);
    }
}

static int find_network_at(int touch_x, int touch_y) {
    if (touch_x < WIFI_LIST_X || touch_x >= WIFI_LIST_X + WIFI_LIST_W - 50) {
        return -1;
    }
    if (touch_y < WIFI_LIST_Y || touch_y >= WIFI_LIST_Y + WIFI_LIST_H) {
        return -1;
    }

    int item_idx = ((touch_y - WIFI_LIST_Y) / WIFI_ITEM_H) + s_scroll_offset;
    if (item_idx >= 0 && item_idx < s_network_count) {
        return item_idx;
    }
    return -1;
}

static int find_button_at(int touch_x, int touch_y) {
    int btn_y = s_y + WIFI_SELECTOR_HEIGHT - 55;
    int btn_h = 45;

    // Scan (centered)
    if (ui_touch_in_rect(touch_x, touch_y, WIFI_BTN_START_X, btn_y, WIFI_BTN_W, btn_h)) {
        return BTN_SCAN;
    }
    // Connect (centered, next to Scan)
    if (ui_touch_in_rect(touch_x, touch_y, WIFI_BTN_START_X + WIFI_BTN_W + WIFI_BTN_GAP, btn_y,
                         WIFI_BTN_W, btn_h)) {
        return BTN_CONNECT;
    }
    // Close button (top-right X)
    if (ui_touch_on_close_button(touch_x, touch_y, WIFI_CLOSE_X, WIFI_CLOSE_Y, WIFI_CLOSE_SIZE)) {
        return BTN_CANCEL;
    }

    // Scroll up (aligned with X button)
    if (ui_touch_in_rect(touch_x, touch_y, WIFI_SCROLL_BTN_X, WIFI_LIST_Y, WIFI_SCROLL_BTN_W,
                         WIFI_SCROLL_BTN_W)) {
        return BTN_UP;
    }
    // Scroll down (aligned with X button)
    if (ui_touch_in_rect(touch_x, touch_y, WIFI_SCROLL_BTN_X,
                         WIFI_LIST_Y + WIFI_LIST_H - WIFI_SCROLL_BTN_W, WIFI_SCROLL_BTN_W,
                         WIFI_SCROLL_BTN_W)) {
        return BTN_DOWN;
    }

    // Enable checkbox (title row)
    if (ui_touch_in_rect(touch_x, touch_y, WIFI_CB_X - 2, WIFI_CB_Y - 2, WIFI_CB_TOUCH_W,
                         WIFI_CB_TOUCH_H)) {
        return BTN_ENABLE_CB;
    }

    return -1;
}

// Password callback
static void on_password_entered(const char* password, void* user_data) {
    s_entering_password = false;
    strncpy(s_password, password, sizeof(s_password) - 1);
    s_password[sizeof(s_password) - 1] = '\0';

    // Clear screen before redrawing
    gfx_fillScreen(UI_COLOR_BG_DARK);

    // Redraw selector so status message appears on clean background
    wifi_selector_draw();

    // Trigger connection (this may set status message)
    if (s_connect_callback != nullptr && s_selected_idx >= 0) {
        s_connect_callback(s_networks[s_selected_idx].ssid, s_password, s_user_data);
    }
}

static void start_connect() {
    if (s_selected_idx < 0) {
        return;
    }

    if (s_networks[s_selected_idx].secure) {
        // Need password - show keyboard
        s_entering_password = true;
        s_password[0] = '\0';

        // Clear the entire screen first for clean keyboard display
        gfx_fillScreen(UI_COLOR_BG_DARK);

        // Build title with SSID
        char title[64];
        snprintf(title, sizeof(title), wstr(WIFI_STR_PASSWORD_FOR),
                 s_networks[s_selected_idx].ssid);

        // Center keyboard on screen
        int kb_x = (UI_SCREEN_WIDTH - KEYBOARD_WIDTH) / 2;
        int kb_y = (UI_SCREEN_HEIGHT - KEYBOARD_HEIGHT) / 2;

        keyboard_show(kb_x, kb_y, title, "", 32, on_password_entered, nullptr, true);
    } else {
        // Open network - connect directly
        if (s_connect_callback != nullptr) {
            s_connect_callback(s_networks[s_selected_idx].ssid, "", s_user_data);
        }
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

void wifi_selector_show(int pos_x, int pos_y, wifi_connect_callback_t connect_callback,
                        wifi_scan_callback_t scan_callback, wifi_enable_callback_t enable_callback,
                        bool enabled, void* user_data) {
    s_x = pos_x;
    s_y = pos_y;
    s_connect_callback = connect_callback;
    s_scan_callback = scan_callback;
    s_enable_callback = enable_callback;
    s_enabled = enabled;
    s_user_data = user_data;

    // Keep cached networks (s_network_count preserved) — only reset selection state
    s_selected_idx = -1;
    s_scroll_offset = 0;
    s_scanning = false;
    s_has_scanned = (s_network_count > 0);  // show "push refresh" only if no cached networks
    s_status_msg[0] = '\0';
    s_status_is_error = false;
    s_entering_password = false;
    s_pressed_btn = -1;

    s_visible = true;
    wifi_selector_draw();
}

void wifi_selector_hide() {
    s_visible = false;
    if (keyboard_is_visible()) {
        keyboard_hide();
    }
}

bool wifi_selector_is_visible() {
    return s_visible;
}

void wifi_set_networks(const WiFiNetworkInfo* networks, int count) {
    // Clear existing networks first
    memset(s_networks, 0, sizeof(s_networks));

    s_has_scanned = true;
    s_network_count = (count > WIFI_MAX_NETWORKS) ? WIFI_MAX_NETWORKS : count;

    for (int i = 0; i < s_network_count; i++) {
        memcpy(&s_networks[i], &networks[i], sizeof(WiFiNetworkInfo));
    }

    s_selected_idx = -1;
    s_scroll_offset = 0;
    s_scanning = false;

    if (s_visible && !s_entering_password) {
        wifi_selector_draw();
    }
}

void wifi_set_scanning(bool scanning) {
    s_scanning = scanning;
    if (s_visible && !s_entering_password) {
        draw_network_list();
    }
}

void wifi_set_status(const char* message, bool is_error) {
    if (message != nullptr && message[0] != '\0') {
        strncpy(s_status_msg, message, sizeof(s_status_msg) - 1);
        s_status_msg[sizeof(s_status_msg) - 1] = '\0';
    } else {
        s_status_msg[0] = '\0';
    }
    s_status_is_error = is_error;

    if (s_visible && !s_entering_password) {
        draw_status_line();
    }
}

bool wifi_selector_handle_touch(int touch_x, int touch_y, bool pressed) {
    if (!s_visible) {
        return false;
    }

    // If keyboard is showing, delegate to it
    if (keyboard_is_visible()) {
        bool handled = keyboard_handle_touch(touch_x, touch_y, pressed);
        if (!keyboard_is_visible()) {
            // Keyboard was closed (Cancel pressed) - clear screen and redraw selector
            s_entering_password = false;
            gfx_fillScreen(UI_COLOR_BG_DARK);
            wifi_selector_draw();
        }
        return handled;
    }

    // Handle touch release - process even if coordinates are outside bounds
    // (touch coordinates may be invalid on release)
    if (!pressed) {
        if (s_pressed_btn >= 0) {
            int btn = s_pressed_btn;
            s_pressed_btn = -1;

            // Restore button appearance first (for buttons that don't cause full redraw)
            if (btn == BTN_SCAN || btn == BTN_CONNECT || btn == BTN_CANCEL) {
                draw_button_with_state(btn);
            }

            switch (btn) {
                case BTN_SCAN:
                    if (s_scan_callback != nullptr && !s_scanning) {
                        s_scanning = true;
                        s_network_count = 0;
                        s_selected_idx = -1;
                        wifi_selector_draw();
                        s_scan_callback(s_user_data);
                    }
                    break;

                case BTN_CONNECT:
                    if (s_selected_idx >= 0 && !s_scanning) {
                        start_connect();
                    }
                    break;

                case BTN_CANCEL:
                    wifi_selector_hide();
                    break;

                case BTN_UP:
                    if (s_scroll_offset > 0) {
                        s_scroll_offset--;
                        draw_network_list();
                    }
                    break;

                case BTN_DOWN:
                    if (s_scroll_offset + WIFI_VISIBLE_ITEMS < s_network_count) {
                        s_scroll_offset++;
                        draw_network_list();
                    }
                    break;

                case BTN_ENABLE_CB:
                    s_enabled = !s_enabled;
                    wifi_selector_draw();
                    if (s_enable_callback != nullptr) {
                        s_enable_callback(s_enabled, s_user_data);
                    }
                    break;
            }
            return true;
        }
        return false;
    }

    // Check bounds for press events
    if (!ui_touch_in_rect(touch_x, touch_y, s_x, s_y, WIFI_SELECTOR_WIDTH, WIFI_SELECTOR_HEIGHT)) {
        return false;
    }

    // Touch pressed - check for network selection
    int net_idx = find_network_at(touch_x, touch_y);
    if (net_idx >= 0 && net_idx != s_selected_idx) {
        int old_idx = s_selected_idx;
        s_selected_idx = net_idx;
        if (old_idx >= 0) {
            draw_network_item(old_idx, false);
        }
        draw_network_item(s_selected_idx, true);
        draw_buttons();  // Update connect button state
        return true;
    }

    // Check buttons - show pressed state
    int btn = find_button_at(touch_x, touch_y);
    if (btn >= 0 && btn != s_pressed_btn) {
        s_pressed_btn = btn;
        // Draw button in pressed state
        if (btn == BTN_SCAN || btn == BTN_CONNECT || btn == BTN_CANCEL) {
            draw_button_with_state(btn);
        }
    }
    return true;
}

void wifi_selector_set_enabled(bool enabled) {
    if (s_enabled == enabled)
        return;
    s_enabled = enabled;
    if (s_visible) {
        wifi_selector_draw();
    }
}

void wifi_selector_draw() {
    if (!s_visible) {
        return;
    }

    // Don't draw if keyboard is showing
    if (keyboard_is_visible()) {
        return;
    }

    // Background
    gfx_fillRoundRect(s_x, s_y, WIFI_SELECTOR_WIDTH, WIFI_SELECTOR_HEIGHT, UI_RADIUS_NORMAL,
                      UI_COLOR_BG_PANEL);
    gfx_drawRoundRect(s_x, s_y, WIFI_SELECTOR_WIDTH, WIFI_SELECTOR_HEIGHT, UI_RADIUS_NORMAL,
                      UI_COLOR_BORDER);

    // Title (centered)
    gfx_set_font(UI_TEXT_SIZE_LARGE);
    gfx_setTextColor(UI_COLOR_TEXT_HEADER);
    gfx_drawCentreString(wstr(WIFI_STR_TITLE), s_x + (WIFI_SELECTOR_WIDTH / 2), s_y + 15);

    // Title row: checkbox + close button (always visible)
    draw_enable_checkbox();
    draw_button_with_state(BTN_CANCEL);

    // Status line (always visible, shows Off / Connected / Not connected)
    draw_status_line();

    // Body elements only when enabled
    if (s_enabled) {
        draw_network_list();
        draw_buttons();
    }
}

void wifi_set_connected_ssid(const char* ssid) {
    if (ssid != nullptr && ssid[0] != '\0') {
        strncpy(s_connected_ssid, ssid, sizeof(s_connected_ssid) - 1);
        s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
    } else {
        s_connected_ssid[0] = '\0';
    }
    if (s_visible && !s_entering_password) {
        draw_network_list();
    }
}

const char* wifi_get_selected_ssid() {
    if (s_selected_idx >= 0 && s_selected_idx < s_network_count) {
        return s_networks[s_selected_idx].ssid;
    }
    return nullptr;
}
