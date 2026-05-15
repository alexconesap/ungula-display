// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_keyboard.h
 * @brief Reusable text keyboard component
 *
 * Provides a modal text keyboard overlay for entering text values.
 * Features include:
 * - Full QWERTY layout
 * - Shift for uppercase
 * - Symbols mode
 * - Backspace and Clear
 * - Enter to confirm
 * - Cancel button
 * - Input field with current text
 *
 * Reusable across projects - just include and call the API functions.
 */

#pragma once
#include <cstdint>

#include "ui_theme.h"
#include "ungula/display/gfx_core.h"

using namespace ungula::display;

// Callback function type for when text is confirmed
typedef void (*keyboard_callback_t)(const char *text, void *user_data);

// ============================================================================
// KEYBOARD CONFIGURATION
// ============================================================================

// Maximum text length
#define KEYBOARD_MAX_TEXT 32

// Keyboard button sizes
#define KEYBOARD_KEY_WIDTH 50
#define KEYBOARD_KEY_HEIGHT 40
#define KEYBOARD_KEY_GAP 4

// Overall keyboard dimensions
#define KEYBOARD_WIDTH 560
#define KEYBOARD_HEIGHT 335

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Show the text keyboard overlay
 * @param pos_x X position of keyboard
 * @param pos_y Y position of keyboard
 * @param title Title to display (e.g., "Enter Password for MyNetwork")
 * @param initial_text Initial text to display (can be empty or nullptr)
 * @param max_length Maximum text length (capped at KEYBOARD_MAX_TEXT)
 * @param callback Function to call when text is confirmed
 * @param user_data User data passed to callback
 * @param password_mode If true, initially mask password with asterisks
 */
void keyboard_show(int pos_x, int pos_y, const char *title, const char *initial_text,
                   int max_length, keyboard_callback_t callback, void *user_data = nullptr,
                   bool password_mode = false);

/**
 * @brief Hide the keyboard (cancel without saving)
 */
void keyboard_hide();

/**
 * @brief Check if keyboard is currently visible
 * @return true if keyboard is showing
 */
bool keyboard_is_visible();

/**
 * @brief Handle touch input for keyboard
 * @param touch_x Touch X coordinate
 * @param touch_y Touch Y coordinate
 * @param pressed true if touch is active, false if released
 * @return true if touch was handled by keyboard
 */
bool keyboard_handle_touch(int touch_x, int touch_y, bool pressed);

/**
 * @brief Draw the keyboard (call after show, or to refresh)
 */
void keyboard_draw();

/**
 * @brief Get current input text
 * @return Pointer to current text buffer (do not modify)
 */
const char *keyboard_get_text();
