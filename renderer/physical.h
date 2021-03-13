// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

struct PhysicalDeviceInfo {
    VkPhysicalDevice handle = VK_NULL_HANDLE;
    uint32_t         gpuQueueIndex = INVALID_Q_IDX;
    uint32_t         presentQueueIndex = INVALID_Q_IDX;
    uint32_t         sparesQueueIndex = INVALID_Q_IDX;
    float         maxAnisotropy = 1.0f;
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
