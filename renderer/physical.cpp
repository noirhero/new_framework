// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "physical.h"

#include "renderer_util.h"
#include "allocator_cpu.h"

namespace Physical {
    using namespace Renderer;

    // Instance
    using VkExtensionPropertyArray = std::vector<VkExtensionProperties>;
    struct InstanceLayer {
        VkLayerProperties        property{};
        VkExtensionPropertyArray extensions;
    };

    std::vector<InstanceLayer> g_instanceLayers;
    VkInstance                 g_instance = VK_NULL_HANDLE;

    VkInstance Instance::Get() {
        return g_instance;
    }

    bool CollectingLayerProperties() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (0 == layerCount) {
            return false;
        }

        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        const auto findKhronosLayerFn = [](const VkLayerProperties& layerProperty)->bool {
            return "VK_LAYER_KHRONOS_validation"s == layerProperty.layerName;
        };
        if (layers.end() == std::find_if(layers.begin(), layers.end(), findKhronosLayerFn)) {
            Output::Print("This hardware not support VULKAN.\n"s);
            return false;
        }

        { // Check to common extensions
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            VkExtensionPropertyArray extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            for (const auto& extensionProperties : extensions) {
                Util::CheckToInstanceExtensionProperties(extensionProperties);
            }
        }

        for (const auto& layer : layers) {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr);
            if (0 == extensionCount) {
                g_instanceLayers.emplace_back(InstanceLayer{ layer });
                continue;
            }

            VkExtensionPropertyArray extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, extensions.data());

            for (const auto& extensionProperties : extensions) {
                Util::CheckToInstanceExtensionProperties(extensionProperties);
            }

            g_instanceLayers.emplace_back(InstanceLayer{ layer, extensions });
        }

        return true;
    }

    bool Instance::Create() {
        if(false == CollectingLayerProperties()) {
            return false;
        }

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
        instInfo.enabledLayerCount = static_cast<uint32_t>(enableLayerNames.size());
        instInfo.ppEnabledLayerNames = enableLayerNames.data();

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
        instInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensionNames.size());
        instInfo.ppEnabledExtensionNames = enableExtensionNames.data();

        if (VK_SUCCESS != vkCreateInstance(&instInfo, Allocator::CPU(), &g_instance)) {
            return false;
        }

        return true;
    }

    void Instance::Destroy() {
        if(VK_NULL_HANDLE != g_instance) {
            vkDestroyInstance(g_instance, Allocator::CPU());
            g_instance = VK_NULL_HANDLE;
        }

        g_instanceLayers.clear();
    }

    // Device
    using VkQueueFamilyPropertiesArray = std::vector<VkQueueFamilyProperties>;

    struct PhysicalDevice {
        VkPhysicalDevice                 handle{ VK_NULL_HANDLE };

        VkPhysicalDeviceProperties       property{};
        VkPhysicalDeviceMemoryProperties memoryProperty{};
        VkQueueFamilyPropertiesArray     queueFamilyProperties;

        uint32_t                         gpuQueueIndex = std::numeric_limits<uint32_t>::max();
        uint32_t                         presentQueueIndex = std::numeric_limits<uint32_t>::max();
        uint32_t                         sparesQueueIndex = std::numeric_limits<uint32_t>::max();
    };

    std::vector<PhysicalDevice> g_physicalDevices;

    PhysicalDeviceInfo Device::GetGPU() {
        for(const auto& device : g_physicalDevices) {
            if (Util::IsValidQueue(device.gpuQueueIndex))
                return { device.handle, device.gpuQueueIndex, device.presentQueueIndex, device.sparesQueueIndex };
        }
        return {};
    }

    void Device::Collect(VkSurfaceKHR surface) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(g_instance, &deviceCount, nullptr);
        if (0 == deviceCount) {
            return;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(g_instance, &deviceCount, devices.data());

        for (auto* device : devices) {
            uint32_t extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            if (0 == extensionCount) {
                continue;
            }

            VkExtensionPropertyArray extensions(extensionCount);
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

            if (Util::CheckToQueueFamilyProperties(device, surface, physicalDevice.queueFamilyProperties)) {
                physicalDevice.gpuQueueIndex = Util::GetGPUQueueIndex();
                physicalDevice.presentQueueIndex = Util::GetPresentQueueIndex();
                physicalDevice.sparesQueueIndex = Util::GetSparesQueueIndex();
            }
        }
    }
}
