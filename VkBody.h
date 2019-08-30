// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	struct VulkanDevice;
	class VulkanSwapChain;

	class Body
	{
	public:
		bool Initialize();
		void Release();

	private:
		bool CreateInstance(bool isValidate);
		void SelectPhysicalDevice();

		VkDebugReportCallbackEXT _debugExt = VK_NULL_HANDLE;

		VkInstance _inst = VK_NULL_HANDLE;

		VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties _deviceProperties{};
		VkPhysicalDeviceFeatures _deviceFeatures{};
		VkPhysicalDeviceMemoryProperties _deviceMemoryProperties{};

		VulkanDevice* _vulkanDevice = nullptr;
		VkDevice _device = VK_NULL_HANDLE;
		VkQueue _queue = VK_NULL_HANDLE;
		VkFormat _depthFormat = VK_FORMAT_UNDEFINED;

		VulkanSwapChain* _swapChain = nullptr;
	};
}
