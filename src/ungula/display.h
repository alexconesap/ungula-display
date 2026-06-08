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
#include <ungula/core.h>

// Display core
#include "ungula/display/gfx_bitmap.h"
#include "ungula/display/gfx_config.h"
#include "ungula/display/gfx_core.h"

// UI components (rendering + input widgets only)
#include "ungula/display/ui/ui_keyboard.h"
#include "ungula/display/ui/ui_keypad.h"
#include "ungula/display/ui/ui_macros.h"
#include "ungula/display/ui/ui_theme.h"
#include "ungula/display/ui/ui_widgets.h"
#include "ungula/display/ui/ui_wifi.h"

// Backwards-compatibility shim: lift the entire ungula::display namespace into
// the global scope so consumers that still use bare `GfxCore`, `GfxConfig`,
// `gfx`, `gfx_init`, `gfx_setCursor`, etc., keep compiling. Drop this once
// every consumer qualifies through `ungula::display::`.
using namespace ungula::display;
