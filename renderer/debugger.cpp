// Copyright 2011-2020 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "debugger.h"

#include "allocator_cpu.h"

namespace Renderer {
    VkDebugReportCallbackEXT g_debugHandle = VK_NULL_HANDLE;

    VkBool32 VKAPI_PTR Report(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objectType*/, uint64_t /*object*/, size_t /*location*/, int32_t /*messageCode*/,
        const char* layerPrefix, const char* message, void* /*userData*/) {
        if(VK_DEBUG_REPORT_ERROR_BIT_EXT & flags) {
            Output::Print(fmt::format("[Vulkan] ERROR [{}] : {}\n", layerPrefix, message));
        }
        else if(VK_DEBUG_REPORT_WARNING_BIT_EXT & flags) {
            Output::Print(fmt::format("[Vulkan] WARNING [{}] : {}\n", layerPrefix, message));
        }
        else if(VK_DEBUG_REPORT_INFORMATION_BIT_EXT & flags) {
            Output::Print(fmt::format("[Vulkan] INFORMATION [{}] : {}\n", layerPrefix, message));
        }
        else if(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT & flags) {
            Output::Print(fmt::format("[Vulkan] PERFORMANCE [{}] : {}\n", layerPrefix, message));
        }
        else if(VK_DEBUG_REPORT_DEBUG_BIT_EXT & flags) {
            Output::Print(fmt::format("[Vulkan] DEBUG [{}] : {}\n", layerPrefix, message));
        }
        else {
            return VK_FALSE;
        }

        return VK_TRUE;
    }

    bool Debugger::Initialize(VkInstance instance) {
        auto* createFn = PFN_vkCreateDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
        if(nullptr == createFn) {
            return false;
        }

        VkDebugReportCallbackCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        createInfo.pfnCallback = Report;
        createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;

        if(VK_SUCCESS != createFn(instance, &createInfo, Allocator::CPU(), &g_debugHandle)) {
            return false;
        }

        return true;
    }

    void Debugger::Destroy(VkInstance instance) {
        if(VK_NULL_HANDLE == g_debugHandle) {
            return;
        }

        auto* destroyFn = PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
        if (nullptr == destroyFn) {
            return;
        }

        destroyFn(instance, g_debugHandle, Allocator::CPU());
        g_debugHandle = VK_NULL_HANDLE;
    }
}
