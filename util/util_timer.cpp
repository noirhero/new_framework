// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "util_timer.h"

namespace Timer {
    std::chrono::time_point g_prevTp = std::chrono::system_clock::now();
    float                   g_delta = 0.0f;
    float                   g_oneSeconds = 0.0f;
    uint32_t                g_countFrame = 0;
    uint32_t                g_frame = 0;

    void Reset() {
        g_prevTp = std::chrono::system_clock::now();
        g_delta = 0.0f;
        g_oneSeconds = 0.0f;
        g_countFrame = 0;
        g_frame = 0;
    }

    float Update() {
        const auto now = std::chrono::system_clock::now();
        g_delta = 0.001f * std::chrono::duration_cast<std::chrono::milliseconds>(now - g_prevTp).count();
        g_prevTp = now;

        ++g_countFrame;
        g_oneSeconds += g_delta;
        if (1.0f <= g_oneSeconds) {
            g_oneSeconds = 0.0f;
            g_frame = g_countFrame;
            g_countFrame = 0;
        }

        return g_delta;
    }
}
