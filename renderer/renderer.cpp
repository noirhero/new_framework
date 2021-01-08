// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "renderer.h"

#include "../win_handle.h"
#include "renderer_util.h"
#include "allocator_cpu.h"
#include "allocator_vma.h"
#include "debugger.h"

namespace Renderer {
    using VkLayerPropArray = std::vector<VkLayerProperties>;
    using VkExtensionPropArray = std::vector<VkExtensionProperties>;
    using VkPhysicalDevices = std::vector<VkPhysicalDevice>;
    using VkQueueFamilyPropArray = std::vector<VkQueueFamilyProperties>;
    using VkImages = std::vector<VkImage>;
    using VkImageViews = std::vector<VkImageView>;

    struct InstanceLayer {
        VkLayerProperties    property{};
        VkExtensionPropArray extensions;
    };
    using InstanceLayers = std::vector<InstanceLayer>;
    InstanceLayers g_instanceLayers;

    bool CollectingLayerProperties() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (0 == layerCount) {
            return false;
        }

        VkLayerPropArray layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        if (layers.end() == std::ranges::find_if(layers, [](const VkLayerProperties& layerProperty)->bool {
            return "VK_LAYER_KHRONOS_validation"s == layerProperty.layerName;
            })) {
            Output::Print("This hardware not support VULKAN.\n"s);
            return false;
        }

        for (const auto& layer : layers) {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr);
            if (0 == extensionCount) {
                g_instanceLayers.emplace_back(layer);
                continue;
            }

            VkExtensionPropArray extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, extensions.data());

            for(const auto& extensionProperties : extensions) {
                Util::CheckToInstanceExtensionProperties(extensionProperties);
            }

            g_instanceLayers.emplace_back(layer, extensions);
        }

        return true;
    }

    VkInstance g_instance = VK_NULL_HANDLE;

    bool CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = Util::GetAPIVersion();

        VkInstanceCreateInfo instInfo{};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> enableLayerNames = {
            "VK_LAYER_LUNARG_api_dump",
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char*> enableExtensionNames = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        };
#if defined(_DEBUG)
        for (const auto& layer : g_instanceLayers) {
            enableLayerNames.emplace_back(layer.property.layerName);

            for (const auto& extension : layer.extensions) {
                enableExtensionNames.emplace_back(extension.extensionName);
            }
        }
