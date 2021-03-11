// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Descriptor {
    class Layout;
}

namespace Buffer {
    class Uniform {
    public:
        Uniform(VkDeviceSize size, VkBuffer buffer, VmaAllocation alloc, const VmaAllocationInfo& allocInfo) : _size(size), _buffer(buffer), _alloc(alloc), _allocInfo(allocInfo) {}
        ~Uniform();

        VkDescriptorBufferInfo Information() const noexcept;

    private:
        VkDeviceSize           _size = 0;
        VkBuffer               _buffer = VK_NULL_HANDLE;
        VmaAllocation          _alloc = VK_NULL_HANDLE;
        VmaAllocationInfo      _allocInfo{};
    };
    using UniformUPtr = std::unique_ptr<Uniform>;

    UniformUPtr CreateSimpleUniformBuffer(VkDeviceSize size);
}
