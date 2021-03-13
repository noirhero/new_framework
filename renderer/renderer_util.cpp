// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "renderer_util.h"

#include "allocator_cpu.h"

namespace Renderer {
    uint32_t Util::GetAPIVersion() {
#if VMA_VULKAN_VERSION == 1002000
        return VK_API_VERSION_1_2;
#elif VMA_VULKAN_VERSION == 1001000
        return VK_API_VERSION_1_1;
#elif VMA_VULKAN_VERSION == 1000000
        return VK_API_VERSION_1_0;
#else
#error Invalid VMA_VULKAN_VERSION.
        return UINT32_MAX;
#endif
    }

    bool VK_KHR_get_physical_device_properties2_enabled = false;
    bool VK_EXT_debug_utils_enabled = false;
    void Util::CheckToInstanceExtensionProperties(const VkExtensionProperties& properties) {
        const auto* name = properties.extensionName;

        if (0 == strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_0 == GetAPIVersion()) {
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
    bool VK_EXT_memory_priority_enabled = false;
    void Util::CheckToPhysicalDeviceExtensionProperties(const VkExtensionProperties& properties) {
        const auto* name = properties.extensionName;

        if (0 == strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_0 == GetAPIVersion()) {
                VK_KHR_get_memory_requirements2_enabled = true;
            }
        }
        else if (0 == strcmp(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_0 == GetAPIVersion()) {
                VK_KHR_dedicated_allocation_enabled = true;
            }
        }
        else if (0 == strcmp(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_0 == GetAPIVersion()) {
                VK_KHR_bind_memory2_enabled = true;
            }
        }
        else if (0 == strcmp(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, name))
            VK_EXT_memory_budget_enabled = true;
        else if (0 == strcmp(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME, name))
            VK_AMD_device_coherent_memory_enabled = true;
        else if (0 == strcmp(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_2 > GetAPIVersion()) {
                VK_KHR_buffer_device_address_enabled = true;
            }
        }
        else if (0 == strcmp(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, name)) {
            if (VK_API_VERSION_1_2 > GetAPIVersion()) {
                VK_EXT_buffer_device_address_enabled = true;
            }
        }
        else if (0 == strcmp(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, name)) {
            VK_EXT_memory_priority_enabled = true;
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

        VkPhysicalDeviceMemoryPriorityFeaturesEXT physicalDeviceMemoryPriorityFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT };
        if (VK_EXT_memory_priority_enabled) {
            physicalDeviceMemoryPriorityFeatures.pNext = features2.pNext;
            features2.pNext = &physicalDeviceMemoryPriorityFeatures;
        }

        vkGetPhysicalDeviceFeatures2(device, &features2);

        g_SparseBindingEnabled = 0 != features2.features.sparseBinding;

        // The extension is supported as fake with no real support for this feature? Don't use it.
        if (VK_AMD_device_coherent_memory_enabled && VK_FALSE == memoryFeatures.deviceCoherentMemory)
            VK_AMD_device_coherent_memory_enabled = false;

        if (VK_KHR_buffer_device_address_enabled || VK_EXT_buffer_device_address_enabled || VK_API_VERSION_1_2 <= GetAPIVersion())
            g_BufferDeviceAddressEnabled = VK_FALSE != bufferAddressFeatures.bufferDeviceAddress;

        if (VK_EXT_memory_priority_enabled && VK_FALSE == physicalDeviceMemoryPriorityFeatures.memoryPriority)
            VK_EXT_memory_priority_enabled = false;
    }

    uint32_t g_GraphicsQueueFamilyIndex = INVALID_IDX;
    uint32_t g_PresentQueueFamilyIndex = INVALID_IDX;
    uint32_t g_SparseBindingQueueFamilyIndex = INVALID_IDX;

    bool Util::CheckToQueueFamilyProperties(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<VkQueueFamilyProperties>& properties) {
        g_GraphicsQueueFamilyIndex = INVALID_IDX;
        g_PresentQueueFamilyIndex = INVALID_IDX;
        g_SparseBindingQueueFamilyIndex = INVALID_IDX;

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

            if (0 != g_PresentQueueFamilyIndex) {
                VkBool32 isSurfaceSupported = VK_FALSE;
                const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &isSurfaceSupported);
                if (0 <= result && VK_TRUE == isSurfaceSupported) {
                    g_PresentQueueFamilyIndex = index;
                }
            }

            if (g_SparseBindingEnabled && INVALID_IDX != g_SparseBindingQueueFamilyIndex &&
                0 != (VK_QUEUE_SPARSE_BINDING_BIT & queueFlags)) {
                g_SparseBindingQueueFamilyIndex = index;
            }

            ++index;
        }
        if (INVALID_IDX == g_GraphicsQueueFamilyIndex) {
            return false;
        }

