// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_events.h
 * @brief UI event queue for decoupling screen touch actions from application logic
 *
 * Screens push events (e.g., HOME_PRESSED, PROGRAM_SELECTED) into a static
 * ring buffer. The host application polls events in its main loop and maps
 * them to domain actions (e.g., app.homeAll(), app.selectProgram()).
 *
 * This decoupling allows screens to be reused across projects without
 * changing touch handler code — only the event consumer needs to be
 * project-specific.
 *
 * No heap allocation. The buffer holds up to UI_EVENT_QUEUE_SIZE events.
 * If the buffer is full, new events are silently dropped (bounded, no blocking).
 */

#pragma once

#include <cstdint>

namespace ungula::display::ui {

        /// All possible UI events that screens can emit.
        /// Extend this enum for project-specific events if needed.
        enum class UiEventType : uint8_t {
            NONE = 0,

            // Main screen controls
            HOME_PRESSED,
            START_PRESSED,
            STOP_PRESSED,
            JOG_UP_PRESSED,
            JOG_UP_RELEASED,
            JOG_DOWN_PRESSED,
            JOG_DOWN_RELEASED,
            JOG_LEFT_PRESSED,
            JOG_LEFT_RELEASED,
            JOG_RIGHT_PRESSED,
            JOG_RIGHT_RELEASED,

            // Program management
            PROGRAM_SELECTED,         // param1 = program index
            PROGRAM_EDIT_REQUESTED,   // param1 = program index
            PROGRAM_FIELD_SAVED,      // param1 = program index
            PROGRAM_SETTINGS_CLOSED,  // param1 = edited program index

            // WiFi
            WIFI_SCAN_REQUESTED,
            WIFI_CONNECT,         // strParam = ssid, strParam2 = password
            WIFI_ENABLE_CHANGED,  // param1 = enabled (1/0)

            // System
            PAIRING_ENABLE,
            PAIRING_DISABLE,
            CALIBRATION_START,
            CALIBRATION_STOP,
            OTA_UPDATE_REQUESTED,
            REBOOT_REQUESTED,
        };

        /// A single UI event with optional parameters.
        struct UiEvent {
                UiEventType type = UiEventType::NONE;
                int32_t param1 = 0;
                int32_t param2 = 0;
                const char* strParam =
                        nullptr;  // valid until the next event is pushed that uses strParam
                const char* strParam2 = nullptr;
        };

        /// Maximum events in the queue (power of 2 for efficient modulo)
        static constexpr uint8_t UI_EVENT_QUEUE_SIZE = 16;

        /// Initialize the event queue (call once at startup)
        void ui_event_init();

        /// Push an event into the queue. Silently dropped if the queue is full.
        void ui_event_push(UiEventType type, int32_t p1 = 0, int32_t p2 = 0,
                           const char* str = nullptr, const char* str2 = nullptr);

        /// Poll the next event from the queue. Returns true if an event was available.
        bool ui_event_poll(UiEvent& out);

        /// Check if there are pending events without consuming them.
        bool ui_event_pending();

    REMOVED_LINE::display::ui
