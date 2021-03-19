// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Command {
    class Pool;
}

namespace Buffer {
    class Uniform {
    public:
        Uniform(VkDeviceSize size, VkBuffer buffer, VmaAllocation alloc, const VmaAllocationInfo& allocInfo) : _size(size), _buffer(buffer), _alloc(alloc), _allocInfo(allocInfo) {}
        ~Uniform();

        Uniform(const Uniform&) = delete;
        Uniform& operator=(const Uniform&) = delete;

        VkDescriptorBufferInfo Information() const noexcept;
        void                   Flush(std::span<int64_t>&& mappedData);

    private:
        VkDeviceSize           _size = 0;
        VkBuffer               _buffer = VK_NULL_HANDLE;
        VmaAllocation          _alloc = VK_NULL_HANDLE;
        VmaAllocationInfo      _allocInfo{};
    };
    using UniformUPtr = std::unique_ptr<Uniform>;

    UniformUPtr CreateSimpleUniformBuffer(VkDeviceSize size);
}

namespace Buffer {
    class Object {
    public:
        Object(VkBuffer handle, VmaAllocation alloc) : _handle(handle), _alloc(alloc) {}
        ~Object();

        Object(const Object&) = delete;
        Object& operator=(const Object&) = delete;

        VkBuffer      Get() const noexcept { return _handle; }

    private:
        VkBuffer      _handle = VK_NULL_HANDLE;
        VmaAllocation _alloc = VK_NULL_HANDLE;
    };
    using ObjectUPtr = std::unique_ptr<Object>;

    ObjectUPtr CreateObject(std::span<const uint8_t>&& mappedData, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, Command::Pool& cmdPool);
}
