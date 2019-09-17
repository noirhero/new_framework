// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    struct Settings;

    class Win {
    public:
        static void CreateConsole();
        bool        SetupWindow(Settings& settings, HINSTANCE instance, WNDPROC wndProc);

        HINSTANCE   GetInstance() const { return _instance; };
        HWND        GetHandle() const { return _handle; };

    private:
        HINSTANCE   _instance = nullptr;
        HWND        _handle = nullptr;
    };
}
