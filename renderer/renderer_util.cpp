// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "renderer_util.h"

namespace Renderer {
    constexpr auto VulkanAPIVersion = VK_API_VERSION_1_2;
    uint32_t Util::GetAPIVersion() {
        return VulkanAPIVersion;
    }

    bool Util::IsAPIVersion1Upper() {
        const auto majorVersion = VK_VERSION_MAJOR(VulkanAPIVersion);
        return 1 < majorVersion;
    }


    bool VK_KHR_get_physical_device_properties2_enabled = false;
    bool VK_EXT_debug_utils_enabled = false;
    void Util::CheckToInstanceExtensionProperties(const VkExtensionProperties& properties) {
        const auto* name = properties.extensionName;

        if (0 == strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_KHR_get_physical_device_properties2_enabled = true;
            }
        }

        if (0 == strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, name)) {
            VK_EXT_debug_utils_enabled = true;
        }
    }


    bool g_SparseBindingEnabled = false;
    bool g_BufferDeviceAddressEnabled = false;

    bool VK_KHR_get_memory_requirements2_enabled = false;
    bool VK_KHR_dedicated_allocation_enabled = false;
    bool VK_KHR_bind_memory2_enabled = false;
    bool VK_EXT_memory_budget_enabled = false;
    bool VK_AMD_device_coherent_memory_enabled = false;
    bool VK_KHR_buffer_device_address_enabled = false;
    bool VK_EXT_buffer_device_address_enabled = false;
    void Util::CheckToPhysicalDeviceExtensionProperties(const VkExtensionProperties& properties) {
        const auto* name = properties.extensionName;

        if (0 == strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_KHR_get_memory_requirements2_enabled = true;
            }
        }
        else if (0 == strcmp(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_KHR_dedicated_allocation_enabled = true;
            }
        }
        else if (0 == strcmp(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_KHR_bind_memory2_enabled = true;
            }
        }
        else if (0 == strcmp(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, name))
            VK_EXT_memory_budget_enabled = true;
        else if (0 == strcmp(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME, name))
            VK_AMD_device_coherent_memory_enabled = true;
        else if (0 == strcmp(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_KHR_buffer_device_address_enabled = true;
            }
        }
        else if (0 == strcmp(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, name)) {
            if (IsAPIVersion1Upper()) {
                VK_EXT_buffer_device_address_enabled = true;
            }
        }

        if (VK_EXT_buffer_device_address_enabled && VK_KHR_buffer_device_address_enabled)
            VK_EXT_buffer_device_address_enabled = false;
    }

    void Util::CheckToPhysicalDeviceFeatures(VkPhysicalDevice device) {
        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceCoherentMemoryFeaturesAMD memoryFeatures{};
        memoryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;
        if (VK_AMD_device_coherent_memory_enabled) {
            features2.pNext = &memoryFeatures;
        }

        VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferAddressFeatures{};
        bufferAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
        if (VK_KHR_buffer_device_address_enabled || VK_EXT_buffer_device_address_enabled || VK_API_VERSION_1_2 <= GetAPIVersion()) {
            bufferAddressFeatures.pNext = features2.pNext;
            features2.pNext = &bufferAddressFeatures;
        }

        vkGetPhysicalDeviceFeatures2(device, &features2);

        g_SparseBindingEnabled = 0 != features2.features.sparseBinding;

        // The extension is supported as fake with no real support for this feature? Don't use it.
        if (VK_AMD_device_coherent_memory_enabled && VK_FALSE == memoryFeatures.deviceCoherentMemory)
            VK_AMD_device_coherent_memory_enabled = false;

        if (VK_KHR_buffer_device_address_enabled || VK_EXT_buffer_device_address_enabled || VK_API_VERSION_1_2 <= GetAPIVersion())
            g_BufferDeviceAddressEnabled = VK_FALSE != bufferAddressFeatures.bufferDeviceAddress;
    }

    uint32_t g_GraphicsQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
    uint32_t g_PresentQueueFamilyIndex = std::numeric_limits<uint32_t>::max();
    uint32_t g_SparseBindingQueueFamilyIndex = std::numeric_limits<uint32_t>::max();

    bool Util::CheckToQueueFamilyProperties(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& properties) {
        uint32_t index = 0;
        for (const auto& property : properties) {
            if (0 == property.queueCount) {
                ++index;
                continue;
            }

            const auto queueFlags = property.queueFlags;

            constexpr uint32_t flagsForGraphicsQueue = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
            if (0 != g_GraphicsQueueFamilyIndex && flagsForGraphicsQueue == (queueFlags & flagsForGraphicsQueue)) {
                g_GraphicsQueueFamilyIndex = index;
            }

            if(0 != g_PresentQueueFamilyIndex) {
                VkBool32 isSurfaceSupported = VK_FALSE;
                const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &isSurfaceSupported);
                if (0 <= result && VK_TRUE == isSurfaceSupported) {
                    g_PresentQueueFamilyIndex = index;
                }
            }

            if (g_SparseBindingEnabled && std::numeric_limits<uint32_t>::max() != g_SparseBindingQueueFamilyIndex &&
                0 != (VK_QUEUE_SPARSE_BINDING_BIT & queueFlags)) {
                g_SparseBindingQueueFamilyIndex = index;
            }

            ++index;
        }
        if (std::numeric_limits<uint32_t>::max() == g_GraphicsQueueFamilyIndex) {
            return false;
        }

        g_SparseBindingEnabled = g_SparseBindingEnabled && std::numeric_limits<uint32_t>::max() != g_SparseBindingQueueFamilyIndex;

        return true;
    }

    void Util::DecorateVMAAllocateInformation(VmaAllocatorCreateInfo& info) {
        if (VK_KHR_dedicated_allocation_enabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }
        if (VK_KHR_bind_memory2_enabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
        }
#if !defined(VMA_MEMORY_BUDGET) || VMA_MEMORY_BUDGET == 1
        if (VK_EXT_memory_budget_enabled && (
            VK_API_VERSION_1_1 <= GetAPIVersion()|| VK_KHR_get_physical_device_properties2_enabled)) {
            info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }
#endif
        if (VK_AMD_device_coherent_memory_enabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
        }
        if (g_BufferDeviceAddressEnabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        // Uncomment to enable recording to CSV file.
        /*
        static VmaRecordSettings recordSettings = {};
        recordSettings.pFilePath = "VulkanSample.csv";
        outInfo.pRecordSettings = &recordSettings;
        */

        // Uncomment to enable HeapSizeLimit.
        /*
        static std::array<VkDeviceSize, VK_MAX_MEMORY_HEAPS> heapSizeLimit;
        std::fill(heapSizeLimit.begin(), heapSizeLimit.end(), VK_WHOLE_SIZE);
        heapSizeLimit[0] = 512ull * 1024 * 1024;
        outInfo.pHeapSizeLimit = heapSizeLimit.data();
        */
    }

    bool Util::MemoryTypeFromProperties(uint32_t& findIndex, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirementsMask) {
        // Search memory types to find first index with those properties
        for (uint32_t i = 0; i < 32; ++i) {
            if (1 == (typeBits & 1)) {
                // Type is available, does it match user properties?
                if ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask) {
                    findIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }
        // No memory types matched, return failure
        return false;
    }
}
