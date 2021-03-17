// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Image {
    class Sampler;
    class Dimension2;
}

namespace Model::Sampler {
    bool               Initialize();
    void               Destroy();
    Image::Sampler&    Get(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW);
}

namespace Model::Texture2D {
    bool               Initialize();
    void               Destroy();
    Image::Dimension2& Get(std::string&& key, std::span<uint8_t>&& pixels, uint32_t width, uint32_t height);
}
