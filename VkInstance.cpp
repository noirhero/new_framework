// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkInstance.h"

#include "VkCommon.h"

namespace Vk
{
	bool Instance::Initialize(const Settings& settings)
	{
		const VkApplicationInfo appInfo
		{
			VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr/*pNext*/,
			"ApplicationName", 0/*applicationVersion*/,
			"EngineName", 0/*engineVersion*/,
			VK_API_VERSION_1_0
		};

		std::vector<const char*> instanceExtensions
		{
			VK_KHR_SURFACE_EXTENSION_NAME
		};

		// Enable surface extensions depending on os
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

		VkInstanceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &appInfo;

		if (false == instanceExtensions.empty())
		{
			if (true == settings.validation)
			{
				instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}

			info.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			info.ppEnabledExtensionNames = instanceExtensions.data();
		}

		if (true == settings.validation)
		{
			info.enabledLayerCount = 1;
			const char *validationLayerNames[] =
			{
				"VK_LAYER_LUNARG_standard_validation"
			};
			info.ppEnabledLayerNames = validationLayerNames;

			if (VK_SUCCESS == vkCreateInstance(&info, nullptr, &_instance))
				return true;
		}
		else
		{
			if (VK_SUCCESS == vkCreateInstance(&info, nullptr, &_instance))
				return true;
		}

		return false;
	}

	void Instance::Release()
	{
		if (VK_NULL_HANDLE == _instance)
			return;

		vkDestroyInstance(_instance, nullptr);
		_instance = VK_NULL_HANDLE;
	}
}
