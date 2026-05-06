// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_keypad.h
 * @brief Reusable numeric keypad component
 *
 * Provides a modal numeric keypad overlay for entering integer values.
 * Features include:
 * - 0-9 digit buttons
 * - Clear (backspace) button
 * - Enter button
 * - Cancel (X) button
 * - Input field with current value
 * - Min/max value validation
 *
 * Reusable across projects - just include and call the API functions.
 */

#pragma once

#include "ui_theme.h"
#include "ungula/display/gfx_core.h"

using namespace ungula::display;

// Callback function type for when value is confirmed
typedef void (*keypad_callback_t)(int value, void* user_data);

// Callback function type for when keypad is cancelled (X button)
typedef void (*keypad_cancel_callback_t)(void* user_data);

// ============================================================================
// KEYPAD CONFIGURATION
// ============================================================================

// Keypad button sizes (larger for easier finger operation)
#define KEYPAD_BTN_WIDTH 80
#define KEYPAD_BTN_HEIGHT 55
#define KEYPAD_BTN_GAP 6

// Calculate overall keypad dimensions
#define KEYPAD_WIDTH (((KEYPAD_BTN_WIDTH + KEYPAD_BTN_GAP) * 3) + 24)
#define KEYPAD_HEIGHT \
    (((KEYPAD_BTN_HEIGHT + KEYPAD_BTN_GAP) * 4) + 120)  // Title (50) + display (50) + gap + buttons

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Show the numeric keypad overlay
 * @param pos_x X position of keypad
 * @param pos_y Y position of keypad
 * @param title Title to display (e.g., "Enter Speed") or nullptr for default
 * @param initial_value Initial value to display
 * @param min_value Minimum allowed value
 * @param max_value Maximum allowed value
 * @param callback Function to call when value is confirmed
 * @param user_data User data passed to callback
 */
void keypad_show(int pos_x, int pos_y, const char* title, int initial_value, int min_value,
                 int max_value, keypad_callback_t callback, void* user_data = nullptr,
                 keypad_cancel_callback_t cancel_callback = nullptr, bool password_mode = false,
                 int max_digits = 0, bool anti_guess = false);

/**
 * @brief Hide the keypad (cancel without saving)
 */
void keypad_hide();

/**
 * @brief Check if keypad is currently visible
 * @return true if keypad is showing
 */
bool keypad_is_visible();

/**
 * @brief Handle touch input for keypad
 * @param touch_x Touch X coordinate
 * @param touch_y Touch Y coordinate
 * @param pressed true if touch is active, false if released
 * @return true if touch was handled by keypad
 */
bool keypad_handle_touch(int touch_x, int touch_y, bool pressed);

/**
 * @brief Draw the keypad (call after show, or to refresh)
 */
void keypad_draw();

/**
 * @brief Get current input value
 * @return Current value entered in keypad
 */
int keypad_get_value();

/**
 * @brief Get raw digit buffer (valid immediately after callback, before next show)
 * @return Null-terminated string with all digits typed by the user
 */
const char* keypad_get_raw_digits();
