// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Alex Conesa
// See LICENSE file for details.

/**
 * @file ui_events.cpp
 * @brief Static ring buffer implementation for the UI event queue.
 */

#include "ui_events.h"

#include <cstring>

namespace ungula {
    namespace ui {

        // Static ring buffer — no heap, fixed size, single-producer single-consumer safe
        static UiEvent s_queue[UI_EVENT_QUEUE_SIZE];
        static uint8_t s_head = 0;  // write position
        static uint8_t s_tail = 0;  // read position

        // Static buffers for string parameters (copied on push so the caller's memory can be
        // reused)
        static constexpr uint8_t STR_BUF_SIZE = 65;
        static char s_strBuf1[STR_BUF_SIZE];
        static char s_strBuf2[STR_BUF_SIZE];

        void ui_event_init() {
            s_head = 0;
            s_tail = 0;
            for (uint8_t i = 0; i < UI_EVENT_QUEUE_SIZE; ++i) {
                s_queue[i].type = UiEventType::NONE;
            }
        }

        void ui_event_push(UiEventType type, int32_t p1, int32_t p2, const char* str,
                           const char* str2) {
            uint8_t next = (s_head + 1) & (UI_EVENT_QUEUE_SIZE - 1);
            if (next == s_tail) {
                return;  // queue full, drop the event
            }

            UiEvent& ev = s_queue[s_head];
            ev.type = type;
            ev.param1 = p1;
            ev.param2 = p2;

            // Copy string parameters into static buffers
            if (str != nullptr) {
                std::strncpy(s_strBuf1, str, STR_BUF_SIZE - 1);
                s_strBuf1[STR_BUF_SIZE - 1] = '\0';
                ev.strParam = s_strBuf1;
            } else {
                ev.strParam = nullptr;
            }

            if (str2 != nullptr) {
                std::strncpy(s_strBuf2, str2, STR_BUF_SIZE - 1);
                s_strBuf2[STR_BUF_SIZE - 1] = '\0';
                ev.strParam2 = s_strBuf2;
            } else {
                ev.strParam2 = nullptr;
            }

            s_head = next;
        }

        bool ui_event_poll(UiEvent& out) {
            if (s_tail == s_head) {
                return false;  // empty
            }

            out = s_queue[s_tail];
            s_queue[s_tail].type = UiEventType::NONE;
            s_tail = (s_tail + 1) & (UI_EVENT_QUEUE_SIZE - 1);
            return true;
        }

        bool ui_event_pending() {
            return s_tail != s_head;
        }

    }  // namespace ui
}  // namespace ungula
