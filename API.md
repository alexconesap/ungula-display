# UngulaDisplay

LLM-oriented API reference for `lib_display`. UI stack for ESP32-S3 RGB
touchscreens (default target: Waveshare 7" 800x480 + GT911 touch). Wraps
LovyanGFX with a thin C-style facade and a set of touch-driven widgets,
modal keypad/keyboard, and a WiFi selector. The graphics driver is
LovyanGFX-only; **U8g2 is supported only as a font source via
`gfx_register_u8g2_font()`**, not as a rendering backend.

Master include: `<ungula/display.h>` (pulls every public header below).

---

## LLM quick map

- **Primary include**: `#include <ungula/display.h>`.
- **Arduino discovery include**: `#include <ungula_display.h>` (forwarder only; host code should keep using the real header).
- **Namespace root**: `ungula::display`.
- **Own source minimum**: `C++17`.
- **Effective minimum for consumers**: `C++17`.
- **Dependency impact**: Declared internal dependency `UngulaCore` is `C++17`.
- **Supported architectures**: `esp32`.
- **Read order for coding agents**: `Usage` (working patterns) -> `API` (symbols/signatures) -> `Lifecycle`/`Error handling`/`Threading` notes in this file.

### Use-case index

- [Use case: bring up the default 7" panel and draw something](#use-case-bring-up-the-default-7-panel-and-draw-something)
- [Use case: poll touch and react to a button](#use-case-poll-touch-and-react-to-a-button)
- [Use case: numeric keypad modal](#use-case-numeric-keypad-modal)
- [Use case: text keyboard modal](#use-case-text-keyboard-modal)
- [Use case: WiFi network selector](#use-case-wifi-network-selector)
- [Use case: emit and consume UI events](#use-case-emit-and-consume-ui-events)
- [Use case: draw a 1-bit logo / a 2-bit indexed logo](#use-case-draw-a-1-bit-logo-a-2-bit-indexed-logo)
- [Use case: register a U8g2 font and use it for text](#use-case-register-a-u8g2-font-and-use-it-for-text)
- [Use case: compensate font baseline drift across languages](#use-case-compensate-font-baseline-drift-across-languages)

### LLM rules

- Use only symbols and include paths documented in this file; do not infer extra public API from implementation files.
- Prefer the use-case patterns here over ad-hoc rewrites; keep dependency wiring and lifecycle order identical unless the task explicitly changes API design.
- Treat headers under `detail/`, `platform/`, and `platforms/` as internal unless this document calls them out as public.
- If required behavior is missing from the documented API, report the gap explicitly instead of inventing new public symbols.


## Usage

### Use case: bring up the default 7" panel and draw something

```cpp
#include <ungula/display.h>

void setup() {
    GfxConfig hw = GfxConfig::waveshare7inch();   // 800x480 defaults
    gfx_init(hw);                                  // configure + init LGFX

    ui_clear(UI_COLOR_BG_DARK);
    ui_draw_panel(10, 80, 780, 390);
    ui_draw_button(300, 200, 200, 60, "START", UI_COLOR_BTN_SUCCESS);
}

void loop() {}
```

When to use: any project running on a Waveshare ESP32-S3-Touch-LCD-7
(or pin-compatible) board.

Note: the README and `demo_800x480.ino` also call `ExpanderInit(...)`,
`BLset(...)`, `BLblink()`. **Those symbols are not declared by this
library** — they live in the external `ESP_IOExpander` (CH422G) library
that controls backlight and reset. If your board needs them, include and
call that library directly. `gfx_init()` does not touch the expander.

### Use case: poll touch and react to a button

```cpp
#include <ungula/display.h>

constexpr int BX = 300, BY = 200, BW = 200, BH = 60;

void loop() {
    int32_t tx = 0, ty = 0;
    if (!gfx_get_touch(&tx, &ty)) return;

    if (ui_touch_in_rect(tx, ty, BX, BY, BW, BH)) {
        ui_draw_button(BX, BY, BW, BH, "START", UI_COLOR_BTN_PRESSED);
        // ...handle press...
    }
}
```

When to use: any screen that has touchable regions. `gfx_get_touch()`
returns `false` when no finger is down.

### Use case: numeric keypad modal

```cpp
#include <ungula/display.h>

static void onValue(int value, void*) {
    // user confirmed `value`
}
static void onCancel(void*) {
    // redraw whatever was behind the keypad
}

void openKeypad() {
    keypad_show(200, 100, "Set Temperature",
                /*initial=*/350, /*min=*/200, /*max=*/600,
                onValue, /*user_data=*/nullptr, onCancel);
}

void loop() {
    int32_t tx, ty;
    bool touched = gfx_get_touch(&tx, &ty);
    if (keypad_is_visible()) {
        keypad_handle_touch(tx, ty, touched);
        return;  // keypad consumes input while visible
    }
    // ...other touch handling...
}
```

When to use: integer entry with min/max validation. Use
`password_mode=true` and `max_digits>0` for PIN entry.

### Use case: text keyboard modal

```cpp
#include <ungula/display.h>

static void onText(const char* text, void*) {
    // confirmed text
}

keyboard_show(50, 50, "Device Name", "Node-1",
              /*max_length=*/32, onText);

// In loop, mirror the keypad pattern:
if (keyboard_is_visible()) {
    keyboard_handle_touch(tx, ty, touched);
    return;
}
```

When to use: free-form text entry up to 32 chars
(`KEYBOARD_MAX_TEXT`). Set `password_mode=true` to mask input.

### Use case: WiFi network selector

```cpp
#include <ungula/display.h>

static void onConnect(const char* ssid, const char* password, void*) {
    // start connection attempt
}
static void onScan(void*) {
    // kick off WiFi.scanNetworks() asynchronously, then call
    // wifi_set_networks(...) when done
}

wifi_selector_show(10, 80, onConnect, onScan);

WiFiNetworkInfo nets[WIFI_MAX_NETWORKS];
// fill nets[0..n-1]
wifi_set_networks(nets, n);

// loop:
if (wifi_selector_is_visible()) {
    wifi_selector_handle_touch(tx, ty, touched);
    return;
}
```

When to use: in-app WiFi provisioning UI. Scanning is delegated to the
host application via the scan callback.

### Use case: draw a 1-bit logo / a 2-bit indexed logo

```cpp
#include <ungula/display.h>

extern const uint8_t logo_mono[] PROGMEM;     // from pngToArray.py --mode mono
gfx_draw_bitmap(10, 5, logo_mono, /*w=*/300, /*h=*/60, UI_COLOR_TEXT_PRIMARY);

extern const uint8_t logo_idx[] PROGMEM;      // from pngToArray.py --mode color
static const uint16_t palette[4] = { 0x0000, 0xFFFF, 0x49CF, 0xC1D0 };
gfx_draw_indexed_bitmap(7, 3, logo_idx, 287, 58, palette);
```

When to use: PROGMEM logos and icons. `gfx_draw_indexed_bitmap` treats
palette index 0 as transparent.

### Use case: register a U8g2 font and use it for text

```cpp
#include <ungula/display.h>
#include <u8g2_fonts.h>  // your U8g2 font subset (PROGMEM uint8_t array)

void setup() {
    gfx_init();
    const void* es_font = gfx_register_u8g2_font(u8g2_font_es_subset);
    gfx_setFont(es_font);
    gfx_setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx_drawCentreString("Configuración", 400, 100);
}
```

When to use: i18n with subsetted U8g2 fonts. Up to 16 custom fonts can
be registered. For built-in FreeSans/FreeMono use `gfx_builtin_font()`.

### Use case: compensate font baseline drift across languages

```cpp
gfx_set_font_y_offset(-3);          // shift text 3 px up
gfx_setCursor(100, 200);            // actual cursor lands at y=197
gfx_print("Hola");                  // no extra offset (cursor already shifted)
gfx_drawCentreString("Hola", 400, 100);  // y becomes 97
```

When to use: when switching between fonts whose ascent metrics differ
(e.g. Adafruit GFX vs U8g2). Only text wrappers apply the offset; raw
LovyanGFX shape calls (`gfx.fillRect`, etc.) draw at exact pixels.

---

## Public types

| Type | Header | Purpose |
| --- | --- | --- |
| `LGFX` | `display/gfx_core.h` | LovyanGFX subclass for ESP32-S3 RGB + GT911. Configured by `gfx_init()`; access via `extern LGFX gfx`. |
| `GfxConfig` | `display/gfx_config.h` | Plain struct of resolution, pin map, RGB timing, touch I2C, rotation. Factory: `GfxConfig::waveshare7inch()`. |
| `enum class GfxFont` | `display/gfx_core.h` | Built-in font IDs: `SANS_9PT/12PT/18PT/24PT`, `MONO_9PT/12PT/18PT/24PT`, `TINY`. |
| `gfx_font_hook_t` | `display/gfx_core.h` | `void(*)(int size)` — project-level font selector for `gfx_set_font()`. |
| `keypad_callback_t` | `ui/ui_keypad.h` | `void(*)(int value, void* user_data)`. |
| `keypad_cancel_callback_t` | `ui/ui_keypad.h` | `void(*)(void* user_data)`. |
| `keyboard_callback_t` | `ui/ui_keyboard.h` | `void(*)(const char* text, void* user_data)`. |
| `WiFiNetworkInfo` | `ui/ui_wifi.h` | `{ char ssid[33]; int rssi; bool secure; }`. |
| `wifi_connect_callback_t` | `ui/ui_wifi.h` | `void(*)(const char* ssid, const char* password, void* user_data)`. |
| `wifi_scan_callback_t` | `ui/ui_wifi.h` | `void(*)(void* user_data)`. |
| `wifi_enable_callback_t` | `ui/ui_wifi.h` | `void(*)(bool enabled, void* user_data)`. |
| `enum WifiStringId` | `ui/ui_wifi.h` | i18n string IDs (`WIFI_STR_TITLE`, `WIFI_STR_SCANNING`, ...). |
| `wifi_string_hook_t` | `ui/ui_wifi.h` | `const char*(*)(WifiStringId)` — i18n provider. |

### Theme constants (`ui/ui_theme.h`)

Colors (RGB565): `UI_COLOR_BG_DARK`, `UI_COLOR_BG_PANEL`,
`UI_COLOR_ACCENT`, `UI_COLOR_ACCENT_DIM`, `UI_COLOR_SUCCESS`,
`UI_COLOR_DANGER`, `UI_COLOR_WARNING`, `UI_COLOR_TEXT_PRIMARY`,
`UI_COLOR_TEXT_SECONDARY`, `UI_COLOR_TEXT_HEADER`, `UI_COLOR_BORDER`,
`UI_COLOR_BORDER_DIM`. Aliases: `UI_COLOR_BTN_PRIMARY/SUCCESS/DANGER/PRESSED`,
`UI_COLOR_LED_ON/OFF`.

Sizes: `UI_PADDING_SMALL/NORMAL/LARGE`,
`UI_RADIUS_SMALL/NORMAL/LARGE`, `UI_BUTTON_HEIGHT`,
`UI_BUTTON_WIDTH_SMALL/MEDIUM/LARGE`, `UI_LED_RADIUS`,
`UI_INPUT_HEIGHT`, `UI_STATUS_BAR_HEIGHT`.

Layout: `UI_SCREEN_WIDTH`, `UI_SCREEN_HEIGHT` (defaults 800/480 —
override via `-D`), `UI_HEADER_HEIGHT`, `UI_HEADER_Y`, `UI_CONTENT_X/Y`,
`UI_CONTENT_WIDTH/HEIGHT`.

Text sizes: `UI_TEXT_SIZE_SMALL=1`, `_NORMAL=2`, `_LARGE=3`, `_XLARGE=4`.

Macro: `TOUCH_IN_RECT(tx,ty,x,y,w,h)` (`ui/ui_macros.h`).

---

## Public functions

### Display init and global access

- `extern LGFX gfx;` — the one display instance. Direct access to
  LovyanGFX primitives is allowed (`gfx.fillScreen`, `gfx.fillRect`,
  `gfx.print`, etc.) but text positioning should go through the
  `gfx_*` wrappers if Y-offset is in use.
- `void gfx_init(const GfxConfig& config = GfxConfig::waveshare7inch());`
  Configures the panel/bus/touch from `config` then calls `gfx.init()`.
  Call once in `setup()` after the IO expander is up.
- `bool gfx_get_touch(int32_t* pos_x, int32_t* pos_y);`
  Returns `true` and writes coordinates if a touch is active.

### Font / text wrappers (apply Y offset)

- `void gfx_set_font_hook(gfx_font_hook_t hook);` — replace
  `gfx.setTextSize` with a project mapper. `nullptr` to revert.
- `void gfx_set_font(int size);` — calls the hook if registered, else
  `gfx.setTextSize(size)`.
- `void gfx_set_font_y_offset(int8_t offset);`
- `int8_t gfx_get_font_y_offset();`
- `void gfx_drawCentreString(const char* text, int x, int y);`
- `void gfx_drawString(const char* text, int x, int y);`
- `void gfx_setCursor(int x, int y);`
- `void gfx_print(const char* text);`
- `void gfx_printf(const char* fmt, ...);` — printf-format-checked.
- `const void* gfx_builtin_font(GfxFont);`
- `const void* gfx_register_u8g2_font(const uint8_t* font_data);` —
  static storage; max 16 fonts.

### Graphics passthroughs (no Y offset)

`gfx_fillScreen`, `gfx_setTextColor`, `gfx_setFont`, `gfx_setTextSize`,
`gfx_fillRect`, `gfx_fillRoundRect`, `gfx_drawRoundRect`,
`gfx_fillCircle`, `gfx_fillTriangle`. All operate on `gfx`.

### Bitmap drawing (`display/gfx_bitmap.h`)

- `void gfx_draw_bitmap(int x, int y, const uint8_t* bitmap,
  int w, int h, uint16_t color);` — 1-bit packed, MSB-first.
- `void gfx_draw_bitmap_scaled(int32_t x, int32_t y,
  const uint8_t* bitmap, int srcW, int srcH, int dstW, int dstH,
  uint16_t color);` — nearest-neighbor scale.
- `void gfx_draw_indexed_bitmap(int x, int y, const uint8_t* data,
  int w, int h, const uint16_t palette[4]);` — 2-bit packed; index 0 is
  transparent.

### Widgets (`ui/ui_widgets.h`)

- `ui_draw_panel(x, y, w, h, fill=UI_COLOR_BG_PANEL, border=UI_COLOR_BORDER, radius=UI_RADIUS_NORMAL)`
- `ui_draw_content_frame()`
- `ui_draw_box(x, y, w, h, border=UI_COLOR_BORDER, border_w=1)`
- `ui_clear(bg=UI_COLOR_BG_PANEL)`
- `ui_draw_button(x, y, w, h, text, bg=UI_COLOR_BTN_PRIMARY, text_color=UI_COLOR_TEXT_PRIMARY, text_size=UI_TEXT_SIZE_NORMAL, radius=UI_RADIUS_NORMAL)`
- `ui_draw_icon_button(x, y, w, h, icon, icon_w, icon_h, bg, icon_color, radius)`
- `ui_draw_led(x, y, is_on, radius=UI_LED_RADIUS)`
- `ui_draw_status_bar(x, y, w, h, text, bg=UI_COLOR_ACCENT_DIM)`
- `ui_draw_value_box(x, y, w, h, label, value, unit=nullptr, border=UI_COLOR_SUCCESS)`
- `ui_draw_temp_display(x, y, label, temp)` — note: layout is hardcoded
  for 800x480.
- `ui_draw_text_centered(x, y, w, text, color, size)`
- `ui_draw_text(x, y, text, color, size)`
- `bool ui_touch_in_rect(tx, ty, x, y, w, h)`
- `ui_draw_modal_backdrop()`
- `ui_draw_close_button(x, y, size, pressed=false)`
- `bool ui_touch_on_close_button(tx, ty, x, y, size)`

### Keypad (`ui/ui_keypad.h`)

- `keypad_show(x, y, title, initial, min, max, cb,
  user_data=nullptr, cancel_cb=nullptr, password_mode=false,
  max_digits=0, anti_guess=false)`
- `keypad_hide()`, `bool keypad_is_visible()`
- `bool keypad_handle_touch(tx, ty, pressed)` — returns `true` if it
  consumed the event.
- `keypad_draw()`, `int keypad_get_value()`,
  `const char* keypad_get_raw_digits()`
- Sizing macros: `KEYPAD_BTN_WIDTH=80`, `KEYPAD_BTN_HEIGHT=55`,
  `KEYPAD_BTN_GAP=6`, `KEYPAD_WIDTH`, `KEYPAD_HEIGHT`.

### Keyboard (`ui/ui_keyboard.h`)

- `keyboard_show(x, y, title, initial_text, max_length, cb,
  user_data=nullptr, password_mode=false)`
- `keyboard_hide()`, `bool keyboard_is_visible()`
- `bool keyboard_handle_touch(tx, ty, pressed)`
- `keyboard_draw()`, `const char* keyboard_get_text()`
- Sizing macros: `KEYBOARD_MAX_TEXT=32`, `KEYBOARD_KEY_WIDTH=50`,
  `KEYBOARD_KEY_HEIGHT=40`, `KEYBOARD_KEY_GAP=4`, `KEYBOARD_WIDTH=560`,
  `KEYBOARD_HEIGHT=335`.

### WiFi selector (`ui/ui_wifi.h`)

- `wifi_set_string_hook(hook)` — i18n.
- `wifi_selector_show(x, y, connect_cb, scan_cb=nullptr,
  enable_cb=nullptr, enabled=true, user_data=nullptr)`
- `wifi_selector_set_enabled(bool)`, `wifi_selector_hide()`,
  `bool wifi_selector_is_visible()`
- `wifi_set_networks(const WiFiNetworkInfo*, int count)` — copies
  internally; `count` capped at `WIFI_MAX_NETWORKS` (10).
- `wifi_set_scanning(bool)`, `wifi_set_status(const char*, bool is_error=false)`
- `bool wifi_selector_handle_touch(tx, ty, pressed)`,
  `wifi_selector_draw()`
- `wifi_set_connected_ssid(const char*)`,
  `const char* wifi_get_selected_ssid()`
- Layout macros: `WIFI_SELECTOR_WIDTH=780`, `WIFI_SELECTOR_HEIGHT=390`.

> The UI event queue moved to `lib_station_ui` (`ungula::station_ui`,
> `ungula/station_ui/ui_events.h`). lib_display is rendering + input widgets
> only; it does not carry the application event vocabulary.

---

## Lifecycle

1. (Optional, board-specific) Bring up the CH422G IO expander
   externally (`ESP_IOExpander` library) and turn on backlight.
2. `gfx_init(config)` once in `setup()`.
3. (Optional) `gfx_set_font_hook(...)`, `gfx_set_font_y_offset(...)`,
   `gfx_register_u8g2_font(...)`.
4. Per frame: poll `gfx_get_touch()`. If a modal is visible
   (`keypad_is_visible()`, `keyboard_is_visible()`,
   `wifi_selector_is_visible()`) route the touch to its
   `*_handle_touch()` and skip your own touch logic that frame.
5. Modals are torn down by their own Enter/Cancel/Close buttons; the
   confirm callback fires before the modal hides itself. Call your
   own redraw from the cancel callback if needed.

---

## Threading / timing / hardware notes

- Single-threaded Arduino loop assumed. The modals and global `gfx` are
  not interrupt-safe and not thread-safe.
- `gfx_get_touch()` and the modal draw paths can be slow (full-screen
  redraws on a 16-bit RGB bus). Don't call them from ISRs or RTOS
  high-priority tasks.
- LovyanGFX uses DMA on the RGB bus; no further configuration is
  exposed by this library.
- Hardware preconditions: ESP32-S3 with PSRAM (RGB framebuffer),
  GT911 wired on a separate I2C port from any other I2C peripherals
  (default `touch_i2c_port = 1`).

---

## Internals not part of the public API

- `LGFX::_bus_instance`, `_panel_instance`, `_light_instance`,
  `_touch_instance` — exposed for LovyanGFX wiring inside `configure()`;
  do not poke them from app code.
- `LGFX::configure(const GfxConfig&)` — called by `gfx_init()`. Don't
  call directly; calling order with `init()` matters.
- The internal U8g2 font registration table (capacity 16) is private;
  fonts can only be added, not removed.
- `gfx_*.cpp`, `ui_*.cpp` — implementation files. Don't include.
- README mentions `ExpanderInit`, `BLset`, `BLblink`, and
  `GfxConfig::expander_sda/expander_scl` — **these are not part of
  this library's public API and not declared in any header here**.
  They originate from the external `ESP_IOExpander` (CH422G) library.
  Treat them as out-of-scope for `lib_display`.

---

## Recommended improvements (not yet implemented — proposed)

The library is shallow in three places. None of the items below
exist; do not call them.

1. **IO-expander integration is implicit.** `gfx_init()` does not
   initialize backlight/reset, yet the README and demo assume the
   caller wires `ESP_IOExpander` manually. *Proposed*: an optional
   `expander` field on `GfxConfig` plus a thin
   `gfx_backlight(uint8_t pct)` so the caller never touches the
   expander directly.
2. **Modal touch routing is manual.** Every host loop must check
   `keypad_is_visible() / keyboard_is_visible() /
   wifi_selector_is_visible()` and call the matching
   `*_handle_touch()` in the right order. *Proposed*: a single
   `ui_dispatch_touch(tx, ty, pressed) -> bool` that returns `true`
   when a modal consumed the event, removing the priority order from
   the caller.
3. **`ui_draw_temp_display()` is hardcoded for 800x480** and uses a
   project-specific 4-zone layout. *Proposed*: drop it from the
   library or replace with a generic `ui_draw_value_box()` variant.

---

## LLM usage rules

- Use only the symbols listed above. If a header file is not in the
  table at top, it is not public.
- Prefer `gfx_*` text wrappers over `gfx.drawCentreString` / `gfx.setCursor`
  whenever font Y-offset may be in use.
- Use `ui_*` widgets instead of hand-rolling `gfx.fillRoundRect` +
  `gfx.print` sequences.
- For touch, always check the modal `*_is_visible()` flags before
  acting on the coordinates yourself.
- Do not initialize the CH422G expander or backlight from inside
  this library — it's the host's responsibility.
- Do not assume U8g2 is a rendering backend. The only U8g2 surface
  here is `gfx_register_u8g2_font()`, which feeds bytes into LovyanGFX.
- Do not invent `gfx_*` or `ui_*` helpers; if a needed function isn't
  here, surface that as a gap.
