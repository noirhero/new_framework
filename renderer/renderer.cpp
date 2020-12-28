// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "renderer.h"

#include "../win_handle.h"
#include "allocator_cpu.h"
#include "debugger.h"

namespace Renderer {
    using VkLayerPropArray = std::vector<VkLayerProperties>;
    using VkExtensionPropArray = std::vector<VkExtensionProperties>;
    using VkPhysicalDevices = std::vector<VkPhysicalDevice>;
    using VkQueueFamilyPropArray = std::vector<VkQueueFamilyProperties>;

    struct InstanceLayer {
        VkLayerProperties    property{};
        VkExtensionPropArray extensions;
    };
    using InstanceLayers = std::vector<InstanceLayer>;
    InstanceLayers g_instanceLayers;

    bool CollectingLayerProperties() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if(0 == layerCount) {
            return false;
        }

        VkLayerPropArray layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for(const auto& layer : layers) {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr);
            if(0 == extensionCount) {
                continue;
            }

            VkExtensionPropArray extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, extensions.data());

            g_instanceLayers.emplace_back(layer, extensions);
        }

        return true;
    }

    VkInstance g_instance = VK_NULL_HANDLE;

    bool CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instInfo{};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> enableLayerNames = {
            "VK_LAYER_LUNARG_api_dump"
        };
        std::vector<const char*> enableExtensionNames = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        };
#if defined(_DEBUG)
        for(const auto& layer : g_instanceLayers) {
            enableLayerNames.emplace_back(layer.property.layerName);

            for(const auto& extension : layer.extensions) {
                enableExtensionNames.emplace_back(extension.extensionName);
            }
        }
