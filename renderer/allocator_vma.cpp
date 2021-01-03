// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#define VMA_IMPLEMENTATION
#include "../pch.h"
#include "allocator_vma.h"

#include "renderer_util.h"
#include "allocator_cpu.h"

namespace Renderer {
    VmaAllocator g_vmaAllocator = VK_NULL_HANDLE;

    bool Allocator::InitializeVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
        VmaAllocatorCreateInfo info{};
        info.instance = instance;
        info.physicalDevice = physicalDevice;
        info.device = device;
        info.vulkanApiVersion = Util::GetAPIVersion();

        Util::DecorateVMAAllocateInformation(info);

        info.pAllocationCallbacks = CPU();

        if(VK_SUCCESS != vmaCreateAllocator(&info, &g_vmaAllocator)) {
            return false;
        }

        return true;
    }

    //void Allocator::ReleaseVMA() {
    //    if (g_hAllocator != VK_NULL_HANDLE) {
    //        vmaDestroyAllocator(g_hAllocator);
    //        g_hAllocator = nullptr;
    //    }
    //}

    //VmaAllocator& Allocator::VMA() {
    //    return g_vmaAllocator;
    //}

    //VmaAllocation g_depthAllocation;
    //VmaAllocation* Allocator::VMADepth() {
    //    return &g_depthAllocation;
    //}
}
