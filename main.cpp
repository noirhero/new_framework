// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"

#include "VkMain.h"
#include "VkFramework.h"
#include "VkWin.h"

LRESULT CALLBACK WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam) {
    Vk::HandleMessage(handle, msg, wParam, lParam);

    return DefWindowProc(handle, msg, wParam, lParam);
}

int32_t APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int32_t) {
#if defined(_DEBUG)
    Vk::GetSettings().validation = true;
#endif

    if(false == Vk::Initialize("./../data/"s)) {
        return 0;
    }

    Vk::Win win;
    win.SetupWindow(Vk::GetSettings(), instance, WndProc);
    if (true == Vk::GetSettings().validation) {
        Vk::Win::CreateConsole();
    }

    Vk::Prepare(win.GetInstance(), win.GetHandle());
    Vk::RenderLoop(win.GetHandle());

    Vk::Release();

    return 0;
}