        g_SparseBindingEnabled = g_SparseBindingEnabled && INVALID_IDX != g_SparseBindingQueueFamilyIndex;

        return true;
    }

    template<typename TMain, typename TNew>
    void ChainNextPointer(TMain* mainStruct, TNew* newStruct) {
        struct VkAnyStruct {
            VkStructureType type;
            void* next;
        };

        auto* lastStruct = reinterpret_cast<VkAnyStruct*>(mainStruct);
        while (nullptr != lastStruct->next) {
            lastStruct = reinterpret_cast<VkAnyStruct*>(lastStruct->next);
        }

        lastStruct->next = newStruct;
    }

    void Util::DecorateDeviceFeatures(
        VkPhysicalDeviceFeatures2& features,
        VkPhysicalDeviceCoherentMemoryFeaturesAMD& memoryFeatures,
        VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& addressFeatures,
        VkPhysicalDeviceMemoryPriorityFeaturesEXT& memPriorityFeatures) {
        features.features.samplerAnisotropy = VK_TRUE;
        features.features.sparseBinding = g_SparseBindingEnabled ? VK_TRUE : VK_FALSE;

        if (VK_AMD_device_coherent_memory_enabled) {
            memoryFeatures.deviceCoherentMemory = VK_TRUE;
            ChainNextPointer(&features, &memoryFeatures);
        }

        if (g_BufferDeviceAddressEnabled) {
            addressFeatures.bufferDeviceAddress = VK_TRUE;
            ChainNextPointer(&features, &addressFeatures);
        }

        if (VK_EXT_memory_priority_enabled) {
            ChainNextPointer(&features, &memPriorityFeatures);
        }
    }

    Util::ConstCharPtrs Util::GetEnableDeviceExtensionNames() {
        ConstCharPtrs names;
        names.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (VK_KHR_get_memory_requirements2_enabled)
            names.emplace_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        if (VK_KHR_dedicated_allocation_enabled)
            names.emplace_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
        if (VK_KHR_bind_memory2_enabled)
            names.emplace_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
        if (VK_EXT_memory_budget_enabled)
            names.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        if (VK_AMD_device_coherent_memory_enabled)
            names.emplace_back(VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME);
        if (VK_KHR_buffer_device_address_enabled)
            names.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        if (VK_EXT_buffer_device_address_enabled)
            names.emplace_back(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        if (VK_EXT_memory_priority_enabled)
            names.emplace_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);

        return names;
    }

    uint32_t Util::GetGPUQueueIndex() {
        return g_GraphicsQueueFamilyIndex;
    }

    uint32_t Util::GetPresentQueueIndex() {
        return g_PresentQueueFamilyIndex;
    }

    uint32_t Util::GetSparesQueueIndex() {
        return g_SparseBindingQueueFamilyIndex;
    }

    bool Util::IsValidQueue(uint32_t queueIndex) {
        return INVALID_IDX != queueIndex;
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
            VK_API_VERSION_1_1 <= GetAPIVersion() || VK_KHR_get_physical_device_properties2_enabled)) {
            info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }
#endif
        if (VK_AMD_device_coherent_memory_enabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;
        }
        if (g_BufferDeviceAddressEnabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
#if !defined(VMA_MEMORY_PRIORITY) || VMA_MEMORY_PRIORITY == 1
        if (VK_EXT_memory_priority_enabled) {
            info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
        }
#endif

#if defined(_DEBUG)
        info.pAllocationCallbacks = Allocator::CPU();
#endif
    }

    bool Util::MemoryTypeFromProperties(uint32_t& findIndex, const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirementsMask) {
        for (uint32_t i = 0; i < 32; ++i) {
            if (1 == (typeBits & 1)) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask) {
                    findIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }

        return false;
    }
}
