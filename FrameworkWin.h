// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Framework::Win {
    bool Initialize(std::string&& assetPath, HINSTANCE instance);
    void Release();

    void Prepare();
    void RenderLoop();
}
