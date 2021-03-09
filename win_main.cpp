// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "win_handle.h"

LRESULT CALLBACK WndProc(HWND winHandle, UINT message, WPARAM paramW, LPARAM paramL) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        Main::Resize(LOWORD(paramL), HIWORD(paramL));
        break;
    default:
        return DefWindowProc(winHandle, message, paramW, paramL);
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
    constexpr auto winStyle = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME;

    RECT winRect = { 0, 0, width, height };
    AdjustWindowRect(&winRect, winStyle, FALSE);
    const auto w = winRect.right - winRect.left;
    const auto h = winRect.bottom - winRect.top;

    const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const auto x = screenWidth / 2 - w / 2;
    const auto y = screenHeight / 2 - h / 2;

    HWND winHandle = CreateWindowW(className, className, winStyle, x, y, w, h, nullptr, nullptr, winInstance, nullptr);
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
