// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "logical.h"

#include "renderer_common.h"
#include "renderer_util.h"
#include "allocator_cpu.h"
#include "allocator_vma.h"
#include "physical.h"
#include "surface.h"

namespace Logical {
    using namespace Renderer;

    // Device.
    PhysicalDeviceInfo g_gpuInfo;
    VkDevice           g_device = VK_NULL_HANDLE;
    VkQueue            g_gpuQueue = VK_NULL_HANDLE;
    VkQueue            g_presentQueue = VK_NULL_HANDLE;

    VkDevice Device::Get() {
        return g_device;
    }

    bool Device::Create() {
        g_gpuInfo = Physical::Device::GetGPU();
        if (false == Util::IsValidQueue(g_gpuInfo.gpuQueueIndex) || false == Util::IsValidQueue(g_gpuInfo.presentQueueIndex)) {
            // Todo : Spare queue index process.
            return false;
        }

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        constexpr float queuePriority[] = { 0.0f };
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = g_gpuInfo.gpuQueueIndex;
        queueInfo.pQueuePriorities = queuePriority;

        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;

        VkPhysicalDeviceFeatures2 deviceFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD };
        VkPhysicalDeviceBufferDeviceAddressFeaturesEXT addressFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT };
        VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT };
        Util::DecorateDeviceFeatures(deviceFeatures, coherentMemoryFeatures, addressFeatures, memoryPriorityFeatures);
        deviceInfo.pNext = &deviceFeatures;

        auto extensionNames = Util::GetEnableDeviceExtensionNames();
        deviceInfo.enabledExtensionCount = static_cast<decltype(deviceInfo.enabledExtensionCount)>(extensionNames.size());
        deviceInfo.ppEnabledExtensionNames = extensionNames.data();

        if (VK_SUCCESS != vkCreateDevice(g_gpuInfo.handle, &deviceInfo, Allocator::CPU(), &g_device)) {
            return false;
        }

        vkGetDeviceQueue(g_device, g_gpuInfo.gpuQueueIndex, 0, &g_gpuQueue);
        vkGetDeviceQueue(g_device, g_gpuInfo.presentQueueIndex, 0, &g_presentQueue);

        if(false == Allocator::CreateVMA(Physical::Instance::Get(), g_gpuInfo.handle, g_device)) {
            return false;
        }

        return true;
    }

    void Device::Destroy() {
        Allocator::DestroyVMA();

        if (VK_NULL_HANDLE != g_device) {
            vkDestroyDevice(g_device, Allocator::CPU());
            g_device = VK_NULL_HANDLE;
        }

        g_gpuInfo = {};
    }

    // Swap chain.
    struct SwapChainInfo {
        std::vector<VkSurfaceFormatKHR> supportFormats;
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkPresentModeKHR>   presentModes;
        uint32_t                        presentQueueIndex = 0;
        uint32_t                        imageCount = 0;
        uint32_t                        width = std::numeric_limits<uint32_t>::max();
        uint32_t                        height = std::numeric_limits<uint32_t>::max();
        VkFormat                        format = VK_FORMAT_UNDEFINED;
        VkFormat                        depthFormat = VK_FORMAT_UNDEFINED;
        VkPresentModeKHR                presentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkSurfaceTransformFlagBitsKHR   preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        VkSwapchainKHR                  handle = VK_NULL_HANDLE;
        VkImages                        images;
        VkImageViews                    imageViews;

        VkImage                         depthImage = VK_NULL_HANDLE;
        VmaAllocation                   depthAlloc = VK_NULL_HANDLE;
        VkImageView                     depthImageView = VK_NULL_HANDLE;
    };
    SwapChainInfo g_swapChain;

    bool SwapChain::Create() {
        auto* instance = Physical::Instance::Get();
        auto* surface = Surface::Get();

        // Format.
        auto* getSurfaceFormatFn = PFN_vkGetPhysicalDeviceSurfaceFormatsKHR(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        uint32_t formatCount = 0;
        getSurfaceFormatFn(g_gpuInfo.handle, surface, &formatCount, nullptr);
        if (0 == formatCount) {
            return false;
        }

        auto& formats = g_swapChain.supportFormats;
        formats.resize(formatCount);
        getSurfaceFormatFn(g_gpuInfo.handle, surface, &formatCount, formats.data());

        if (1 == formatCount && VK_FORMAT_UNDEFINED == formats[0].format) {
            g_swapChain.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        else {
            g_swapChain.format = formats[0].format;
        }

        // Capabilities.
        auto* getSurfaceCapabilities = PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        getSurfaceCapabilities(g_gpuInfo.handle, surface, &g_swapChain.capabilities);

        g_swapChain.width = g_swapChain.capabilities.currentExtent.width;
        g_swapChain.height = g_swapChain.capabilities.currentExtent.height;

        g_swapChain.imageCount = g_swapChain.capabilities.minImageCount + 1;
        if (0 < g_swapChain.capabilities.maxImageCount) {
            g_swapChain.imageCount = std::min(g_swapChain.imageCount, g_swapChain.capabilities.maxImageCount);
        }

        if (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & g_swapChain.capabilities.supportedTransforms) {
            g_swapChain.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            g_swapChain.preTransform = g_swapChain.capabilities.currentTransform;
        }

        // Preset mode.
        auto* getSurfacePresentModeFn = PFN_vkGetPhysicalDeviceSurfacePresentModesKHR(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
        uint32_t presentModeCount = 0;
        getSurfacePresentModeFn(g_gpuInfo.handle, surface, &presentModeCount, nullptr);
        if (0 == presentModeCount) {
            return false;
        }

        auto& presentModes = g_swapChain.presentModes;
        presentModes.resize(presentModeCount);
        getSurfacePresentModeFn(g_gpuInfo.handle, surface, &presentModeCount, presentModes.data());

        for (const auto supportPresentMode : presentModes) {
            if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == supportPresentMode) {
                g_swapChain.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                break;
            }
            if (VK_PRESENT_MODE_MAILBOX_KHR == supportPresentMode) {
                g_swapChain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (VK_PRESENT_MODE_IMMEDIATE_KHR == supportPresentMode) {
                g_swapChain.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        // SwapChain.
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = g_swapChain.imageCount;
        swapchainInfo.imageFormat = g_swapChain.format;
        swapchainInfo.imageExtent = { g_swapChain.width, g_swapChain.height };
        swapchainInfo.preTransform = g_swapChain.preTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.presentMode = g_swapChain.presentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        const uint32_t queueFamilyIndices[] = { g_gpuInfo.gpuQueueIndex, g_gpuInfo.presentQueueIndex };
        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainInfo.queueFamilyIndexCount = 2;
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        auto* createSwapChainFn = PFN_vkCreateSwapchainKHR(vkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR"));
        if (VK_SUCCESS != createSwapChainFn(g_device, &swapchainInfo, Allocator::CPU(), &g_swapChain.handle)) {
            return false;
        }

        // SwapChain image.
        auto* getSwapChainImageFn = PFN_vkGetSwapchainImagesKHR(vkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR"));
        uint32_t imageCount = 0;
        getSwapChainImageFn(g_device, g_swapChain.handle, &imageCount, nullptr);
        if (0 == imageCount) {
            return false;
        }

        g_swapChain.images.resize(imageCount);
        getSwapChainImageFn(g_device, g_swapChain.handle, &imageCount, g_swapChain.images.data());

        // SwapChain image view.
        g_swapChain.imageViews.resize(imageCount);
        uint32_t imageViewIndex = 0;
        for (auto* image : g_swapChain.images) {
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.format = g_swapChain.format;
            imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.image = image;
            if (VK_SUCCESS != vkCreateImageView(g_device, &imageViewInfo, Allocator::CPU(), &g_swapChain.imageViews[imageViewIndex++])) {
                return false;
            }
        }

        // Depth image.
        g_swapChain.depthFormat = VK_FORMAT_D16_UNORM;

        VkImageCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthInfo.imageType = VK_IMAGE_TYPE_2D;
        depthInfo.format = g_swapChain.depthFormat;
        depthInfo.extent = { g_swapChain.width, g_swapChain.height, 1 };
        depthInfo.mipLevels = 1;
        depthInfo.arrayLayers = 1;
        depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkFormatProperties depthFormatProperties{};
        vkGetPhysicalDeviceFormatProperties(g_gpuInfo.handle, depthInfo.format, &depthFormatProperties);

        if (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & depthFormatProperties.optimalTilingFeatures) {
            depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        }
        else if (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & depthFormatProperties.linearTilingFeatures) {
            depthInfo.tiling = VK_IMAGE_TILING_LINEAR;
        }
        else {
            return false;
        }

        VmaAllocationCreateInfo depthImageAllocCreateInfo{};
        depthImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &depthInfo, &depthImageAllocCreateInfo, &g_swapChain.depthImage, &g_swapChain.depthAlloc, nullptr)) {
            return false;
        }

        // Depth image view.
        VkImageViewCreateInfo depthImageViewInfo{};
        depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthImageViewInfo.image = g_swapChain.depthImage;
        depthImageViewInfo.format = depthInfo.format;
        depthImageViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewInfo.subresourceRange.baseMipLevel = 0;
        depthImageViewInfo.subresourceRange.levelCount = 1;
        depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewInfo.subresourceRange.layerCount = 1;
        depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        if (VK_FORMAT_D16_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D24_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D32_SFLOAT_S8_UINT == depthInfo.format) {
            depthImageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        if (VK_SUCCESS != vkCreateImageView(g_device, &depthImageViewInfo, Allocator::CPU(), &g_swapChain.depthImageView)) {
            return false;
        }

        return true;
    }

    void SwapChain::Destroy() {
        // Depth.
        if (VK_NULL_HANDLE != g_swapChain.depthImageView) {
            vkDestroyImageView(g_device, g_swapChain.depthImageView, Allocator::CPU());
            g_swapChain.depthImageView = VK_NULL_HANDLE;
        }

        if (VK_NULL_HANDLE != g_swapChain.depthImage) {
            vmaDestroyImage(Allocator::VMA(), g_swapChain.depthImage, g_swapChain.depthAlloc);
            g_swapChain.depthImage = VK_NULL_HANDLE;
            g_swapChain.depthAlloc = VK_NULL_HANDLE;
        }

        // Image.
        for (auto* imageView : g_swapChain.imageViews) {
            if (VK_NULL_HANDLE != imageView) {
                vkDestroyImageView(g_device, imageView, Allocator::CPU());
            }
        }
        g_swapChain.imageViews.clear();
        g_swapChain.images.clear();

        // Handle.
        if (VK_NULL_HANDLE != g_swapChain.handle) {
            auto* destroySwapChainFn = PFN_vkDestroySwapchainKHR(vkGetInstanceProcAddr(Physical::Instance::Get(), "vkDestroySwapchainKHR"));
            destroySwapChainFn(g_device, g_swapChain.handle, Allocator::CPU());
            g_swapChain.handle = VK_NULL_HANDLE;
        }
    }

    // Command pool.
    CommandPool::~CommandPool() {
        if(VK_NULL_HANDLE != _handle) {
            vkDestroyCommandPool(g_device, _handle, Allocator::CPU());
    	}
    }

    CommandPoolUPtr AllocateGPUCommandPool() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = g_gpuInfo.gpuQueueIndex;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool handle = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateCommandPool(g_device, &info, Allocator::CPU(), &handle)) {
            return nullptr;
        }

        return std::make_unique<CommandPool>(handle);
    }
}
