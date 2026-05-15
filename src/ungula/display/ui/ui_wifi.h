// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_wifi.h
 * @brief Reusable WiFi network selector component
 *
 * Provides a modal WiFi configuration screen for:
 * - Scanning available networks
 * - Displaying network list with signal strength
 * - Selecting a network
 * - Entering password via keyboard
 * - Connecting to selected network
 *
 * Reusable across projects - just include and call the API functions.
 */

#pragma once
#include <cstdint>

#include "ui_theme.h"
#include "ungula/display/gfx_core.h"

using namespace ungula::display;

// Maximum networks to display
#define WIFI_MAX_NETWORKS 10

// Network info structure
struct WiFiNetworkInfo {
        char ssid[33]; // SSID (max 32 chars + null)
        int rssi; // Signal strength
        bool secure; // Has password
};

// Callback function type for when connection is requested
// Called with SSID and password when user confirms
// password will be empty string if network is open
typedef void (*wifi_connect_callback_t)(const char *ssid, const char *password, void *user_data);

// Callback function type for when scan is requested
// Implementation should call wifi_set_networks() when scan completes
typedef void (*wifi_scan_callback_t)(void *user_data);

// Callback function type for when the enable checkbox is toggled
typedef void (*wifi_enable_callback_t)(bool enabled, void *user_data);

// ============================================================================
// WIFI SELECTOR CONFIGURATION
// ============================================================================

// WiFi selector dimensions (matches other modal screens: 780x390 at position 10,80)
#define WIFI_SELECTOR_WIDTH 780
#define WIFI_SELECTOR_HEIGHT 390

// ============================================================================
// STRING HOOK (for i18n — override default English strings)
// ============================================================================

/// String IDs used by the WiFi selector component
enum WifiStringId : uint8_t {
        WIFI_STR_TITLE = 0, // "WiFi Networks"
        WIFI_STR_SCANNING, // "Scanning..."
        WIFI_STR_NO_NETWORKS, // "No networks found"
        WIFI_STR_PUSH_SCAN, // "Push refresh to scan"
        WIFI_STR_BTN_SCAN, // "SCAN"
        WIFI_STR_BTN_CONNECT, // "CONNECT"
        WIFI_STR_PASSWORD_FOR, // "Password for: %s"
        WIFI_STR_ENABLE, // "Enable Internet"
        WIFI_STR_COUNT
};

/// String provider callback type. Returns translated string for the given ID.
typedef const char *(*wifi_string_hook_t)(WifiStringId id);

/**
 * @brief Register a string provider for i18n support
 *
 * When set, all WiFi selector text uses this hook instead of the built-in
 * English defaults. Pass nullptr to revert to English.
 */
void wifi_set_string_hook(wifi_string_hook_t hook);

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Show the WiFi selector overlay
 * @param pos_x X position
 * @param pos_y Y position
 * @param connect_callback Function to call when user wants to connect
 * @param scan_callback Function to call when user wants to scan (optional)
 * @param enable_callback Function to call when enable checkbox is toggled (optional)
 * @param enabled Initial enabled state for the checkbox
 * @param user_data User data passed to callbacks
 */
void wifi_selector_show(int pos_x, int pos_y, wifi_connect_callback_t connect_callback,
                        wifi_scan_callback_t scan_callback = nullptr,
                        wifi_enable_callback_t enable_callback = nullptr, bool enabled = true,
                        void *user_data = nullptr);

/**
 * @brief Set the enabled state (shows/hides body elements).
 * When disabled: only title, close button, and checkbox visible.
 * When enabled: full UI (network list, buttons, status).
 */
void wifi_selector_set_enabled(bool enabled);

/**
 * @brief Hide the WiFi selector
 */
void wifi_selector_hide();

/**
 * @brief Check if WiFi selector is currently visible
 * @return true if selector is showing
 */
bool wifi_selector_is_visible();

/**
 * @brief Set the list of available networks
 * @param networks Array of network info
 * @param count Number of networks (max WIFI_MAX_NETWORKS)
 *
 * Call this from your scan callback when networks are found.
 * The component will copy the data internally.
 */
void wifi_set_networks(const WiFiNetworkInfo *networks, int count);

/**
 * @brief Set scanning state (shows spinner/message)
 * @param scanning true if scan is in progress
 */
void wifi_set_scanning(bool scanning);

/**
 * @brief Set connection status message
 * @param message Status message (nullptr to clear)
 * @param is_error true if this is an error message
 */
void wifi_set_status(const char *message, bool is_error = false);

/**
 * @brief Handle touch input for WiFi selector
 * @param touch_x Touch X coordinate
 * @param touch_y Touch Y coordinate
 * @param pressed true if touch is active, false if released
 * @return true if touch was handled
 */
bool wifi_selector_handle_touch(int touch_x, int touch_y, bool pressed);

/**
 * @brief Draw the WiFi selector (call after show, or to refresh)
 */
void wifi_selector_draw();

/**
 * @brief Set the SSID of the currently connected network.
 * When set, the network list highlights this SSID with a "connected" indicator.
 * Pass nullptr or empty string to clear.
 */
void wifi_set_connected_ssid(const char *ssid);

/**
 * @brief Get currently selected SSID
 * @return Pointer to selected SSID or nullptr if none selected
 */
const char *wifi_get_selected_ssid();
