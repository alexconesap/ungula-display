// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

#pragma once
#ifndef __cplusplus
#error UngulaDisplay requires a C++ compiler
#endif

// Ungula Display Library - UI and WiFi for embedded projects
// Include this header to activate the library in Arduino

// Depend on UngulaCore - must be included first so Arduino CLI
// discovers lib/ include paths before our headers reference them.
#include <ungula_core.h>

// Display core
#include "display/gfx_bitmap.h"
#include "display/gfx_config.h"
#include "display/gfx_core.h"

// UI components
#include "ui/ui_events.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_keypad.h"
#include "ui/ui_macros.h"
#include "ui/ui_theme.h"
#include "ui/ui_widgets.h"
#include "ui/ui_wifi.h"

// WiFi channel enum (from UngulaCore)
#include <wifi/wifi_channel.h>
