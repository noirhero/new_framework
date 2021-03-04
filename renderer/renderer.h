// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    bool Initialize();
    void Release();

    void Run();
    void Resize(uint32_t width, uint32_t height);
}
