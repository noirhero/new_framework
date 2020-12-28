// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "win_handle.h"

namespace Win {
    HINSTANCE g_winInstance = nullptr;
    HWND      g_winHandle = nullptr;

    void Initialize(HINSTANCE winInstance, HWND winHandle) {
        g_winInstance = winInstance;
        g_winHandle = winHandle;
    }

    HINSTANCE Instance() {
        return g_winInstance;
    }

    HWND Handle() {
        return g_winHandle;
    }
}
