// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    namespace Allocator {
        bool InitializeVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
        void ReleaseVMA();

        VmaAllocator VMA();
        VmaAllocation VMADepth();
    }
}
