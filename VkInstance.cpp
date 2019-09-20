// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkInstance.h"

#include "VkUtils.h"

namespace Vk {
    bool Instance::Initialize(bool isValidation) {
        const VkApplicationInfo appInfo {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr,
            "ApplicationName", 0,
            "EngineName", 0,
            VK_API_VERSION_1_0
        };

        std::vector<const char*> instanceExtensions{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        if (true == isValidation) {
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &appInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        info.ppEnabledExtensionNames = instanceExtensions.data();

        if (true == isValidation) {
            info.enabledLayerCount = 1;
            const char *validationLayerNames[] = {
                "VK_LAYER_LUNARG_standard_validation"
            };
            info.ppEnabledLayerNames = validationLayerNames;
        }

        CheckResult(vkCreateInstance(&info, nullptr, &_instance));

        return (VK_NULL_HANDLE == _instance) ? false : true;
    }

    void Instance::Release() {
        if (VK_NULL_HANDLE == _instance)
            return;

        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}
