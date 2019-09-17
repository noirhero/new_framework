// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkDebug.h"

#include "VkUtils.h"

namespace Vk {
    VKAPI_ATTR VkBool32 VKAPI_CALL MsgCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objType*/,
                                               uint64_t /*srcObject*/, size_t /*location*/, int32_t msgCode,
                                               const char * layerPrefix, const char * msg, void * /*userData*/) {
        std::string prefix;
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            prefix += "ERROR:";
        };
        if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
            prefix += "WARNING:";
        };
        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            prefix += "DEBUG:";
        }
        std::stringstream debugMessage;
        debugMessage << prefix << " [" << layerPrefix << "] Code " << msgCode << " : " << msg << "\n";
        std::cout << debugMessage.str() << "\n";

        fflush(stdout);
        return VK_FALSE;
    }

    bool Debug::Initialize(VkInstance instance) {
        const auto createFn = PFN_vkCreateDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
        if (nullptr == createFn)
            return false;

        const VkDebugReportCallbackCreateInfoEXT info {
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, nullptr,
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
            MsgCallback, nullptr
        };
        VK_CHECK_RESULT(createFn(instance, &info, nullptr, &_handle));

        return (VK_NULL_HANDLE == _handle) ? false : true;
    }

    void Debug::Release(VkInstance instance) {
        if (VK_NULL_HANDLE == _handle)
            return;

        const auto destroyFn = PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
        if (nullptr == destroyFn)
            return;

        destroyFn(instance, _handle, nullptr);
        _handle = VK_NULL_HANDLE;
    }
}
