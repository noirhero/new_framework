// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "surface.h"

#include "../win_handle.h"
#include "allocator_cpu.h"
#include "physical.h"

namespace Surface {
    using namespace Renderer;

    VkSurfaceKHR g_surface = VK_NULL_HANDLE;

    VkSurfaceKHR Get() {
        return g_surface;
    }

    void Destroy() {
        if (VK_NULL_HANDLE != g_surface) {
            auto* destroySurfaceFn = PFN_vkDestroySurfaceKHR(vkGetInstanceProcAddr(Physical::Instance::Get(), "vkDestroySurfaceKHR"));
            destroySurfaceFn(Physical::Instance::Get(), g_surface, Allocator::CPU());
            g_surface = VK_NULL_HANDLE;
        }
    }

#if defined(_WINDOWS)
    bool Windows::Create() {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = Win::Instance();
        surfaceInfo.hwnd = Win::Handle();

        if (VK_SUCCESS != vkCreateWin32SurfaceKHR(Physical::Instance::Get(), &surfaceInfo, Allocator::CPU(), &g_surface)) {
            return false;
        }

        return true;
    }
#endif // _WINDOWS
}
