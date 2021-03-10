// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "logical.h"

#include "renderer_util.h"
#include "allocator_cpu.h"
#include "physical.h"

namespace Logical {
    using namespace Renderer;

    PhysicalDeviceInfo g_gpuInfo;
    VkDevice           g_device = VK_NULL_HANDLE;
    VkQueue            g_gpuQueue = VK_NULL_HANDLE;
    VkQueue            g_presentQueue = VK_NULL_HANDLE;

    VkDevice Device::Get() {
        return g_device;
    }

    bool Device::Create() {
        g_gpuInfo = Physical::Device::GetGPU();
        if (false == Util::IsValidQueue(g_gpuInfo.gpuQueueIndex) || false == Util::IsValidQueue(g_gpuInfo.presentQueueIndex)) {
            // Todo : Spare queue index.
            return false;
        }

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        constexpr float queuePriority[] = { 0.0f };
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = g_gpuInfo.gpuQueueIndex;
        queueInfo.pQueuePriorities = queuePriority;

        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;

        VkPhysicalDeviceFeatures2 deviceFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD };
        VkPhysicalDeviceBufferDeviceAddressFeaturesEXT addressFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT };
        VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT };
        Util::DecorateDeviceFeatures(deviceFeatures, coherentMemoryFeatures, addressFeatures, memoryPriorityFeatures);
        deviceInfo.pNext = &deviceFeatures;

        auto extensionNames = Util::GetEnableDeviceExtensionNames();
        deviceInfo.enabledExtensionCount = static_cast<decltype(deviceInfo.enabledExtensionCount)>(extensionNames.size());
        deviceInfo.ppEnabledExtensionNames = extensionNames.data();

        if (VK_SUCCESS != vkCreateDevice(g_gpuInfo.handle, &deviceInfo, Allocator::CPU(), &g_device)) {
            return false;
        }

        vkGetDeviceQueue(g_device, g_gpuInfo.gpuQueueIndex, 0, &g_gpuQueue);
        vkGetDeviceQueue(g_device, g_gpuInfo.presentQueueIndex, 0, &g_presentQueue);

        return true;
    }

    void Device::Destroy() {
        if (VK_NULL_HANDLE != g_device) {
            vkDestroyDevice(g_device, Allocator::CPU());
            g_device = VK_NULL_HANDLE;
        }

        g_gpuInfo = {};
    }
}
