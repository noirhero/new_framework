// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    namespace Util {
        using ConstCharPtrs = std::vector<const char*>;

        uint32_t      GetAPIVersion();
        bool          IsAPIVersion1Upper();

        void          CheckToInstanceExtensionProperties(const VkExtensionProperties& properties);
        void          CheckToPhysicalDeviceExtensionProperties(const VkExtensionProperties& properties);
        void          CheckToPhysicalDeviceFeatures(VkPhysicalDevice device);
        bool          CheckToQueueFamilyProperties(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& properties);

        void          DecorateDeviceFeatures(
            VkPhysicalDeviceFeatures2& features, 
            VkPhysicalDeviceCoherentMemoryFeaturesAMD& memoryFeatures,
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& addressFeatures,
            VkPhysicalDeviceMemoryPriorityFeaturesEXT& memPriorityFeatures);
        ConstCharPtrs GetEnableDeviceExtensionNames();

        uint32_t      GetGPUQueueIndex();
        uint32_t      GetPresentQueueIndex();

        void          DecorateVMAAllocateInformation(VmaAllocatorCreateInfo& info);

        bool          MemoryTypeFromProperties(uint32_t& findIndex, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirementsMask);
    }
}
