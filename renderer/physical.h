// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

struct PhysicalDeviceInfo {
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    uint32_t         gpuQueueIndex = std::numeric_limits<uint32_t>::max();
    uint32_t         presentQueueIndex = std::numeric_limits<uint32_t>::max();
    uint32_t         sparesQueueIndex = std::numeric_limits<uint32_t>::max();
};

namespace Physical::Instance {
    VkInstance Get();

    bool       Create();
    void       Destroy();
}

namespace Physical::Device {
    PhysicalDeviceInfo GetGPU();

    void               Collect(VkSurfaceKHR surface);
}
