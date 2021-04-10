// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

#include "model.h"

namespace Command {
    class Pool;
}

namespace Image {
    class Sampler;
    class Dimension2;
}

namespace Buffer {
    class Object;
}

namespace Data::Sampler {
    bool                   Initialize();
    void                   Destroy();
    Image::Sampler& Get(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW);
}

namespace Data::Texture2D {
    bool                   Initialize(Command::Pool& cmdPool);
    void                   Destroy();
    Image::Dimension2& Get(const std::vector<unsigned char>& pixels, uint32_t width, uint32_t height, Command::Pool& cmdPool);
    Image::Dimension2& Get(std::string&& fileName, Command::Pool& cmdPool);
}
