// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "buffer.h"

#include "renderer_common.h"
#include "allocator_vma.h"
#include "logical.h"

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

    void Uniform::Flush(std::span<int64_t>&& mappedData) {
        const auto size = mappedData.size();

        VK_CHECK(vmaInvalidateAllocation(Allocator::VMA(), _alloc, 0, size));
        memcpy_s(_allocInfo.pMappedData, size, mappedData.data(), size);
        VK_CHECK(vmaFlushAllocation(Allocator::VMA(), _alloc, 0, size));
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

    // Object.
    Object::~Object() {
        if (VK_NULL_HANDLE != _handle) {
            vmaDestroyBuffer(Allocator::VMA(), _handle, _alloc);
        }
    }

    ObjectUPtr CreateObject(std::span<int64_t>&& mappedData, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, Logical::CommandPool& cmdPool) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.size = static_cast<decltype(bufferInfo.size)>(mappedData.size());
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        struct StagingBuffer {
            VkBuffer          buffer = VK_NULL_HANDLE;
            VmaAllocation     alloc = VK_NULL_HANDLE;
            VmaAllocationInfo allocInfo{};

            ~StagingBuffer() {
                if (VK_NULL_HANDLE != buffer) {
                    vmaDestroyBuffer(Allocator::VMA(), buffer, alloc);
                }
            }
        };
        StagingBuffer staging;
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &staging.buffer, &staging.alloc, &staging.allocInfo)) {
            return nullptr;
        }
        memcpy_s(staging.allocInfo.pMappedData, bufferInfo.size, mappedData.data(), bufferInfo.size);

        bufferInfo.usage = bufferUsage;
        allocInfo.usage = memoryUsage;
        allocInfo.flags = 0;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &buffer, &alloc, nullptr)) {
            return nullptr;
        }

        const VkBufferCopy copyRegion = { 0, 0, bufferInfo.size };

        auto* cmdBuf = cmdPool.ImmediatelyBegin();
        vkCmdCopyBuffer(cmdBuf, staging.buffer, buffer, 1, &copyRegion);
        cmdPool.ImmediatelyEndAndSubmit();

        return std::make_unique<Object>(buffer, alloc);
    }
}
