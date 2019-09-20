// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkBuffer.h"

#include "VkUtils.h"
#include "VulkanDevice.h"

namespace Vk {
    void Buffer::Create(VulkanDevice* inDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map) {
        device = inDevice->logicalDevice;

        inDevice->CreateBuffer(usageFlags, memoryPropertyFlags, size, &buffer, &memory);
        descriptor = { buffer, 0, size };
        if (map) {
            CheckResult(vkMapMemory(inDevice->logicalDevice, memory, 0, size, 0, &mapped));
        }
    }

    void Buffer::Destroy() {
        if (mapped) {
            UnMap();
        }

        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    }

    void Buffer::Map() {
        CheckResult(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped));
    }

    void Buffer::UnMap() {
        if (mapped) {
            vkUnmapMemory(device, memory);
            mapped = nullptr;
        }
    }

    void Buffer::Flush(VkDeviceSize size) const {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.size = size;
        CheckResult(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
    }
}
