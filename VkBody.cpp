// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkBody.h"

#include "VkCommon.h"
#include "VkDevice.h"
#include "VkSwapChain.h"

namespace Vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugMsgCallback(
		VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject, size_t location, int32_t msgCode,
		const char * pLayerPrefix, const char * pMsg, void * pUserData)
	{
		std::string prefix("");
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
		debugMessage << prefix << " [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";
		std::cout << debugMessage.str() << "\n";
		fflush(stdout);

		return VK_FALSE;
	}

	bool Body::Initialize()
	{
		bool isValidate = false;
#if defined(_DEBUG)
		isValidate = true;
#endif

		if (false == CreateInstance(isValidate))
			return false;

		if (true == isValidate)
		{
			AllocConsole();
			AttachConsole(GetCurrentProcessId());
			FILE *stream;
			freopen_s(&stream, "CONOUT$", "w+", stdout);
			freopen_s(&stream, "CONOUT$", "w+", stderr);
			SetConsoleTitle(TEXT("Vulkan validation output"));

			const auto debugReportCreateFn = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(_inst, "vkCreateDebugReportCallbackEXT"));
			if (nullptr != debugReportCreateFn)
			{
				VkDebugReportCallbackCreateInfoEXT debugCreateInfo{};
				debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
				debugCreateInfo.pfnCallback = DebugMsgCallback;
				debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

				VK_CHECK_RESULT(debugReportCreateFn(_inst, &debugCreateInfo, nullptr, &_debugExt));
			}
		}

		SelectPhysicalDevice();

		/*
			Device creation
		*/
		_vulkanDevice = new VulkanDevice(_physicalDevice);

		VkPhysicalDeviceFeatures enabledFeatures{};
		if (VK_TRUE == _deviceFeatures.samplerAnisotropy)
			enabledFeatures.samplerAnisotropy = VK_TRUE;

		std::vector<const char*> enabledExtensions;
		const auto res = _vulkanDevice->createLogicalDevice(enabledFeatures, enabledExtensions);
		if (VK_SUCCESS != res)
		{
			std::cerr << "Could not create Vulkan device!" << std::endl;
			exit(res);
		}

		_device = _vulkanDevice->logicalDevice;

		/*
			Graphics queue
		*/
		vkGetDeviceQueue(_device, _vulkanDevice->queueFamilyIndices.graphics, 0, &_queue);

		/*
			Suitable depth format
		*/
		const std::vector<VkFormat> depthFormats =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		VkBool32 validDepthFormat = VK_FALSE;
		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps{};
			vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &formatProps);

			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				_depthFormat = format;
				validDepthFormat = VK_TRUE;
				break;
			}
		}
		assert(VK_TRUE == validDepthFormat);

		_swapChain = new VulkanSwapChain();
		_swapChain->connect(_inst, _physicalDevice, _device);

		return true;
	}

	void Body::Release()
	{
	}

	bool Body::CreateInstance(bool isValidate)
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "New Framework";
		appInfo.pEngineName = "Paradise";
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

		// Enable surface extensions depending on os
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = NULL;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		if (instanceExtensions.size() > 0)
		{
			if (true == isValidate)
			{
				instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}
			instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		}

		if (true == isValidate)
		{
			instanceCreateInfo.enabledLayerCount = 1;
			const char *validationLayerNames[] =
			{
				"VK_LAYER_LUNARG_standard_validation"
			};
			instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
		}

		return VK_SUCCESS == vkCreateInstance(&instanceCreateInfo, nullptr, &_inst);
	}

	void Body::SelectPhysicalDevice()
	{
		uint32_t gpuCount = 0;
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(_inst, &gpuCount, nullptr));
		assert(gpuCount > 0);

		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		const auto err = vkEnumeratePhysicalDevices(_inst, &gpuCount, physicalDevices.data());
		if(VK_SUCCESS != err)
		{
			std::cerr << "Could not enumerate physical devices!" << std::endl;
			exit(err);
		}

		const uint32_t selectedDevice = 0;
		_physicalDevice = physicalDevices[selectedDevice];

		vkGetPhysicalDeviceProperties(_physicalDevice, &_deviceProperties);
		vkGetPhysicalDeviceFeatures(_physicalDevice, &_deviceFeatures);
		vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_deviceMemoryProperties);
	}
}
