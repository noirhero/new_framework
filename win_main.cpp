// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "win_handle.h"

LRESULT CALLBACK WndProc(HWND winHandle, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        Main::Resize(LOWORD(lParam), HIWORD(lParam));
        break;
    default:
        return DefWindowProc(winHandle, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE winInstance, HINSTANCE /*prevInstance*/, LPWSTR /*cmdLine*/, int cmdShow) {
    constexpr wchar_t className[] = L"Framework";

    WNDCLASSEXW winProperties{};
    winProperties.cbSize = sizeof(WNDCLASSEX);
    winProperties.style = CS_HREDRAW | CS_VREDRAW;
    winProperties.lpfnWndProc = WndProc;
    winProperties.hInstance = winInstance;
    winProperties.hCursor = LoadCursor(nullptr, IDC_ARROW);
    winProperties.lpszClassName = className;
    if (0 == RegisterClassExW(&winProperties)) {
        return 0;
    }

    constexpr auto width = 640;
    constexpr auto height = 480;
    HWND winHandle = CreateWindowW(className, className, WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME, 100, 100, width, height, nullptr, nullptr, winInstance, nullptr);
    if (nullptr == winHandle) {
        return 0;
    }

    ShowWindow(winHandle, cmdShow);
    UpdateWindow(winHandle);
    Win::Initialize(winInstance, winHandle);

    Path::Initialize();

    if (false == Main::Initialize()) {
        Main::Finalize();
        return 0;
    }

    MSG msg{};
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (false == Main::Run(Timer::Update())) {
                msg.message = WM_QUIT;
            }
        }
    }
    Main::Finalize();

    return static_cast<int>(msg.wParam);
}
