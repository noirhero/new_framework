// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    struct VulkanDevice;

    /*
    Vulkan buffer object
    */
    struct Buffer {
        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo descriptor{};
        int32_t count = 0;
        void* mapped = nullptr;

        void Create(VulkanDevice* inDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map = true);
        void Destroy();
        void Map();
        void UnMap();
        void Flush(VkDeviceSize size = VK_WHOLE_SIZE) const;
    };

    using Buffers = std::vector<Buffer>;
}
