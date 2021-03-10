// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Path {
    void           Initialize();

    const char*    GetCurrentPathAnsi();
    const wchar_t* GetCurrentPath();

    const char*    GetResourcePathAnsi();
    const wchar_t* GetResourcePath();
}
