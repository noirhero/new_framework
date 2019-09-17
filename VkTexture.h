// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    struct VulkanDevice;

    class Texture {
    public:
        VulkanDevice *device = nullptr;

        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 0;
        uint32_t layerCount = 0;

        VkDescriptorImageInfo descriptor{};
        VkSampler sampler = VK_NULL_HANDLE;

        void UpdateDescriptor();
        void Destroy();
    };

    class Texture2D : public Texture {
    public:
        void loadFromFile(
            std::string filename,
            VkFormat format,
            VulkanDevice *inDevice,
            VkQueue copyQueue,
            VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout inImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void loadFromBuffer(
            void* buffer,
            VkDeviceSize bufferSize,
            VkFormat format,
            uint32_t inWidth,
            uint32_t inHeight,
            VulkanDevice *inDevice,
            VkQueue copyQueue,
            VkFilter filter = VK_FILTER_LINEAR,
            VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout inImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

    class TextureCubeMap : public Texture {
    public:
        void loadFromFile(
            std::string filename,
            VkFormat format,
            VulkanDevice *inDevice,
            VkQueue copyQueue,
            VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageLayout inImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };
}
