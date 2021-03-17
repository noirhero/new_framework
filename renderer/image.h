// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Command {
    class Pool;
}

namespace Image {
    class Sampler {
    public:
        Sampler(VkSampler handle) : _handle(handle) {}
        ~Sampler();

        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;

        constexpr VkSampler Get() const noexcept { return _handle; }

    private:
        VkSampler           _handle = VK_NULL_HANDLE;
    };
    using SamplerUPtr = std::unique_ptr<Sampler>;

    SamplerUPtr CreateSimpleLinearSampler();
    SamplerUPtr CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW);
}

namespace Image {
    class Dimension2 {
    public:
        Dimension2(VkImage image, VmaAllocation alloc, VkImageView view) : _image(image), _alloc(alloc), _view(view) {}
        ~Dimension2();

        Dimension2(const Dimension2&) = delete;
        Dimension2& operator=(const Dimension2&) = delete;

        VkDescriptorImageInfo Information(VkSampler sampler) const noexcept;

    private:
        VkImage               _image = VK_NULL_HANDLE;
        VmaAllocation         _alloc = VK_NULL_HANDLE;
        VkImageView           _view = VK_NULL_HANDLE;
    };
    using Dimension2UPtr = std::unique_ptr<Dimension2>;

    Dimension2UPtr CreateSimple2D(std::string&& path, Command::Pool& cmdPool);
    Dimension2UPtr CreateFourPixel2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, Command::Pool& cmdPool);
}
