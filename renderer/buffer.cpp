// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "buffer.h"

#include "allocator_vma.h"
#include "descriptor.h"

namespace Buffer {
    using namespace Renderer;

    // Uniform buffer.
    Uniform::~Uniform() {
        if (VK_NULL_HANDLE != _buffer) {
            vmaDestroyBuffer(Allocator::VMA(), _buffer, _alloc);
        }
    }

    VkDescriptorBufferInfo Uniform::Information() const noexcept {
        return { _buffer, 0, _size };
    }

    UniformUPtr CreateSimpleUniformBuffer(VkDeviceSize size) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.size = size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo{};
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &buffer, &alloc, &allocationInfo)) {
            return nullptr;
        }

        return std::make_unique<Uniform>(size, buffer, alloc, allocationInfo);
    }
}