#endif
        instInfo.enabledLayerCount = static_cast<uint32_t>(enableLayerNames.size());
        instInfo.ppEnabledLayerNames = enableLayerNames.data();

        instInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensionNames.size());
        instInfo.ppEnabledExtensionNames = enableExtensionNames.data();

        if (VK_SUCCESS != vkCreateInstance(&instInfo, Allocator::CPU(), &g_instance)) {
            return false;
        }

        return true;
    }

    void DestroyInstance() {
        if (VK_NULL_HANDLE != g_instance) {
            vkDestroyInstance(g_instance, Allocator::CPU());
            g_instance = VK_NULL_HANDLE;
        }
    }

    VkSurfaceKHR g_surface = VK_NULL_HANDLE;

    bool CreateSurface() {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = Win::Instance();
        surfaceInfo.hwnd = Win::Handle();

        if (VK_SUCCESS != vkCreateWin32SurfaceKHR(g_instance, &surfaceInfo, Allocator::CPU(), &g_surface)) {
            return false;
        }
        return true;
    }

    void DestroySurface() {
        if(VK_NULL_HANDLE != g_surface) {
            auto* destroySurfaceFn = PFN_vkDestroySurfaceKHR(vkGetInstanceProcAddr(g_instance, "vkDestroySurfaceKHR"));
            destroySurfaceFn(g_instance, g_surface, Allocator::CPU());
            g_surface = VK_NULL_HANDLE;
        }
    }

    struct PhysicalDevice {
        VkPhysicalDevice                 device{ VK_NULL_HANDLE };
        VkPhysicalDeviceProperties       property{};
        VkPhysicalDeviceMemoryProperties memoryProperty{};
        VkQueueFamilyPropArray           queueFamilyProperties;
        InstanceLayers                   layers;
    };
    using PhysicalDevices = std::vector<PhysicalDevice>;
    PhysicalDevices g_physicalDevices;

    void CollectPhysicalDevices() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(g_instance, &deviceCount, nullptr);
        if (0 == deviceCount) {
            return;
        }
        VkPhysicalDevices devices(deviceCount);
        vkEnumeratePhysicalDevices(g_instance, &deviceCount, devices.data());

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (0 == layerCount) {
            return;
        }
        VkLayerPropArray layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (auto* device : devices) {
            g_physicalDevices.emplace_back(device);
            auto& physicalDevice = g_physicalDevices.back();

            Util::CheckToPhysicalDeviceFeatures(device);

            vkGetPhysicalDeviceProperties(device, &physicalDevice.property);
            vkGetPhysicalDeviceMemoryProperties(device, &physicalDevice.memoryProperty);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            if (0 < queueFamilyCount) {
                physicalDevice.queueFamilyProperties.resize(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, physicalDevice.queueFamilyProperties.data());
            }

            Util::CheckToQueueFamilyProperties(device, g_surface, physicalDevice.queueFamilyProperties);

            for (const auto& layer : layers) {
                uint32_t extensionCount = 0;
                vkEnumerateDeviceExtensionProperties(device, layer.layerName, &extensionCount, nullptr);
                if (0 == extensionCount) {
                    continue;
                }

                VkExtensionPropArray extensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, layer.layerName, &extensionCount, extensions.data());

                for(const auto& extensionProperties : extensions) {
                    Util::CheckToPhysicalDeviceExtensionProperties(extensionProperties);
                }

                physicalDevice.layers.emplace_back(layer, extensions);
            }
        }
    }

    struct Device {
        VkDevice         device{ VK_NULL_HANDLE };
        VkPhysicalDevice gpuDevice{ VK_NULL_HANDLE };
        uint32_t         gpuQueueIndex{ 0 };
        VkQueue          gpuQueue{ VK_NULL_HANDLE };
        VkQueue          presentQueue{ VK_NULL_HANDLE };
    };
    Device g_device;

    std::tuple<VkPhysicalDevice, uint32_t> FindGraphicsDeviceAndQueueIndex() {
        for (const auto& physicalDevice : g_physicalDevices) {
            uint32_t queueIndex = 0;
            for (const auto& queueProperty : physicalDevice.queueFamilyProperties) {
                if (VK_QUEUE_GRAPHICS_BIT & queueProperty.queueFlags) {
                    return { physicalDevice.device, queueIndex };
                }
                ++queueIndex;
            }
        }
        return {};
    }

    bool CreateDevice() {
        auto [gpuDevice, gpuQueueIndex] = FindGraphicsDeviceAndQueueIndex();

        constexpr float queuePriority[] = { 0.0f };
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = gpuQueueIndex;
        queueInfo.pQueuePriorities = queuePriority;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;

        constexpr char const* extensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        deviceInfo.enabledExtensionCount = ARRAYSIZE(extensionNames);
        deviceInfo.ppEnabledExtensionNames = extensionNames;

        VkDevice device = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDevice(gpuDevice, &deviceInfo, Allocator::CPU(), &device)) {
            return false;
        }

        VkQueue gpuQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, gpuQueueIndex, 0, &gpuQueue);

        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, Util::GetPresentQueueIndex(), 0, &presentQueue);

        g_device = { device, gpuDevice, gpuQueueIndex, gpuQueue, presentQueue };
        return true;
    }

    void DestroyDevice() {
        if (VK_NULL_HANDLE != g_device.device) {
            vkDestroyDevice(g_device.device, Allocator::CPU());
            g_device = {};
        }
    }

    VkCommandPool g_gpuCmdPool = VK_NULL_HANDLE;

    bool CreateGPUCommandPool() {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = g_device.gpuQueueIndex;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (VK_SUCCESS != vkCreateCommandPool(g_device.device, &info, Allocator::CPU(), &g_gpuCmdPool)) {
            return false;
        }

        return true;
    }

    void DestroyGPUCommandPool() {
        if (VK_NULL_HANDLE != g_gpuCmdPool) {
            vkDestroyCommandPool(g_device.device, g_gpuCmdPool, Allocator::CPU());
            g_gpuCmdPool = VK_NULL_HANDLE;
        }
    }

    VkCommandBuffer g_gpuCmdBuf = VK_NULL_HANDLE;

    bool AllocateGPUCommandBuffer() {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = g_gpuCmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;
        if (VK_SUCCESS != vkAllocateCommandBuffers(g_device.device, &info, &g_gpuCmdBuf)) {
            return false;
        }

        return true;
    }

    struct Swapchain {
        std::vector<VkSurfaceFormatKHR> supportFormats;
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkPresentModeKHR>   presentModes;
        uint32_t                        presentQueueIndex = 0;
        uint32_t                        imageCount = 0;
        uint32_t                        width = std::numeric_limits<uint32_t>::max();
        uint32_t                        height = std::numeric_limits<uint32_t>::max();
        VkFormat                        format = VK_FORMAT_UNDEFINED;
        VkPresentModeKHR                presentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkSurfaceTransformFlagBitsKHR   preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        VkSwapchainKHR                  handle = VK_NULL_HANDLE;
        VkImages                        images;
        VkImageViews                    imageViews;

        VkImage                         depthImage = VK_NULL_HANDLE;
        VmaAllocation                   depthAlloc = VK_NULL_HANDLE;
        VkImageView                     depthImageView = VK_NULL_HANDLE;
    };
    Swapchain g_swapchain;

    bool CreateSwapchain() {
        // Format.
        auto* getSurfaceFormatFn = PFN_vkGetPhysicalDeviceSurfaceFormatsKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        uint32_t formatCount = 0;
        getSurfaceFormatFn(g_device.gpuDevice, g_surface, &formatCount, nullptr);
        if (0 == formatCount) {
            return false;
        }

        auto& formats = g_swapchain.supportFormats;
        formats.resize(formatCount);
        getSurfaceFormatFn(g_device.gpuDevice, g_surface, &formatCount, formats.data());

        if (1 == formatCount && VK_FORMAT_UNDEFINED == formats[0].format) {
            g_swapchain.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        else {
            g_swapchain.format = formats[0].format;
        }

        // Capabilities.
        auto* getSurfaceCapabilities = PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        getSurfaceCapabilities(g_device.gpuDevice, g_surface, &g_swapchain.capabilities);

        if (std::numeric_limits<uint32_t>::max() == g_swapchain.capabilities.currentExtent.width) {
            RECT winRect{};
            GetWindowRect(Win::Handle(), &winRect);

            g_swapchain.width = winRect.right;
            g_swapchain.height = winRect.bottom;
        }
        else {
            g_swapchain.width = g_swapchain.capabilities.currentExtent.width;
            g_swapchain.height = g_swapchain.capabilities.currentExtent.height;
        }

        g_swapchain.imageCount = g_swapchain.capabilities.minImageCount + 1;
        if (0 < g_swapchain.capabilities.maxImageCount) {
            g_swapchain.imageCount = std::min(g_swapchain.imageCount, g_swapchain.capabilities.maxImageCount);
        }

        if (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & g_swapchain.capabilities.supportedTransforms) {
            g_swapchain.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            g_swapchain.preTransform = g_swapchain.capabilities.currentTransform;
        }

        // Preset mode.
        auto* getSurfacePresendModeFn = PFN_vkGetPhysicalDeviceSurfacePresentModesKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
        uint32_t presentModeCount = 0;
        getSurfacePresendModeFn(g_device.gpuDevice, g_surface, &presentModeCount, nullptr);
        if (0 == presentModeCount) {
            return false;
        }

        auto& presentModes = g_swapchain.presentModes;
        presentModes.resize(presentModeCount);
        getSurfacePresendModeFn(g_device.gpuDevice, g_surface, &presentModeCount, presentModes.data());

        for (const auto supportPresentMode : presentModes) {
            if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                break;
            }
            if (VK_PRESENT_MODE_MAILBOX_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (VK_PRESENT_MODE_IMMEDIATE_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        // Swapchain.
        //PFN_vkAcquireNextImageKHR(vkGetInstanceProcAddr(g_instance, "vkAcquireNextImageKHR"));
        //PFN_vkQueuePresentKHR(vkGetInstanceProcAddr(g_instance, "vkQueuePresentKHR"));

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = g_surface;
        swapchainInfo.minImageCount = g_swapchain.imageCount;
        swapchainInfo.imageFormat = g_swapchain.format;
        swapchainInfo.imageExtent = { g_swapchain.width, g_swapchain.height };
        swapchainInfo.preTransform = g_swapchain.preTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.presentMode = g_swapchain.presentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        const uint32_t queueFamilyIndices[] = { Util::GetGPUQueueIndex(), Util::GetPresentQueueIndex() };
        if(Util::GetGPUQueueIndex() != Util::GetPresentQueueIndex()) {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainInfo.queueFamilyIndexCount = 2;
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        auto* createSwapchainFn = PFN_vkCreateSwapchainKHR(vkGetInstanceProcAddr(g_instance, "vkCreateSwapchainKHR"));
        if (VK_SUCCESS != createSwapchainFn(g_device.device, &swapchainInfo, Allocator::CPU(), &g_swapchain.handle)) {
            return false;
        }

        // Swapchain image.
        auto* getSwapchainImageFn = PFN_vkGetSwapchainImagesKHR(vkGetInstanceProcAddr(g_instance, "vkGetSwapchainImagesKHR"));
        uint32_t imageCount = 0;
        getSwapchainImageFn(g_device.device, g_swapchain.handle, &imageCount, nullptr);
        if (0 == imageCount) {
            return false;
        }

        g_swapchain.images.resize(imageCount);
        getSwapchainImageFn(g_device.device, g_swapchain.handle, &imageCount, g_swapchain.images.data());

        // Swapchain image view.
        g_swapchain.imageViews.resize(imageCount);
        uint32_t imageViewIndex = 0;
        for (auto image : g_swapchain.images) {
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.format = g_swapchain.format;
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
            if (VK_SUCCESS != vkCreateImageView(g_device.device, &imageViewInfo, Allocator::CPU(), &g_swapchain.imageViews[imageViewIndex++])) {
                return false;
            }
        }

        // Depth image.
        VkImageCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthInfo.imageType = VK_IMAGE_TYPE_2D;
        depthInfo.format = VK_FORMAT_D16_UNORM;
        depthInfo.extent = { g_swapchain.width, g_swapchain.height, 1 };
        depthInfo.mipLevels = 1;
        depthInfo.arrayLayers = 1;
        depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkFormatProperties depthFormatProperties{};
        vkGetPhysicalDeviceFormatProperties(g_device.gpuDevice, depthInfo.format, &depthFormatProperties);

        if(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & depthFormatProperties.optimalTilingFeatures) {
            depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        }
        else if(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & depthFormatProperties.linearTilingFeatures) {
            depthInfo.tiling = VK_IMAGE_TILING_LINEAR;
        }
        else {
            return false;
        }

        VmaAllocationCreateInfo depthImageAllocCreateInfo{};
        depthImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if(VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &depthInfo, &depthImageAllocCreateInfo, &g_swapchain.depthImage, &g_swapchain.depthAlloc, nullptr)) {
            return false;
        }

        // Depth image view.
        VkImageViewCreateInfo depthImageViewInfo{};
        depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthImageViewInfo.image = g_swapchain.depthImage;
        depthImageViewInfo.format = depthInfo.format;
        depthImageViewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
        depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewInfo.subresourceRange.baseMipLevel = 0;
        depthImageViewInfo.subresourceRange.levelCount = 1;
        depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewInfo.subresourceRange.layerCount = 1;
        depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        if(VK_FORMAT_D16_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D24_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D32_SFLOAT_S8_UINT == depthInfo.format) {
            depthImageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        if(VK_SUCCESS != vkCreateImageView(g_device.device, &depthImageViewInfo, Allocator::CPU(), &g_swapchain.depthImageView)) {
            return false;
        }

        return true;
    }

    void DestroySwapchain() {
        // Depth.
        if (VK_NULL_HANDLE != g_swapchain.depthImageView) {
            vkDestroyImageView(g_device.device, g_swapchain.depthImageView, Allocator::CPU());
            g_swapchain.depthImageView = VK_NULL_HANDLE;
        }

        if(VK_NULL_HANDLE != g_swapchain.depthImage) {
            vmaDestroyImage(Allocator::VMA(), g_swapchain.depthImage, g_swapchain.depthAlloc);
            g_swapchain.depthImage = VK_NULL_HANDLE;
            g_swapchain.depthAlloc = VK_NULL_HANDLE;
        }

        // Image.
        for (auto imageView : g_swapchain.imageViews) {
            if (VK_NULL_HANDLE == imageView) {
                continue;
            }

            vkDestroyImageView(g_device.device, imageView, Allocator::CPU());
        }
        g_swapchain.imageViews.clear();
        g_swapchain.images.clear();

        // Handle.
        if (VK_NULL_HANDLE != g_swapchain.handle) {
            auto* destroySwapchainFn = PFN_vkDestroySwapchainKHR(vkGetInstanceProcAddr(g_instance, "vkDestroySwapchainKHR"));
            destroySwapchainFn(g_device.device, g_swapchain.handle, Allocator::CPU());
            g_swapchain.handle = VK_NULL_HANDLE;
        }
    }

    bool Initialize() {
        CollectingLayerProperties();
        if (false == CreateInstance()) {
            return false;
        }

#if defined(_DEBUG)
        Debugger::Initialize(g_instance);
#endif

        if(false == CreateSurface()) {
            return false;
        }

        CollectPhysicalDevices();
        if (false == CreateDevice()) {
            return false;
        }

        if(false == Allocator::CreateVMA(g_instance, g_device.gpuDevice, g_device.device)) {
            return false;
        }

        if (false == CreateGPUCommandPool()) {
            return false;
        }

        if (false == CreateSwapchain()) {
            return false;
        }

        AllocateGPUCommandBuffer();

        { // Command buffer
            VkCommandBufferInheritanceInfo inheritanceInfo{};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.pInheritanceInfo = &inheritanceInfo;
            vkBeginCommandBuffer(g_gpuCmdBuf, &beginInfo);
            // blah, blah, blah...
            vkEndCommandBuffer(g_gpuCmdBuf);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &g_gpuCmdBuf;
            vkQueueSubmit(g_device.gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(g_device.gpuQueue);
        }

        return true;
    }

    void Release() {
        DestroySwapchain();
        DestroyGPUCommandPool();
        Allocator::DestroyVMA();
        DestroyDevice();
        DestroySurface();

#if defined(_DEBUG)
        Debugger::Destroy(g_instance);
#endif

        DestroyInstance();
    }
}
