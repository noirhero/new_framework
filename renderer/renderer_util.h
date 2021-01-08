// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    namespace Util {
        uint32_t GetAPIVersion();
        bool     IsAPIVersion1Upper();

        void     CheckToInstanceExtensionProperties(const VkExtensionProperties& properties);
        void     CheckToPhysicalDeviceExtensionProperties(const VkExtensionProperties& properties);
        void     CheckToPhysicalDeviceFeatures(VkPhysicalDevice device);
        bool     CheckToQueueFamilyProperties(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& properties);

        uint32_t GetGPUQueueIndex();
        uint32_t GetPresentQueueIndex();

        void     DecorateVMAAllocateInformation(VmaAllocatorCreateInfo& info);

        bool     MemoryTypeFromProperties(uint32_t& findIndex, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirementsMask);
    }
}
