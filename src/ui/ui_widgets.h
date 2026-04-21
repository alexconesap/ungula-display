// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_widgets.h
 * @brief Reusable UI widget drawing functions
 *
 * Provides drawing primitives for common UI elements like buttons, panels,
 * LED indicators, value displays, etc. All widgets use the theme colors
 * defined in ui_theme.h.
 *
 * Reusable across projects using the same display hardware.
 */

#pragma once
#include <cstdint>

#include "display/gfx_core.h"
#include "ui_theme.h"

// ============================================================================
// PANEL/CONTAINER WIDGETS
// ============================================================================

/**
 * @brief Draw a panel with optional border
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param fill_color Fill color (use UI_COLOR_* constants)
 * @param border_color Border color (0 for no border)
 * @param radius Corner radius
 */
void ui_draw_panel(int pos_x, int pos_y, int width, int height,
                   uint16_t fill_color = UI_COLOR_BG_PANEL, uint16_t border_color = UI_COLOR_BORDER,
                   int radius = UI_RADIUS_NORMAL);

/**
 * @brief Draw the main content frame (outer border of main area)
 */
void ui_draw_content_frame();

/**
 * @brief Draw a simple box (rectangle border)
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param border_color Border color
 * @param border_width Border width (Defaults to 1px)
 */
void ui_draw_box(int pos_x, int pos_y, int width, int height,
                 uint16_t border_color = UI_COLOR_BORDER, int border_width = 1);

/**
 * @brief Clear the screen with a background color
 * @param background_color Background color to fill
 */
void ui_clear(uint16_t background_color = UI_COLOR_BG_PANEL);

// ============================================================================
// BUTTON WIDGETS
// ============================================================================

/**
 * @brief Draw a text button
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param text Button label
 * @param bg_color Background color
 * @param text_color Text color
 * @param text_size Text size multiplier
 * @param radius Corner radius
 */
void ui_draw_button(int pos_x, int pos_y, int width, int height, const char* text,
                    uint16_t bg_color = UI_COLOR_BTN_PRIMARY,
                    uint16_t text_color = UI_COLOR_TEXT_PRIMARY,
                    int text_size = UI_TEXT_SIZE_NORMAL, int radius = UI_RADIUS_NORMAL);

/**
 * @brief Draw a button with a bitmap icon
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param icon Pointer to icon bitmap data
 * @param icon_w Icon width
 * @param icon_h Icon height
 * @param bg_color Background color
 * @param icon_color Icon color
 * @param radius Corner radius
 */
void ui_draw_icon_button(int pos_x, int pos_y, int width, int height, const uint8_t* icon,
                         int icon_w, int icon_h, uint16_t bg_color = UI_COLOR_BTN_PRIMARY,
                         uint16_t icon_color = UI_COLOR_TEXT_PRIMARY,
                         int radius = UI_RADIUS_NORMAL);

// ============================================================================
// STATUS/INDICATOR WIDGETS
// ============================================================================

/**
 * @brief Draw an LED indicator
 * @param pos_x Center X position
 * @param pos_y Center Y position
 * @param is_on LED state (true = on/green, false = off/red)
 * @param radius LED radius
 */
void ui_draw_led(int pos_x, int pos_y, bool is_on, int radius = UI_LED_RADIUS);

/**
 * @brief Draw a status bar with text
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param text Status text
 * @param bg_color Background color
 */
void ui_draw_status_bar(int pos_x, int pos_y, int width, int height, const char* text,
                        uint16_t bg_color = UI_COLOR_ACCENT_DIM);

// ============================================================================
// VALUE DISPLAY WIDGETS
// ============================================================================

/**
 * @brief Draw a labeled value display box
 * @param pos_x X position
 * @param pos_y Y position
 * @param width Width
 * @param height Height
 * @param label Label text (smaller, grey)
 * @param value Value text (larger, white)
 * @param unit Unit text (optional, grey)
 * @param border_color Border color
 */
void ui_draw_value_box(int pos_x, int pos_y, int width, int height, const char* label,
                       const char* value, const char* unit = nullptr,
                       uint16_t border_color = UI_COLOR_SUCCESS);

/**
 * @brief Draw a temperature display (4-zone layout)
 * @param pos_x X position
 * @param pos_y Y position
 * @param label Label text
 * @param temp Temperature value
 */
void ui_draw_temp_display(int pos_x, int pos_y, const char* label, int temp);

// ============================================================================
// TEXT HELPERS
// ============================================================================

/**
 * @brief Draw text centered horizontally in a region
 * @param pos_x Region X
 * @param pos_y Y position
 * @param width Region width
 * @param text Text to draw
 * @param color Text color
 * @param size Text size
 */
void ui_draw_text_centered(int pos_x, int pos_y, int width, const char* text,
                           uint16_t color = UI_COLOR_TEXT_PRIMARY, int size = UI_TEXT_SIZE_NORMAL);

/**
 * @brief Draw text at position
 * @param pos_x X position
 * @param pos_y Y position
 * @param text Text to draw
 * @param color Text color
 * @param size Text size
 */
void ui_draw_text(int pos_x, int pos_y, const char* text, uint16_t color = UI_COLOR_TEXT_PRIMARY,
                  int size = UI_TEXT_SIZE_NORMAL);

// ============================================================================
// TOUCH HELPERS
// ============================================================================

/**
 * @brief Check if point is inside a rectangle
 * @param touch_x Touch X
 * @param touch_y Touch Y
 * @param pos_x Rectangle X
 * @param pos_y Rectangle Y
 * @param width Rectangle width
 * @param height Rectangle height
 * @return true if touch is inside rectangle
 */
bool ui_touch_in_rect(int touch_x, int touch_y, int pos_x, int pos_y, int width, int height);

// ============================================================================
// MODAL HELPERS
// ============================================================================

/**
 * @brief Draw a darkened backdrop for modal dialogs
 * Creates a visual dimming effect over the screen content
 * Call this before drawing a modal dialog
 */
void ui_draw_modal_backdrop();

// ============================================================================
// CLOSE BUTTON (for modal screens)
// ============================================================================

/**
 * @brief Draw a close button (red X) at the given position
 * @param pos_x X position
 * @param pos_y Y position
 * @param size Button size (square)
 * @param pressed true to show pressed state
 */
void ui_draw_close_button(int pos_x, int pos_y, int size, bool pressed = false);

/**
 * @brief Check if touch is on a close button area
 * @param touch_x Touch X coordinate
 * @param touch_y Touch Y coordinate
 * @param pos_x Button X position
 * @param pos_y Button Y position
 * @param size Button size (square)
 * @return true if touch is on close button
 */
bool ui_touch_on_close_button(int touch_x, int touch_y, int pos_x, int pos_y, int size);
