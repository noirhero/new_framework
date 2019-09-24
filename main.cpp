// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"

#include "FrameworkWin.h"

int32_t WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ wchar_t*, _In_ int32_t) {
    if (false == Framework::Win::Initialize("./../data/"s, instance)) {
        return 0;
    }

    Framework::Win::Prepare();
    Framework::Win::RenderLoop();
    Framework::Win::Release();

    return 0;
}
