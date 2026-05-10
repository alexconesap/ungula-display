// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_theme.h
 * @brief Centralized UI theme definitions - colors, fonts, spacing
 *
 * This module defines the visual theme for the UI. All colors are in RGB565 format.
 * Designed to be reusable across projects - just modify the values here to change
 * the entire application's appearance.
 */

#pragma once
#include <stdint.h>

namespace ungula::display
{
}
using namespace ungula::display;

// ============================================================================
// COLOR PALETTE (RGB565 format)
// ============================================================================

// Background colors
#define UI_COLOR_BG_DARK 0x0063 // Dark navy blue - main background
#define UI_COLOR_BG_PANEL 0x0064 // Slightly lighter - panel backgrounds

// Primary accent colors
#define UI_COLOR_ACCENT 0x19CB // Cyan - primary buttons, borders
#define UI_COLOR_ACCENT_DIM 0x8496 // Grey - pressed state, secondary

// Status colors
#define UI_COLOR_SUCCESS 0x1D4E // Green - connected, success, START
#define UI_COLOR_DANGER 0xF1CA // Red - disconnected, danger, STOP
#define UI_COLOR_WARNING 0xFD20 // Orange/yellow - warnings

// Text colors
#define UI_COLOR_TEXT_PRIMARY 0xFFFF // White - main text
#define UI_COLOR_TEXT_SECONDARY 0x8496 // Grey - labels, secondary text
#define UI_COLOR_TEXT_HEADER 0xEF7E // Off-white - header text

// Border colors
#define UI_COLOR_BORDER 0x19CB // Cyan - standard borders
#define UI_COLOR_BORDER_DIM 0x39E7 // Dim grey - subtle borders

// Convenience aliases for common use cases
#define UI_COLOR_BTN_PRIMARY UI_COLOR_ACCENT
#define UI_COLOR_BTN_SUCCESS UI_COLOR_SUCCESS
#define UI_COLOR_BTN_DANGER UI_COLOR_DANGER
#define UI_COLOR_BTN_PRESSED UI_COLOR_ACCENT_DIM
#define UI_COLOR_LED_ON UI_COLOR_SUCCESS
#define UI_COLOR_LED_OFF UI_COLOR_DANGER

// ============================================================================
// SPACING & SIZING
// ============================================================================

// Standard padding values
#define UI_PADDING_SMALL 5
#define UI_PADDING_NORMAL 10
#define UI_PADDING_LARGE 15

// Corner radius
#define UI_RADIUS_SMALL 5
#define UI_RADIUS_NORMAL 7
#define UI_RADIUS_LARGE 12

// Standard component sizes
#define UI_BUTTON_HEIGHT 60
#define UI_BUTTON_WIDTH_SMALL 66
#define UI_BUTTON_WIDTH_MEDIUM 100
#define UI_BUTTON_WIDTH_LARGE 170

#define UI_LED_RADIUS 8

#define UI_INPUT_HEIGHT 50
#define UI_STATUS_BAR_HEIGHT 35

// ============================================================================
// LAYOUT CONSTANTS
// ============================================================================

// Screen dimensions — override via build defines for different displays
#ifndef UI_SCREEN_WIDTH
#define UI_SCREEN_WIDTH 800
#endif
#ifndef UI_SCREEN_HEIGHT
#define UI_SCREEN_HEIGHT 480
#endif

// Header area
#define UI_HEADER_HEIGHT 70
#define UI_HEADER_Y 0

// Main content area — derived from screen size by default
#ifndef UI_CONTENT_X
#define UI_CONTENT_X 10
#endif
#ifndef UI_CONTENT_Y
#define UI_CONTENT_Y 80
#endif
#ifndef UI_CONTENT_WIDTH
#define UI_CONTENT_WIDTH (UI_SCREEN_WIDTH - 2 * UI_CONTENT_X)
#endif
#ifndef UI_CONTENT_HEIGHT
#define UI_CONTENT_HEIGHT (UI_SCREEN_HEIGHT - UI_CONTENT_Y - UI_CONTENT_X)
#endif

// ============================================================================
// TEXT SIZES (LovyanGFX text size multipliers)
// ============================================================================

#define UI_TEXT_SIZE_SMALL 1
#define UI_TEXT_SIZE_NORMAL 2
#define UI_TEXT_SIZE_LARGE 3
#define UI_TEXT_SIZE_XLARGE 4
