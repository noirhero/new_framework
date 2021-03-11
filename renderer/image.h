// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Logical {
    class CommandPool;
}

namespace Image {
    class Sampler {
    public:
        Sampler(VkSampler handle) : _handle(handle) {}
        ~Sampler();

        constexpr VkSampler Get() const noexcept { return _handle; }

    private:
        VkSampler           _handle = VK_NULL_HANDLE;
    };
    using SamplerUPtr = std::unique_ptr<Sampler>;

    SamplerUPtr CreateSimpleLinearSampler();
}

namespace Image {
    class Dimension2 {
    public:
        Dimension2(VkImage image, VmaAllocation alloc, VkImageView view) : _image(image), _alloc(alloc), _view(view) {}
        ~Dimension2();

        VkDescriptorImageInfo Information(VkSampler sampler) const noexcept;

    private:
        VkImage               _image = VK_NULL_HANDLE;
        VmaAllocation         _alloc = VK_NULL_HANDLE;
        VkImageView           _view = VK_NULL_HANDLE;
    };
    using Dimension2UPtr = std::unique_ptr<Dimension2>;

    Dimension2UPtr CreateSimple2D(std::string&& path, Logical::CommandPool& cmdPool);
}