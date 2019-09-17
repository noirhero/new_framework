// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    class PhysicalDevice {
    public:
        bool                                        Initialize(VkInstance instance);

        VkPhysicalDevice                            Get() const { return _selectDevice; }
        const VkPhysicalDeviceProperties&           GetProperties() const { return _deviceProperties; }
        const VkPhysicalDeviceFeatures&             GetFeatures() const { return _deviceFeatures; }
        const VkPhysicalDeviceMemoryProperties&     GetMemoryProperties() const { return _deviceMemoryProperties; }

    private:
        std::vector<VkPhysicalDevice>               _devices;

        uint32_t                                    _selectIdx = 0;
        VkPhysicalDevice                            _selectDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties                  _deviceProperties{};
        VkPhysicalDeviceFeatures                    _deviceFeatures{};
        VkPhysicalDeviceMemoryProperties            _deviceMemoryProperties{};
    };
}
