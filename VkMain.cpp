// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkMain.h"

#include "VkDevice.h"
#include "VkSwapChain.h"

namespace Vk
{
	void CleanupSwapChain(VulkanSwapChain* ptr)
	{
		ptr->cleanup();
		delete ptr;
	}

	VkFormat FindDepthFormat(VkPhysicalDevice selectDevice)
	{
		const std::vector<VkFormat> depthFormats =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		for (auto& format : depthFormats)
		{
			VkFormatProperties formatProps{};
			vkGetPhysicalDeviceFormatProperties(selectDevice, format, &formatProps);

			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				return format;
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	bool Main::Initialize(const Settings & settings)
	{
		if (false == InitializePhysicalGroup(settings))
			return false;

		if (false == InitializeLogicalGroup())
			return false;

		return true;
	}

	void Main::Prepare(Settings& settings, HINSTANCE instance, HWND window)
	{
		_swapChain->initSurface(instance, window);
		_swapChain->create(&settings.width, &settings.height, settings.vsync);

		_cmdPool.Initialize(_swapChain->queueNodeIndex, _logicalDevice);

		_renderPass.Initialize(settings, _swapChain->colorFormat, _depthFormat, _logicalDevice);

		_pipelineCache.Initialize(_logicalDevice);
	}

	void Main::Release()
	{
		_pipelineCache.Release(_logicalDevice);
		_renderPass.Release(_logicalDevice);
		_cmdPool.Release(_logicalDevice);

		ReleaseLogicalGroup();
		ReleasePhysicalGroup();
	}

	void Main::RecreateSwapChain(Settings & settings)
	{
		_swapChain->create(&settings.width, &settings.height, settings.vsync);
	}

	VkResult Main::AcquireNextImage(uint32_t & currentBuffer, VkSemaphore semaphore)
	{
		return _swapChain->acquireNextImage(semaphore, &currentBuffer);
	}

	VkResult Main::QueuePresent(uint32_t currentBuffer, VkSemaphore semaphore)
	{
		return _swapChain->queuePresent(_gpuQueue, currentBuffer, semaphore);
	}

	uint32_t Main::GetSwapChainImageCount() const
	{
		return _swapChain->imageCount;
	}

	bool Main::InitializePhysicalGroup(const Settings& settings)
	{
		if (false == _inst.Initialize(settings))
			return false;

		_instanceHandle = _inst.Get();

		if (true == settings.validation)
			_debug.Initialize(_instanceHandle);

		_physDevice.Initialize(_inst.Get());
		_selectDevice = _physDevice.Get();

		return true;
	}

	void Main::ReleasePhysicalGroup()
	{
		_selectDevice = VK_NULL_HANDLE;

		_debug.Release(_instanceHandle);

		_instanceHandle = VK_NULL_HANDLE;
		_inst.Release();
	}

	bool Main::InitializeLogicalGroup()
	{
		_device = new VulkanDevice(_physDevice.Get());

		VkPhysicalDeviceFeatures enabledFeatures{};
		if (VK_TRUE == _physDevice.GetFeatures().samplerAnisotropy)
			enabledFeatures.samplerAnisotropy = VK_TRUE;

		const std::vector<const char*> enabledExtensions{};
		VkResult res = _device->createLogicalDevice(enabledFeatures, enabledExtensions);
		if (res != VK_SUCCESS)
		{
			std::cerr << "Could not create Vulkan device!" << std::endl;
			exit(res);
		}

		_logicalDevice = _device->logicalDevice;
		vkGetDeviceQueue(_logicalDevice, _device->queueFamilyIndices.graphics, 0, &_gpuQueue);

		if (VK_FORMAT_UNDEFINED == _depthFormat)
		{
			_depthFormat = FindDepthFormat(_selectDevice);
			assert(VK_FORMAT_UNDEFINED != _depthFormat);
		}

		_swapChain = new VulkanSwapChain;
		_swapChain->connect(_instanceHandle, _selectDevice, _logicalDevice);

		return true;
	}

	void Main::ReleaseLogicalGroup()
	{
		if (nullptr != _swapChain)
		{
			_swapChain->cleanup();
			delete _swapChain;
			_swapChain = nullptr;
		}

		if (nullptr != _device)
		{
			_logicalDevice = VK_NULL_HANDLE;
			_gpuQueue = VK_NULL_HANDLE;

			delete _device;
			_device = nullptr;
		}
	}
}
