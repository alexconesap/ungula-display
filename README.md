# UngulaDisplay

> **High-performance embedded C++ libraries for ESP32, STM32 and other MCUs** — UI stack for the Waveshare 7-inch RGB display. Supported targets: ESP32-S3 only.

Display and UI widget library for ESP32-S3 RGB touchscreens. Uses LovyanGFX for graphics, GT911 for capacitive touch via I2C, and ESP_IOExpander for the CH422G IO expander.

> **Portability note**: This library is intended for internal Vasis Medical projects that use the same ESP32-S3 RGB touchscreen hardware (Waveshare 7" or compatible). While the display resolution and pin mapping are configurable via `GfxConfig`, the UI widgets contain hardcoded visual element positions, sizes, and color values tuned for 800x480 screens. The theme colors and layout constants would need rework for a significantly different display. This is not a general-purpose UI framework.

Hardware configuration (resolution, pins, timing, pixel clock) is passed as a struct to `gfx_init()`. Defaults match the Waveshare ESP32-S3-Touch-LCD-7 (800x480).

## Quick Start

```cpp
#include <ungula/display.h>

// Use default hardware config (Waveshare 7" 800x480)
GfxConfig hw = GfxConfig::waveshare7inch();
ExpanderInit(hw.expander_sda, hw.expander_scl);
gfx_init(hw);

// Draw something
gfx.fillScreen(UI_COLOR_BG_DARK);
ui_draw_panel(10, 80, 780, 390);
ui_draw_button(300, 200, 200, 60, "START", UI_COLOR_BTN_SUCCESS);
```

### Custom Display

If you are using a different panel, override the config:

```cpp
#include <ungula/display.h>

GfxConfig hw;
hw.width = 480;
hw.height = 640;
hw.freq_write = 16000000;  // 16 MHz pixel clock
hw.rotation = 1;
// ... override pin assignments as needed
gfx_init(hw);
```

## Examples

### Touch Handling

```cpp
#include <ungula/display.h>

void loop() {
    int32_t tx, ty;
    if (gfx_get_touch(&tx, &ty)) {
        // Button at (300, 200), size 200x60
        if (ui_touch_in_rect(tx, ty, 300, 200, 200, 60)) {
            ui_draw_button(300, 200, 200, 60, "START", UI_COLOR_BTN_PRESSED);
            handleStartPressed();
        }
    }
}
```

### Building a Screen

A typical screen with header, content panel, status indicator, and buttons:

```cpp
#include <ungula/display.h>

void drawMainScreen() {
    gfx.fillScreen(UI_COLOR_BG_DARK);

    // Header
    gfx.setTextColor(UI_COLOR_TEXT_PRIMARY);
    gfx_set_font(3);
    gfx_drawCentreString("My Application", 400, 20);

    // Content panel
    ui_draw_panel(10, 80, 780, 300);

    // Status LED (green = connected)
    ui_draw_led(30, 100, true);
    ui_draw_text(50, 95, "Connected", UI_COLOR_TEXT_SECONDARY);

    // Temperature display
    ui_draw_temp_display(30, 140, "Zone 1", 315);

    // Action buttons
    ui_draw_button(200, 340, 150, 50, "START", UI_COLOR_SUCCESS);
    ui_draw_button(450, 340, 150, 50, "STOP", UI_COLOR_DANGER);
}
```

### Numeric Input (Keypad)

Pop up a keypad for the user to enter a number:

```cpp
#include <ungula/display.h>

void onTempButtonPressed() {
    keypad_show(
        200, 100,             // position
        "Set Temperature",    // title
        350,                  // initial value
        200, 600,             // min, max
        [](int value, void*) {
            setTemperature(value);
        },
        nullptr,              // user_data
        [](void*) {
            // user cancelled — redraw the screen behind the keypad
            drawMainScreen();
        }
    );
}

// In your touch loop:
if (keypad_is_visible()) {
    keypad_handle_touch(tx, ty, touched);
    return;  // keypad consumes all touches while visible
}
```

### Text Input (Keyboard)

Full QWERTY keyboard for names, passwords, etc.:

```cpp
#include <ungula/display.h>

keyboard_show(
    50, 50,                  // position
    "Device Name",           // title
    "Node-1",                // initial text
    32,                      // max length
    [](const char* text, void*) {
        saveDeviceName(text);
    }
);
```

### WiFi Selector

Scan and connect to WiFi networks:

```cpp
#include <ungula/display.h>

wifi_selector_show(
    10, 80,
    [](const char* ssid, const char* password, void*) {
        connectToWifi(ssid, password);
    },
    [](void*) {
        startWifiScan();  // user pressed refresh
    }
);

// After scanning, feed the results:
WiFiNetworkInfo networks[10];
// ... populate from scan results
wifi_set_networks(networks, count);
```

## Image to Array Tool

The `tools/pngToArray.py` script converts a PNG image into a C byte array that you can embed directly in your firmware. It handles transparency (alpha channel) and supports two output modes.

### Monochrome (1-bit)

The default mode produces a 1-bit bitmap suitable for `drawBitmap()`. Every opaque pixel becomes a single bit. You pick the color at render time.

```bash
python3 tools/pngToArray.py my_logo.png
```

This prints a `PROGMEM` array to stdout. Paste it into your code and draw it like this:

```cpp
#include <ungula/display.h>

gfx.drawBitmap(10, 5, logo_data, LOGO_W, LOGO_H, UI_COLOR_TEXT_PRIMARY);
```

A 300x60 image takes about 2 KB — one bit per pixel.

### Color (2-bit indexed)

If your image has a few distinct colors (typical for logos with an icon and text), use `--mode color`. The script clusters the pixels into up to three color groups (dark/text, blue, purple) plus transparent, packs them at 2 bits per pixel, and generates a ready-to-use render function.

```bash
python3 tools/pngToArray.py my_logo.png --mode color
```

The output includes the array, RGB565 color defines, and a `draw_logo()` function. A 300x60 3-color logo takes about 4 KB — still very small.

### Options

| Flag | Default | What it does |
| --- | --- | --- |
| `--mode mono` | yes | 1-bit monochrome output |
| `--mode color` | | 2-bit indexed output with render function |
| `--alpha 32` | 32 | Alpha threshold — pixels at or below this are transparent |
| `--name logo` | logo | C identifier prefix (`logo_data`, `LOGO_W`, `draw_logo`, etc.) |

### Saving to a file

The array goes to stdout and diagnostic info goes to stderr, so you can redirect cleanly:

```bash
python3 tools/pngToArray.py my_logo.png --mode color --name app_logo > logo_data.h
```

## Using the logo

### Drawing a 2-bit Indexed Bitmap

For logos or icons generated with `--mode color`, use `gfx_draw_indexed_bitmap()`:

```cpp
#include <ungula/display.h>

// Logo 287x58 — 2-bit indexed, 3 colors, 4176 bytes
// Palette: 0=transparent, 1=text(white), 2=blue, 3=purple
// Generated by the tool
static const uint8_t logo_data[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, ... };

// Palette: index 0 = transparent (skipped), 1 = text, 2 = blue, 3 = purple
static const uint16_t logo_palette[] = { 0x0000, 0xFFFF, 0x49CF, 0xC1D0 };

gfx_draw_indexed_bitmap(gfx, 7, 3, logo_data, 287, 58, logo_palette, 4);
```

The bitmap data and palette defines are all generated by the tool — just wire them together.

### Requirements

The script needs Python 3 and Pillow:

```bash
pip install Pillow
```

## Display Hardware

| Parameter | Value |
| --- | --- |
| Resolution | 800 x 480 pixels |
| Color depth | RGB565 (16-bit) |
| Touch | GT911 capacitive (I2C, addr 0x14) |
| Backlight | PWM via IO expander (CH422G) |
| Bus | 16-bit RGB parallel |

### I2C Bus Layout

| Device | I2C Port | Address | Notes |
| --- | --- | --- | --- |
| CH422G expander | I2C_NUM_0 (ESP-IDF) | 0x24 | Backlight, reset, SD CS |
| GT911 touch | I2C_NUM_1 (LovyanGFX) | 0x14 | Capacitive touch |

The ESP32-S3 GPIO matrix allows both on the same physical pins (SDA=8, SCL=9).

## IO Expander

The backlight and reset pins go through a CH422G IO expander, so you need to initialize it before the display will show anything:

```cpp
#include <ungula/display.h>

ExpanderInit(8, 9);   // I2C SDA, SCL (defaults)
BLset(HIGH);          // turn on backlight
BLblink();            // visual feedback blink
```

## Theme Colors

| Constant | Use |
| --- | --- |
| `UI_COLOR_BG_DARK` | Main background |
| `UI_COLOR_BG_PANEL` | Panel/card background |
| `UI_COLOR_ACCENT` | Primary accent (cyan) |
| `UI_COLOR_SUCCESS` | Green (start, OK) |
| `UI_COLOR_DANGER` | Red (stop, error) |
| `UI_COLOR_WARNING` | Orange/yellow |
| `UI_COLOR_TEXT_PRIMARY` | White text |
| `UI_COLOR_TEXT_SECONDARY` | Grey text |

Screen dimensions (`UI_SCREEN_WIDTH`, `UI_SCREEN_HEIGHT`) default to 800x480 but can be overridden via `-D` build flags.

## Text Vertical Offset

When using multiple font sources (for example Adafruit GFX for English and U8g2 subset fonts for Spanish), the text baseline often doesn't match between fonts. A label that looks perfectly positioned in English may render 3-4 pixels lower in Spanish or Vietnamese because the font has different ascent metrics.

The library provides a global vertical offset that shifts all text rendering up or down. The offset applies automatically to every text operation that goes through the wrapper functions — you don't need to adjust individual coordinates.

### How it works

Set the offset once (typically when the user switches language):

```cpp
#include <ungula/display.h>

gfx_set_font_y_offset(-3);  // shift text 3 pixels up
```

From that point, all text wrappers apply the correction:

```cpp
#include <ungula/display.h>

gfx_setCursor(100, 200);         // actual cursor: (100, 197)
gfx.print("Configuración");      // prints at y=197

gfx_drawCentreString("Hola", 400, 100);  // actual y: 97
```

### Text wrappers

Use these instead of calling `gfx.setCursor()` or `gfx.drawCentreString()` directly:

| Wrapper | Offset applied | Use for |
| --- | --- | --- |
| `gfx_setCursor(x, y)` | Yes | Position cursor before `gfx.print()` |
| `gfx_drawCentreString(text, x, y)` | Yes | Centered text |
| `gfx_drawString(text, x, y)` | Yes | Left-aligned text |
| `gfx_print(text)` | No | Print at current cursor (already adjusted) |

The rule is: functions that accept a Y coordinate apply the offset. Functions that print at the current cursor position (like `gfx.print()`) don't — because the cursor was already adjusted by `gfx_setCursor()`.

### When to use direct calls

Graphics primitives like `gfx.fillRect()`, `gfx.drawRoundRect()`, `gfx.fillCircle()` etc. do NOT get the offset — they draw at exact pixel coordinates. This is intentional. Only text moves; boxes, lines, and shapes stay fixed.

### Integration with lib_i18n

If you use `lib_i18n` for multi-language support, each language can register a vertical offset at boot:

```cpp
#include <ungula/display.h>

i18n::addLanguage(i18n::Lang::English,    strings_en, N,  0);  // baseline
i18n::addLanguage(i18n::Lang::Spanish,    strings_es, N, -3);  // 3px up
i18n::addLanguage(i18n::Lang::Vietnamese, strings_vi, N, -5);  // 5px up
```

Then on language change:

```cpp
#include <ungula/display.h>

i18n::setLanguage(newLang);
gfx_set_font_y_offset(i18n::fontYOffset());
```

## Demo Sketches

Two example sketches are provided in the `examples/` folder:

| Example | Hardware | Description |
| --- | --- | --- |
| `demo_800x480` | Waveshare 7" 800x480 | Full library with IO expander, touch, widgets |
| `demo_320x240` | ESP32 + ILI9341 SPI | Standalone LGFX config, adapted layout |

## Dependencies

This library requires:

- [UngulaCore](https://github.com/alexconesap/ungula-core.git) (`lib/`) — string utilities and time abstraction.
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) `1.2.0` — graphics driver (RGB bus, touch, DMA).
- [ESP_IOExpander](https://github.com/esp-arduino-libs/ESP_IOExpander) `0.0.3` — CH422G IO expander for backlight and reset control. Uses the ESP-IDF native I2C driver internally (no Arduino Wire.h dependency).

LovyanGFX and ESP_IOExpander are pulled in automatically by Arduino CLI via `library.properties`.

If you are cloning repos manually, make sure both `lib/` and `lib_display/` are siblings in your project:

```text
your_project/
  lib/            ← UngulaCore
  lib_display/    ← this library
  src/
    main.ino
```

## Acknowledgements

Thanks to Claude and ChatGPT for helping on generating this documentation.

## License

MIT License — see [LICENSE](license.txt) file.

---

## Arduino CLI symlink note (rarely relevant)

This library ships a flat forwarder header at `src/ungula_display.h` that
just `#include`s `ungula/display.h`. `library.properties` `includes=` points
at the forwarder.

It only exists to work around an Arduino CLI quirk: when the library is
consumed through a symlink, the CLI sometimes fails to discover headers
nested under `src/ungula/`. The flat forwarder fixes that scan.

**Host code keeps including the real header**:

```cpp
#include <ungula/display.h>
```

PlatformIO, ESP-IDF component builds, and plain CMake setups can ignore
the forwarder.