#endif
        instInfo.enabledLayerCount = static_cast<uint32_t>(enableLayerNames.size());
        instInfo.ppEnabledLayerNames = enableLayerNames.data();

        instInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensionNames.size());
        instInfo.ppEnabledExtensionNames = enableExtensionNames.data();

        if(VK_SUCCESS != vkCreateInstance(&instInfo, Allocator::CPU(), &g_instance)) {
            return false;
        }

        return true;
    }

    void DestroyInstance() {
        if(VK_NULL_HANDLE != g_instance) {
            vkDestroyInstance(g_instance, Allocator::CPU());
            g_instance = VK_NULL_HANDLE;
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

        for(auto* device : devices) {
            g_physicalDevices.emplace_back(device);
            auto& physicalDevice = g_physicalDevices.back();

            vkGetPhysicalDeviceProperties(device, &physicalDevice.property);
            vkGetPhysicalDeviceMemoryProperties(device, &physicalDevice.memoryProperty);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            if(0 < queueFamilyCount) {
                physicalDevice.queueFamilyProperties.resize(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, physicalDevice.queueFamilyProperties.data());
            }

            for(const auto& layer : layers) {
                uint32_t extensionCount = 0;
                vkEnumerateDeviceExtensionProperties(device, layer.layerName, &extensionCount, nullptr);
                if(0 == extensionCount) {
                    continue;
                }

                VkExtensionPropArray extensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, layer.layerName, &extensionCount, extensions.data());

                physicalDevice.layers.emplace_back(layer, extensions);
            }
        }
    }

    struct Device {
        VkDevice         device{ VK_NULL_HANDLE };
        VkPhysicalDevice gpuDevice{ VK_NULL_HANDLE };
        uint32_t         gpuQueueIndex{ 0 };
        VkQueue          gpuQueue{ VK_NULL_HANDLE };
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
        if(VK_SUCCESS != vkCreateDevice(gpuDevice, &deviceInfo, Allocator::CPU(), &device)) {
            return false;
        }

        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, gpuQueueIndex, 0, &queue);

        g_device = { device, gpuDevice, gpuQueueIndex, queue };
        return true;
    }

    void DestroyDevice() {
        if(VK_NULL_HANDLE != g_device.device) {
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

        if(VK_SUCCESS != vkCreateCommandPool(g_device.device, &info, Allocator::CPU(), &g_gpuCmdPool)) {
            return false;
        }

        return true;
    }

    void DestroyGPUCommandPool() {
        if(VK_NULL_HANDLE != g_gpuCmdPool) {
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
        if(VK_SUCCESS != vkAllocateCommandBuffers(g_device.device, &info, &g_gpuCmdBuf)) {
            return false;
        }

        return true;
    }

    struct Swapchain {
        VkSurfaceKHR                    surface = VK_NULL_HANDLE;
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
    };
    Swapchain g_swapchain;

    bool CreateSwapchain() {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = Win::Instance();
        surfaceInfo.hwnd = Win::Handle();

        if (VK_SUCCESS != vkCreateWin32SurfaceKHR(g_instance, &surfaceInfo, Allocator::CPU(), &g_swapchain.surface)) {
            return false;
        }

        //PFN_vkDestroySurfaceKHR(vkGetInstanceProcAddr(g_instance, "vkDestroySurfaceKHR"));

        //PFN_vkCreateSwapchainKHR(vkGetInstanceProcAddr(g_instance, "vkCreateSwapchainKHR"));
        //PFN_vkDestroySwapchainKHR(vkGetInstanceProcAddr(g_instance, "vkDestroySwapchainKHR"));
        //PFN_vkGetSwapchainImagesKHR(vkGetInstanceProcAddr(g_instance, "vkGetSwapchainImagesKHR"));
        //PFN_vkAcquireNextImageKHR(vkGetInstanceProcAddr(g_instance, "vkAcquireNextImageKHR"));
        //PFN_vkQueuePresentKHR(vkGetInstanceProcAddr(g_instance, "vkQueuePresentKHR"));


        // Present queue index.
        auto* getSurfaceSupportFn = PFN_vkGetPhysicalDeviceSurfaceSupportKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
        for(const auto& physicalDevice : g_physicalDevices) {
            if(g_device.gpuDevice != physicalDevice.device) {
                continue;
            }

            uint32_t queueIndex = 0;
            for(const auto& queueProperty : physicalDevice.queueFamilyProperties) {
                (void)queueProperty;

                VkBool32 isSupportPresent = VK_FALSE;
                getSurfaceSupportFn(g_device.gpuDevice, queueIndex, g_swapchain.surface, &isSupportPresent);
                if(VK_FALSE == isSupportPresent) {
                    ++queueIndex;
                    continue;
                }

                g_swapchain.presentQueueIndex = queueIndex;
                if (g_device.gpuQueueIndex == g_swapchain.presentQueueIndex) {
                    break;
                }
            }
            break;
        }

        // Format.
        auto* getSurfaceFormatFn = PFN_vkGetPhysicalDeviceSurfaceFormatsKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
        uint32_t formatCount = 0;
        getSurfaceFormatFn(g_device.gpuDevice, g_swapchain.surface, &formatCount, nullptr);
        if(0 == formatCount) {
            return false;
        }

        auto& formats = g_swapchain.supportFormats;
        formats.resize(formatCount);
        getSurfaceFormatFn(g_device.gpuDevice, g_swapchain.surface, &formatCount, formats.data());

        if(1 == formatCount && VK_FORMAT_UNDEFINED == formats[0].format) {
            g_swapchain.format = VK_FORMAT_R8G8B8A8_UNORM;
        }
        else {
            g_swapchain.format = formats[0].format;
        }

        // Capabilities.
        auto* getSurfaceCapabilities = PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        getSurfaceCapabilities(g_device.gpuDevice, g_swapchain.surface, &g_swapchain.capabilities);

        if(std::numeric_limits<uint32_t>::max() == g_swapchain.capabilities.currentExtent.width) {
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
        if(0 < g_swapchain.capabilities.maxImageCount) {
            g_swapchain.imageCount = std::min(g_swapchain.imageCount, g_swapchain.capabilities.maxImageCount);
        }

        if(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & g_swapchain.capabilities.supportedTransforms) {
            g_swapchain.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            g_swapchain.preTransform = g_swapchain.capabilities.currentTransform;
        }

        // Preset mode.
        auto* getSurfacePresendModeFn = PFN_vkGetPhysicalDeviceSurfacePresentModesKHR(vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
        uint32_t presentModeCount = 0;
        getSurfacePresendModeFn(g_device.gpuDevice, g_swapchain.surface, &presentModeCount, nullptr);
        if(0 == presentModeCount) {
            return false;
        }

        auto& presentModes = g_swapchain.presentModes;
        presentModes.resize(presentModeCount);
        getSurfacePresendModeFn(g_device.gpuDevice, g_swapchain.surface, &presentModeCount, presentModes.data());

        for(const auto supportPresentMode : presentModes) {
            if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                break;
            }
            if(VK_PRESENT_MODE_MAILBOX_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if(VK_PRESENT_MODE_IMMEDIATE_KHR == supportPresentMode) {
                g_swapchain.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        return true;
    }

    bool Initialize() {
        CollectingLayerProperties();
        if(false == CreateInstance()) {
            return false;
        }

#if defined(_DEBUG)
        Debugger::Initialize(g_instance);
#endif

        CollectPhysicalDevices();
        if(false == CreateDevice()) {
            return false;
        }
        if(false == CreateGPUCommandPool()) {
            return false;
        }

        if(false == CreateSwapchain()) {
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
        DestroyGPUCommandPool();
        DestroyDevice();

#if defined(_DEBUG)
        Debugger::Destroy(g_instance);
#endif

        DestroyInstance();
    }
}
