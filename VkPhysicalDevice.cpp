// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkPhysicalDevice.h"

#include "VkCommon.h"

bool Vk::PhysicalDevice::Initialize(VkInstance instance)
{
	uint32_t gpuCount = 0;
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
	assert(gpuCount > 0);

	_devices.resize(gpuCount, VK_NULL_HANDLE);
	const auto result = vkEnumeratePhysicalDevices(instance, &gpuCount, _devices.data());
	if (VK_SUCCESS != result)
	{
		std::cerr << "Could not enumerate physical devices!" << std::endl;
		exit(result);
	}

	_selectIdx = 0;
	_selectDevice = _devices[_selectIdx];

	vkGetPhysicalDeviceProperties(_selectDevice, &_deviceProperties);
	vkGetPhysicalDeviceFeatures(_selectDevice, &_deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(_selectDevice, &_deviceMemoryProperties);

	return true;
}
