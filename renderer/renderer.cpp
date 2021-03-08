// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "renderer.h"

#include "../win_handle.h"
#include "renderer_common.h"
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

        if (layers.end() == std::find_if(layers.begin(), layers.end(), [](const VkLayerProperties& layerProperty)->bool {
            return "VK_LAYER_KHRONOS_validation"s == layerProperty.layerName;
            })) {
            Output::Print("This hardware not support VULKAN.\n"s);
            return false;
        }

        for (const auto& layer : layers) {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr);
            if (0 == extensionCount) {
                g_instanceLayers.emplace_back(InstanceLayer{ layer });
                continue;
            }

            VkExtensionPropArray extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, extensions.data());

            for (const auto& extensionProperties : extensions) {
                Util::CheckToInstanceExtensionProperties(extensionProperties);
            }

            g_instanceLayers.emplace_back(InstanceLayer{ layer, extensions });
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
        if (VK_NULL_HANDLE != g_surface) {
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
            uint32_t extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            if (0 == extensionCount) {
                continue;
            }

            VkExtensionPropArray extensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

            for (const auto& extensionProperties : extensions) {
                Util::CheckToPhysicalDeviceExtensionProperties(extensionProperties);
            }

            g_physicalDevices.emplace_back(PhysicalDevice{ device });
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

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        constexpr float queuePriority[] = { 0.0f };
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = gpuQueueIndex;
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

    struct Swapchain {
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
        if (Util::GetGPUQueueIndex() != Util::GetPresentQueueIndex()) {
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
        g_swapchain.depthFormat = VK_FORMAT_D16_UNORM;

        VkImageCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthInfo.imageType = VK_IMAGE_TYPE_2D;
        depthInfo.format = g_swapchain.depthFormat;
        depthInfo.extent = { g_swapchain.width, g_swapchain.height, 1 };
        depthInfo.mipLevels = 1;
        depthInfo.arrayLayers = 1;
        depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkFormatProperties depthFormatProperties{};
        vkGetPhysicalDeviceFormatProperties(g_device.gpuDevice, depthInfo.format, &depthFormatProperties);

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
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &depthInfo, &depthImageAllocCreateInfo, &g_swapchain.depthImage, &g_swapchain.depthAlloc, nullptr)) {
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

        if (VK_FORMAT_D16_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D24_UNORM_S8_UINT == depthInfo.format || VK_FORMAT_D32_SFLOAT_S8_UINT == depthInfo.format) {
            depthImageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        if (VK_SUCCESS != vkCreateImageView(g_device.device, &depthImageViewInfo, Allocator::CPU(), &g_swapchain.depthImageView)) {
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

        if (VK_NULL_HANDLE != g_swapchain.depthImage) {
            vmaDestroyImage(Allocator::VMA(), g_swapchain.depthImage, g_swapchain.depthAlloc);
            g_swapchain.depthImage = VK_NULL_HANDLE;
            g_swapchain.depthAlloc = VK_NULL_HANDLE;
        }

        // Image.
        for (auto* imageView : g_swapchain.imageViews) {
            if (VK_NULL_HANDLE != imageView) {
                vkDestroyImageView(g_device.device, imageView, Allocator::CPU());
            }
        }
        g_swapchain.imageViews.clear();
        g_swapchain.images.clear();

        // Handle.
        if (VK_NULL_HANDLE != g_swapchain.handle) {
            auto* destroySwapchainFn = PFN_vkDestroySwapchainKHR(vkGetInstanceProcAddr(g_instance, "vkDestroySwapchainKHR"));
            destroySwapchainFn(g_device.device, g_swapchain.handle, Allocator::CPU());
            g_swapchain.handle = VK_NULL_HANDLE;
        }

        g_swapchain = {};
    }

    bool ImmediateBufferCopy(VkBuffer dest, const VkBuffer src, VkBufferCopy&& copyRegion) {
        VkCommandBufferAllocateInfo cmdBufInfo{};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = g_gpuCmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        VkCommandBuffer immediatelyCmdBuf = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkAllocateCommandBuffers(g_device.device, &cmdBufInfo, &immediatelyCmdBuf)) {
            return false;
        }

        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (VK_SUCCESS != vkBeginCommandBuffer(immediatelyCmdBuf, &cmdBufBeginInfo)) {
            return false;
        }

        vkCmdCopyBuffer(immediatelyCmdBuf, src, dest, 1, &copyRegion);

        vkEndCommandBuffer(immediatelyCmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &immediatelyCmdBuf;
        vkQueueSubmit(g_device.gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(g_device.gpuQueue);

        vkFreeCommandBuffers(g_device.device, g_gpuCmdPool, 1, &immediatelyCmdBuf);

        return true;
    }

    struct VertexBuffer {
        VkBuffer      buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };
    VertexBuffer g_vb;

    bool CreateVertexBuffer() {
        struct VertexWithColor {
            float x, y, z, w;
            float r, g, b, a;
        };
        constexpr VertexWithColor vertices[] = {
            //{ -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0 },
            //{  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0 },
            //{  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0 },
            //{ -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0 },

            {  1.0f, -1.0f, -1.0f, 1.0f, 0.f, 0.f, 0.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 1.f, 0.f, 0.f, 1.0f },
            {  1.0f,  1.0f, -1.0f, 1.0f, 0.f, 1.f, 0.f, 1.0f },
            {  1.0f,  1.0f, -1.0f, 1.0f, 0.f, 1.f, 0.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 1.f, 0.f, 0.f, 1.0f },
            { -1.0f,  1.0f, -1.0f, 1.0f, 1.f, 1.f, 0.f, 1.0f },

            {  1.0f, -1.0f, 1.0f, 1.0f, 0.f, 0.f, 1.f, 1.0f },
            {  1.0f,  1.0f, 1.0f, 1.0f, 0.f, 1.f, 1.f, 1.0f },
            { -1.0f, -1.0f, 1.0f, 1.0f, 1.f, 0.f, 1.f, 1.0f },
            { -1.0f, -1.0f, 1.0f, 1.0f, 1.f, 0.f, 1.f, 1.0f },
            {  1.0f,  1.0f, 1.0f, 1.0f, 0.f, 1.f, 1.f, 1.0f },
            { -1.0f,  1.0f, 1.0f, 1.0f, 1.f, 1.f, 1.f, 1.0f },

            { 1.0f, -1.0f,  1.0f, 1.0f, 1.f, 1.f, 1.f, 1.0f },
            { 1.0f, -1.0f, -1.0f, 1.0f, 1.f, 1.f, 0.f, 1.0f },
            { 1.0f,  1.0f,  1.0f, 1.0f, 1.f, 0.f, 1.f, 1.0f },
            { 1.0f,  1.0f,  1.0f, 1.0f, 1.f, 0.f, 1.f, 1.0f },
            { 1.0f, -1.0f, -1.0f, 1.0f, 1.f, 1.f, 0.f, 1.0f },
            { 1.0f,  1.0f, -1.0f, 1.0f, 1.f, 0.f, 0.f, 1.0f },

            { -1.0f, -1.0f,  1.0f, 1.0f, 0.f, 1.f, 1.f, 1.0f },
            { -1.0f,  1.0f,  1.0f, 1.0f, 0.f, 0.f, 1.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 1.f, 0.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 1.f, 0.f, 1.0f },
            { -1.0f,  1.0f,  1.0f, 1.0f, 0.f, 0.f, 1.f, 1.0f },
            { -1.0f,  1.0f, -1.0f, 1.0f, 0.f, 0.f, 0.f, 1.0f },

            {  1.0f, 1.0f, -1.0f, 1.0f, 1.f, 1.f, 1.f, 1.0f },
            { -1.0f, 1.0f, -1.0f, 1.0f, 0.f, 1.f, 1.f, 1.0f },
            {  1.0f, 1.0f,  1.0f, 1.0f, 1.f, 1.f, 0.f, 1.0f },
            {  1.0f, 1.0f,  1.0f, 1.0f, 1.f, 1.f, 0.f, 1.0f },
            { -1.0f, 1.0f, -1.0f, 1.0f, 0.f, 1.f, 1.f, 1.0f },
            { -1.0f, 1.0f,  1.0f, 1.0f, 0.f, 1.f, 0.f, 1.0f },

            {  1.0f, -1.0f, -1.0f, 1.0f, 1.f, 0.f, 1.f, 1.0f },
            {  1.0f, -1.0f,  1.0f, 1.0f, 1.f, 0.f, 0.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 0.f, 1.f, 1.0f },
            { -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 0.f, 1.f, 1.0f },
            {  1.0f, -1.0f,  1.0f, 1.0f, 1.f, 0.f, 0.f, 1.0f },
            { -1.0f, -1.0f,  1.0f, 1.0f, 0.f, 0.f, 0.f, 1.0f },
        };
        constexpr auto vertexCount = _countof(vertices);
        constexpr auto vertexStride = sizeof(VertexWithColor);
        constexpr auto vertexSize = vertexCount * vertexStride;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.size = vertexSize;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingVertexBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingVertexBufferAlloc = VK_NULL_HANDLE;
        VmaAllocationInfo stagingVertexBufferAllocInfo{};
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &stagingVertexBuffer, &stagingVertexBufferAlloc, &stagingVertexBufferAllocInfo)) {
            return false;
        }
        memcpy_s(stagingVertexBufferAllocInfo.pMappedData, vertexSize, vertices, vertexSize);

        // No need to flush stagingVertexBuffer memory because CPU_ONLY memory is always HOST_COHERENT.
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = 0;
        if(VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &g_vb.buffer, &g_vb.allocation, nullptr)) {
            return false;
        }

        if(false == ImmediateBufferCopy(g_vb.buffer, stagingVertexBuffer, {0, 0, vertexSize})) {
            return false;
        }

        vmaDestroyBuffer(Allocator::VMA(), stagingVertexBuffer, stagingVertexBufferAlloc);

        return true;
    }

    void DestroyVertexBuffer() {
        if (VK_NULL_HANDLE != g_vb.buffer) {
            vmaDestroyBuffer(Allocator::VMA(), g_vb.buffer, g_vb.allocation);
            g_vb.buffer = VK_NULL_HANDLE;
            g_vb.allocation = VK_NULL_HANDLE;
        }
    }

    struct VkIndexBuffer {
        VkBuffer      buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };
    VkIndexBuffer g_ib;

    bool CreateIndexBuffer() {
        constexpr uint16_t indices[] = { 0, 1, 3, 3, 1, 2 };
        constexpr auto indexCount = _countof(indices);
        constexpr auto indexStride = sizeof(uint16_t);
        constexpr auto indexSize = indexCount * indexStride;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.size = indexSize;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingIndexBuffer = VK_NULL_HANDLE;
        VmaAllocation stagingIndexBufferAlloc = VK_NULL_HANDLE;
        VmaAllocationInfo stagingIndexBufferAllocInfo{};
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &stagingIndexBuffer, &stagingIndexBufferAlloc, &stagingIndexBufferAllocInfo)) {
            return false;
        }
        memcpy_s(stagingIndexBufferAllocInfo.pMappedData, indexSize, indices, indexSize);

        // No need to flush stagingVertexBuffer memory because CPU_ONLY memory is always HOST_COHERENT.
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.flags = 0;
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &g_ib.buffer, &g_ib.allocation, nullptr)) {
            return false;
        }

        if (false == ImmediateBufferCopy(g_ib.buffer, stagingIndexBuffer, { 0, 0, indexSize })) {
            return false;
        }

        vmaDestroyBuffer(Allocator::VMA(), stagingIndexBuffer, stagingIndexBufferAlloc);

        return true;
    }

    void DestroyIndexBuffer() {
        if(VK_NULL_HANDLE != g_ib.buffer) {
            vmaDestroyBuffer(Allocator::VMA(), g_ib.buffer, g_ib.allocation);
            g_ib.buffer = VK_NULL_HANDLE;
            g_ib.allocation = VK_NULL_HANDLE;
        }
    }

    VkRenderPass g_renderPass = VK_NULL_HANDLE;
    bool CreateRenderPass(bool isSupportDepth, bool isClear) {
        VkAttachmentDescription attachments[2]{};

        attachments[0].format = g_swapchain.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = isClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[0].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

        if (isSupportDepth) {
            attachments[1].format = g_swapchain.depthFormat;
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp = isClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[1].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
        }

        const VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        const VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subPass{};
        subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPass.colorAttachmentCount = 1;
        subPass.pColorAttachments = &colorRef;
        subPass.pDepthStencilAttachment = isSupportDepth ? &depthRef : nullptr;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = isSupportDepth ? 2 : 1;
        info.pAttachments = attachments;
        info.subpassCount = 1;
        info.pSubpasses = &subPass;

        if (VK_SUCCESS != vkCreateRenderPass(g_device.device, &info, Allocator::CPU(), &g_renderPass)) {
            return false;
        }

        return true;
    }

    void DestroyRenderPass() {
        if (VK_NULL_HANDLE != g_renderPass) {
            vkDestroyRenderPass(g_device.device, g_renderPass, Allocator::CPU());
            g_renderPass = VK_NULL_HANDLE;
        }
    }

    using VkFramebuffers = std::vector<VkFramebuffer>;
    VkFramebuffers g_frameBuffers;
    bool CreateFrameBuffers(bool isIncludeDepth) {
        VkImageView attachments[2]{ VK_NULL_HANDLE, g_swapchain.depthImageView };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = g_renderPass;
        info.attachmentCount = isIncludeDepth ? 2 : 1;
        info.pAttachments = attachments;
        info.width = g_swapchain.width;
        info.height = g_swapchain.height;
        info.layers = 1;

        for (auto* imageView : g_swapchain.imageViews) {
            attachments[0] = imageView;

            VkFramebuffer frameBuffer = VK_NULL_HANDLE;
            if (VK_SUCCESS != vkCreateFramebuffer(g_device.device, &info, Allocator::CPU(), &frameBuffer)) {
                return false;
            }

            g_frameBuffers.emplace_back(frameBuffer);
        }

        return true;
    }

    void ClearFrameBuffers() {
        for (auto* frameBuffer : g_frameBuffers) {
            vkDestroyFramebuffer(g_device.device, frameBuffer, Allocator::CPU());
        }
        g_frameBuffers.clear();
    }

    VkShaderModule g_vsModule = VK_NULL_HANDLE;
    VkShaderModule g_fsModule = VK_NULL_HANDLE;

    VkShaderModule CreateShaderModule(std::string&& path) {
        const auto readFile = File::Read(std::move(path), std::ios::binary);
        if (readFile.empty()) {
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = static_cast<decltype(info.codeSize)>(readFile.size());
        info.pCode = reinterpret_cast<decltype(info.pCode)>(readFile.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateShaderModule(g_device.device, &info, Allocator::CPU(), &shaderModule)) {
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }

    bool CreateShaders() {
        g_vsModule = CreateShaderModule(Path::GetResourcePathAnsi() + "/shaders/draw_vert.spv"s);
        if (VK_NULL_HANDLE == g_vsModule) {
            return false;
        }

        g_fsModule = CreateShaderModule(Path::GetResourcePathAnsi() + "/shaders/draw_frag.spv"s);
        if (VK_NULL_HANDLE == g_fsModule) {
            return false;
        }

        return true;
    }

    void DestroyShaderModules() {
        const auto destroyFn = [](VkShaderModule& shaderModule) {
            if (VK_NULL_HANDLE != shaderModule) {
                vkDestroyShaderModule(g_device.device, shaderModule, Allocator::CPU());
                shaderModule = VK_NULL_HANDLE;
            }
        };

        destroyFn(g_fsModule);
        destroyFn(g_vsModule);
    }

    using VkDescriptorSetLayouts = std::vector<VkDescriptorSetLayout>;
    VkDescriptorSetLayouts g_descriptorSetLayouts;
    bool CreateDescriptorSetLayouts() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = &binding;

        g_descriptorSetLayouts.resize(1);
        if(VK_SUCCESS != vkCreateDescriptorSetLayout(g_device.device, &info, Allocator::CPU(), g_descriptorSetLayouts.data())) {
            return false;
        }

        return true;
    }

    void DestroyDescriptorSetLayouts() {
        for (auto* descLayout : g_descriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(g_device.device, descLayout, Allocator::CPU());
        }
        g_descriptorSetLayouts.clear();
    }

    struct UniformBuffer {
        VkBuffer          buffer = VK_NULL_HANDLE;
        VmaAllocation     allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocInfo{};
        glm::mat4         mvp;
    };
    UniformBuffer g_ub;
    bool CreateUniformBuffer() {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.size = sizeof(glm::mat4);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &bufferInfo, &allocInfo, &g_ub.buffer, &g_ub.allocation, &g_ub.allocInfo)) {
            return false;
        }

        return true;
    }

    void DestroyUniformBuffer() {
        if(VK_NULL_HANDLE != g_ub.buffer) {
            vmaDestroyBuffer(Allocator::VMA(), g_ub.buffer, g_ub.allocation);
            g_ub.buffer = VK_NULL_HANDLE;
            g_ub.allocation = VK_NULL_HANDLE;
        }
    }

    VkDescriptorPool g_descriptorPool = VK_NULL_HANDLE;
    bool CreateDescriptorPool() {
        std::vector<VkDescriptorPoolSize> descriptorSizeInfos;
        descriptorSizeInfos.emplace_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = 1;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.poolSizeCount = static_cast<decltype(info.poolSizeCount)>(descriptorSizeInfos.size());
        info.pPoolSizes = descriptorSizeInfos.data();

        if(VK_SUCCESS != vkCreateDescriptorPool(g_device.device, &info, Allocator::CPU(), &g_descriptorPool)) {
            return false;
        }

        return true;
    }

    void DestroyDescriptorPool() {
        if(VK_NULL_HANDLE != g_descriptorPool) {
            vkDestroyDescriptorPool(g_device.device, g_descriptorPool, Allocator::CPU());
            g_descriptorPool = VK_NULL_HANDLE;
        }
    }

    VkDescriptorSet g_descriptorSet = VK_NULL_HANDLE;
    bool AllocateAndUpdateDescriptorSets() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = g_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = g_descriptorSetLayouts.data();

        if(VK_SUCCESS != vkAllocateDescriptorSets(g_device.device, &allocInfo, &g_descriptorSet)) {
            return false;
        }

        const VkDescriptorBufferInfo ubInfo{ g_ub.buffer, 0, sizeof(glm::mat4) };

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = g_descriptorSet;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &ubInfo;
        write.dstArrayElement = 0;
        write.dstBinding = 0;

        vkUpdateDescriptorSets(g_device.device, 1, &write, 0, nullptr);

        return true;
    }

    void FreeDescriptorSets() {
        if(VK_NULL_HANDLE != g_descriptorSet) {
            vkFreeDescriptorSets(g_device.device, g_descriptorPool, 1, &g_descriptorSet);
            g_descriptorSet = VK_NULL_HANDLE;
        }
    }

    VkPipelineCache g_pipelineCache = VK_NULL_HANDLE;
    bool CreatePipelineCache() {
        VkPipelineCacheCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        if (VK_SUCCESS != vkCreatePipelineCache(g_device.device, &info, Allocator::CPU(), &g_pipelineCache)) {
            return false;
        }
        return true;
    }

    void DestroyPipelineCache() {
        if (VK_NULL_HANDLE != g_pipelineCache) {
            vkDestroyPipelineCache(g_device.device, g_pipelineCache, Allocator::CPU());
            g_pipelineCache = VK_NULL_HANDLE;
        }
    }

    VkPipelineLayout g_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline g_pipeline = VK_NULL_HANDLE;
    bool CreatePipeline(bool isIncludeDepth) {
        VkPushConstantRange pushConstantRanges[1]{};
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = 8/*colorFlag + mixerValue*/;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<decltype(pipelineLayoutCreateInfo.setLayoutCount)>(g_descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = g_descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = _countof(pushConstantRanges);
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

        if (VK_SUCCESS != vkCreatePipelineLayout(g_device.device, &pipelineLayoutCreateInfo, Allocator::CPU(), &g_pipelineLayout)) {
            return false;
        }

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = g_vsModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = g_fsModule;
        fragmentShaderStageInfo.pName = "main";

        const VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        struct VertexWithColor {
            float x, y, z, w;
            float r, g, b, a;
        };
        constexpr auto vertexStride = sizeof(VertexWithColor);

        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = vertexStride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attributeDescriptions[2]{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(float) * 4;

        VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{};
        vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputStateInfo.vertexAttributeDescriptionCount = _countof(attributeDescriptions);
        vertexInputStateInfo.pVertexAttributeDescriptions = attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterStateInfo{};
        rasterStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterStateInfo.depthClampEnable = VK_FALSE;
        rasterStateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterStateInfo.depthBiasEnable = VK_FALSE;
        rasterStateInfo.depthBiasConstantFactor = 0;
        rasterStateInfo.depthBiasClamp = 0;
        rasterStateInfo.depthBiasSlopeFactor = 0;
        rasterStateInfo.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo[1]{};
        colorBlendAttachmentStateInfo[0].colorWriteMask = 0xf;
        colorBlendAttachmentStateInfo[0].blendEnable = VK_FALSE;
        colorBlendAttachmentStateInfo[0].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentStateInfo[0].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentStateInfo[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
        colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateInfo.attachmentCount = 1;
        colorBlendStateInfo.pAttachments = colorBlendAttachmentStateInfo;
        colorBlendStateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateInfo.logicOp = VK_LOGIC_OP_NO_OP;
        colorBlendStateInfo.blendConstants[0] = 1.0f;
        colorBlendStateInfo.blendConstants[1] = 1.0f;
        colorBlendStateInfo.blendConstants[2] = 1.0f;
        colorBlendStateInfo.blendConstants[3] = 1.0f;

        const VkDynamicState dynamicState[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pDynamicStates = dynamicState;
        dynamicStateInfo.dynamicStateCount = _countof(dynamicState);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<decltype(viewport.width)>(g_swapchain.width);
        viewport.height = static_cast<decltype(viewport.height)>(g_swapchain.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = { g_swapchain.width, g_swapchain.height };

        VkPipelineViewportStateCreateInfo viewportStateInfo{};
        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissor;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
        depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateInfo.depthTestEnable = isIncludeDepth;
        depthStencilStateInfo.depthWriteEnable = isIncludeDepth;
        depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateInfo.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilStateInfo.back.compareMask = 0;
        depthStencilStateInfo.back.reference = 0;
        depthStencilStateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.writeMask = 0;
        depthStencilStateInfo.minDepthBounds = 0;
        depthStencilStateInfo.maxDepthBounds = 0;
        depthStencilStateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateInfo.front = depthStencilStateInfo.back;

        VkPipelineMultisampleStateCreateInfo multiSampleStateInfo{};
        multiSampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multiSampleStateInfo.pSampleMask = nullptr;
        multiSampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multiSampleStateInfo.sampleShadingEnable = VK_FALSE;
        multiSampleStateInfo.alphaToCoverageEnable = VK_FALSE;
        multiSampleStateInfo.alphaToOneEnable = VK_FALSE;
        multiSampleStateInfo.minSampleShading = 0.0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = g_pipelineLayout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.pVertexInputState = &vertexInputStateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &rasterStateInfo;
        pipelineInfo.pColorBlendState = &colorBlendStateInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pMultisampleState = &multiSampleStateInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pViewportState = &viewportStateInfo;
        pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineInfo.pStages = shaderStageInfos;
        pipelineInfo.stageCount = _countof(shaderStageInfos);
        pipelineInfo.renderPass = g_renderPass;
        pipelineInfo.subpass = 0;

        if (VK_SUCCESS != vkCreateGraphicsPipelines(g_device.device, g_pipelineCache, 1, &pipelineInfo, Allocator::CPU(), &g_pipeline)) {
            return false;
        }
        return true;
    }

    void DestroyPipeline() {
        if(VK_NULL_HANDLE != g_pipeline) {
            vkDestroyPipeline(g_device.device, g_pipeline, Allocator::CPU());
            g_pipeline = VK_NULL_HANDLE;
        }

        if(VK_NULL_HANDLE != g_pipelineLayout) {
            vkDestroyPipelineLayout(g_device.device, g_pipelineLayout, Allocator::CPU());
            g_pipelineLayout = VK_NULL_HANDLE;
        }
    }

    bool SettingPushConstants() {
        VkCommandBufferAllocateInfo cmdBufInfo{};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = g_gpuCmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        VkCommandBuffer immediatelyCmdBuf = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkAllocateCommandBuffers(g_device.device, &cmdBufInfo, &immediatelyCmdBuf)) {
            return false;
        }

        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (VK_SUCCESS != vkBeginCommandBuffer(immediatelyCmdBuf, &cmdBufBeginInfo)) {
            return false;
        }

        const auto mixerValue = 0.3f;
        uint32_t constants[2] = { 2, 0 };
        memcpy_s(&constants[1], sizeof(float), &mixerValue, sizeof(float));
        vkCmdPushConstants(immediatelyCmdBuf, g_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), constants);

        vkEndCommandBuffer(immediatelyCmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &immediatelyCmdBuf;
        vkQueueSubmit(g_device.gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(g_device.gpuQueue);

        vkFreeCommandBuffers(g_device.device, g_gpuCmdPool, 1, &immediatelyCmdBuf);

        return true;
    }

    using VkCommandBuffers = std::vector<VkCommandBuffer>;
    VkCommandBuffers g_cmdBuffers;

    bool AllocateAndFillCommandBuffers() {
        for (decltype(g_swapchain.images.size()) i = 0; i < g_swapchain.images.size(); ++i) {
            // Create.
            VkCommandBufferAllocateInfo cmdInfo{};
            cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdInfo.commandPool = g_gpuCmdPool;
            cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdInfo.commandBufferCount = 1;

            VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
            if (VK_SUCCESS != vkAllocateCommandBuffers(g_device.device, &cmdInfo, &cmdBuffer)) {
                return false;
            }
            g_cmdBuffers.emplace_back(cmdBuffer);

            // Begin.
            VkCommandBufferInheritanceInfo cmdBufInheritInfo{};
            cmdBufInheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

            VkCommandBufferBeginInfo cmdBufInfo{};
            cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBufInfo.pInheritanceInfo = &cmdBufInheritInfo;

            if (VK_SUCCESS != vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo)) {
                return false;
            }

            // Record.
            VkClearValue clearValues[2]{};
            clearValues[0].color = { 0.27f, 0.39f, 0.49f, 1.0f };
            clearValues[1].depthStencil.depth = 1.0f;
            clearValues[1].depthStencil.stencil = 0;

            VkRenderPassBeginInfo renderPassBegin{};
            renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBegin.renderPass = g_renderPass;
            renderPassBegin.framebuffer = g_frameBuffers[i];
            renderPassBegin.renderArea.extent.width = g_swapchain.width;
            renderPassBegin.renderArea.extent.height = g_swapchain.height;
            renderPassBegin.clearValueCount = 2;
            renderPassBegin.pClearValues = clearValues;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipelineLayout, 0, 1, &g_descriptorSet, 0, nullptr);
            const VkDeviceSize offsets[1]{};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &g_vb.buffer, offsets);
            vkCmdBindIndexBuffer(cmdBuffer, g_ib.buffer, 0, VK_INDEX_TYPE_UINT16);
            const VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(g_swapchain.width), static_cast<float>(g_swapchain.height), 0.0f, 1.0f };
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            const VkRect2D scissor{ {0, 0}, {g_swapchain.width, g_swapchain.height} };
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
            //vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
            vkCmdDraw(cmdBuffer, 3 * 2 * 6, 1, 0, 0);

            vkCmdEndRenderPass(cmdBuffer);

            // End.
            vkEndCommandBuffer(cmdBuffer);
        }

        return true;
    }

    void FreeCommandBuffers() {
        if (g_cmdBuffers.empty()) {
            return;
        }

        vkFreeCommandBuffers(g_device.device, g_gpuCmdPool, static_cast<uint32_t>(g_cmdBuffers.size()), g_cmdBuffers.data());
        g_cmdBuffers.clear();
    }

    bool Initialize() {
        CollectingLayerProperties();
        if (false == CreateInstance()) {
            return false;
        }

#if defined(_DEBUG)
        Debugger::Initialize(g_instance);
#endif

        if (false == CreateSurface()) {
            return false;
        }

        CollectPhysicalDevices();
        if (false == CreateDevice()) {
            return false;
        }

        if (false == Allocator::CreateVMA(g_instance, g_device.gpuDevice, g_device.device)) {
            return false;
        }

        if (false == CreateGPUCommandPool()) {
            return false;
        }

        if (false == CreateSwapchain()) {
            return false;
        }

        if (false == CreateRenderPass(true, true)) {
            return false;
        }

        if (false == CreateFrameBuffers(true)) {
            return false;
        }

        if (false == CreateVertexBuffer() || false == CreateIndexBuffer()) {
            return false;
        }

        if (false == CreateShaders()) {
            return false;
        }

        if(false == CreateDescriptorSetLayouts()) {
            return false;
        }

        if(false == CreateUniformBuffer()) {
            return false;
        }

        if(false == CreateDescriptorPool()) {
            return false;
        }

        if(false == AllocateAndUpdateDescriptorSets()) {
            return false;
        }

        if (false == CreatePipelineCache()) {
            return false;
        }

        if (false == CreatePipeline(true)) {
            return false;
        }

        if(false == SettingPushConstants()) {
            return false;
        }

        if (false == AllocateAndFillCommandBuffers()) {
            return false;
        }

        return true;
    }

    void Release() {
        FreeCommandBuffers();
        DestroyPipeline();
        DestroyPipelineCache();
        FreeDescriptorSets();
        DestroyDescriptorPool();
        DestroyUniformBuffer();
        DestroyDescriptorSetLayouts();
        DestroyShaderModules();
        DestroyIndexBuffer();
        DestroyVertexBuffer();
        ClearFrameBuffers();
        DestroyRenderPass();
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

    uint32_t currentFrameBufferIndex = 0;
    void Run(float delta) {
        // Update uniform buffer.
        const auto projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
        const auto view = glm::lookAt(
            glm::vec3(0, 5, 5), // Camera is in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up
        );
        static auto rotate = 0.0f;
        rotate += delta;

        const auto model = glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0.0f, 1.0f, 0.0f));
        const auto mvp = projection * view * model;

        VkMappedMemoryRange memRange{};
        memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memRange.memory = g_ub.allocInfo.deviceMemory;
        memRange.size = sizeof(glm::mat4);

        VK_CHECK(vmaInvalidateAllocation(Allocator::VMA(), g_ub.allocation, 0, sizeof(glm::mat4)));
        memcpy_s(g_ub.allocInfo.pMappedData, sizeof(glm::mat4), &mvp, sizeof(glm::mat4));
        VK_CHECK(vmaFlushAllocation(Allocator::VMA(), g_ub.allocation, 0, sizeof(glm::mat4)));

        // Acquire image index.
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore presentSemaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(g_device.device, &semaphoreInfo, Allocator::CPU(), &presentSemaphore);

        VkSemaphore drawingSemaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(g_device.device, &semaphoreInfo, Allocator::CPU(), &drawingSemaphore);

        if (VK_SUCCESS != vkAcquireNextImageKHR(g_device.device, g_swapchain.handle, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &currentFrameBufferIndex)) {
            return;
        }

        // Submit.
        const VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &g_cmdBuffers[currentFrameBufferIndex];
        submitInfo.pWaitSemaphores = &presentSemaphore;
        submitInfo.pWaitDstStageMask = &submitPipelineStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &drawingSemaphore;

        vkQueueSubmit(g_device.gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(g_device.gpuQueue);

        // Present.
        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.swapchainCount = 1;
        present.pSwapchains = &g_swapchain.handle;
        present.pImageIndices = &currentFrameBufferIndex;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &drawingSemaphore;

        // Queue the image for presentation.
        vkQueuePresentKHR(g_device.gpuQueue, &present);

        vkDestroySemaphore(g_device.device, drawingSemaphore, Allocator::CPU());
        vkDestroySemaphore(g_device.device, presentSemaphore, Allocator::CPU());
    }

    void Resize(uint32_t /*width*/, uint32_t /*height*/) {
        if(VK_NULL_HANDLE == g_instance) {
            return;
        }

        vkDeviceWaitIdle(g_device.device);

        FreeCommandBuffers();
        DestroyPipeline();
        DestroyPipelineCache();
        ClearFrameBuffers();
        DestroyRenderPass();
        DestroySwapchain();
        DestroyGPUCommandPool();

        VK_CHECK(CreateGPUCommandPool());
        VK_CHECK(CreateSwapchain());
        VK_CHECK(CreateRenderPass(true, true));
        VK_CHECK(CreateFrameBuffers(true));
        VK_CHECK(CreatePipelineCache());
        VK_CHECK(CreatePipeline(true));
        VK_CHECK(SettingPushConstants());
        VK_CHECK(AllocateAndFillCommandBuffers());
    }
}
