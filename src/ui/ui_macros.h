// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_macros.h
 * @brief UI macros for display touch regions control
 */

#pragma once
#include <stdint.h>

// ============================================================================
// HELPER MACROS for touch region hit testing
// ============================================================================

// Check if touch (tx, ty) is within a given rectangular region (x, y, w, h)
#define TOUCH_IN_RECT(tx, ty, x, y, w, h) \
    ((tx) >= (x) && (tx) <= ((x) + (w)) && (ty) >= (y) && (ty) <= ((y) + (h)))

// Sample usage:
// Check if touch is within a control button by index (0=Home, 1=Up, 2=Down, 3=Left, 4=Right) that
// are one beside the other. #define TOUCH_IN_CTRL_BTN(tx, ty, idx)
//    TOUCH_IN_RECT(tx, ty, CtrlBtn::X + (CtrlBtn::W + CtrlBtn::GAP) * (idx), CtrlBtn::Y,
//    CtrlBtn::W, CtrlBtn::H)
