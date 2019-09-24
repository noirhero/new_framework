// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkCommand.h"

namespace Vk {
    class Main;
}

class RenderSceneSystem {
public:
    bool                            Initialize(const Vk::Main& main);
    void                            Release(const Vk::Main& main);

private:
    std::vector<Vk::FrameBuffer>    _frameBuffers;
    std::vector<Vk::CommandBuffer>  _commandBuffers;
};
