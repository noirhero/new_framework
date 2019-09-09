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

        void create(VulkanDevice* device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map = true);
        void destroy();
        void map();
        void unmap();
        void flush(VkDeviceSize size = VK_WHOLE_SIZE);
    };

    using Buffers = std::vector<Buffer>;
}
