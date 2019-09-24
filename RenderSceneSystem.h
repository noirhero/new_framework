// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkMain.h"

class RenderSceneSystem {
public:
    bool                            Initialize(HINSTANCE instance, HWND handle);
    void                            Release();

private:
    Vk::Main                        _main;
    std::vector<Vk::FrameBuffer>    _frameBuffers;
    std::vector<Vk::CommandBuffer>  _commandBuffers;
};
