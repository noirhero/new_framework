// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Renderer {
    namespace Allocator {
        VmaAllocator VMA();

        bool         CreateVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
        void         DestroyVMA();
    }
}
