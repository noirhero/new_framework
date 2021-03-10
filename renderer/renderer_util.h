// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    namespace Util {
        using ConstCharPtrs = std::vector<const char*>;

        uint32_t      GetAPIVersion();

        void          CheckToInstanceExtensionProperties(const VkExtensionProperties& properties);
        void          CheckToPhysicalDeviceExtensionProperties(const VkExtensionProperties& properties);
        void          CheckToPhysicalDeviceFeatures(VkPhysicalDevice device);

        bool          CheckToQueueFamilyProperties(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& properties);
        uint32_t      GetGPUQueueIndex();
        uint32_t      GetPresentQueueIndex();
        uint32_t      GetSparesQueueIndex();
        bool          IsValidQueue(uint32_t queueIndex);

        ConstCharPtrs GetEnableDeviceExtensionNames();
        void          DecorateDeviceFeatures(
            VkPhysicalDeviceFeatures2& features, 
            VkPhysicalDeviceCoherentMemoryFeaturesAMD& memoryFeatures,
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& addressFeatures,
            VkPhysicalDeviceMemoryPriorityFeaturesEXT& memPriorityFeatures);

        void          DecorateVMAAllocateInformation(VmaAllocatorCreateInfo& info);

        bool          MemoryTypeFromProperties(uint32_t& findIndex, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirementsMask);
    }
}
