// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkMain.h"

#include "VkCommon.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

namespace Vk
{
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

	bool Main::Initialize()
	{
		if (false == InitializePhysicalGroup())
			return false;

		if (false == InitializeLogicalGroup())
			return false;

		return true;
	}

	void Main::Prepare(HINSTANCE instance, HWND window)
	{
		_swapChain->initSurface(instance, window);
		_swapChain->create(&_settings.width, &_settings.height, _settings.vsync);

		_cmdPool.Initialize(_swapChain->queueNodeIndex, _logicalDevice);
		_renderPass.Initialize(_swapChain->colorFormat, _depthFormat, _logicalDevice, _settings.multiSampling, _settings.sampleCount);
		_pipelineCache.Initialize(_logicalDevice);

		_frameBufs.Initialize(*_device, *_swapChain, _depthFormat, _renderPass.Get(), _settings);
		_cmdBufs.Initialize(_logicalDevice, _cmdPool.Get(), _swapChain->imageCount);

		CreateFences();
	}

	void Main::Release()
	{
		ReleaseFences();

		_cmdBufs.Release(_logicalDevice, _cmdPool.Get());
		_frameBufs.Release(_logicalDevice);

		_pipelineCache.Release(_logicalDevice);
		_renderPass.Release(_logicalDevice);
		_cmdPool.Release(_logicalDevice);

		ReleaseLogicalGroup();
		ReleasePhysicalGroup();
	}

	void Main::RecreateSwapChain()
	{
		_swapChain->create(&_settings.width, &_settings.height, _settings.vsync);

		_frameBufs.Release(_logicalDevice);
		_frameBufs.Initialize(*_device, *_swapChain, _depthFormat, _renderPass.Get(), _settings);
	}

	VkResult Main::AcquireNextImage(uint32_t & currentBuffer, uint32_t frameIndex)
	{
		VK_CHECK_RESULT(vkWaitForFences(_logicalDevice, 1, &_waitFences[frameIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(_logicalDevice, 1, &_waitFences[frameIndex]));

		return _swapChain->acquireNextImage(_presentCompleteSemaphores[frameIndex], &currentBuffer);
	}

	VkResult Main::QueuePresent(uint32_t currentBuffer, uint32_t frameIndex)
	{
		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.pWaitSemaphores = &_presentCompleteSemaphores[frameIndex];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &_renderCompleteSemaphores[frameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pCommandBuffers = &_cmdBufs.GetPtr()[currentBuffer];
		submitInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkQueueSubmit(_gpuQueue, 1, &submitInfo, _waitFences[frameIndex]));

		return _swapChain->queuePresent(_gpuQueue, currentBuffer, _renderCompleteSemaphores[frameIndex]);
	}

	bool Main::InitializePhysicalGroup()
	{
		if (false == _inst.Initialize(_settings.validation))
			return false;

		_instanceHandle = _inst.Get();

		if (true == _settings.validation)
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

	void Main::CreateFences()
	{
		_waitFences.resize(_settings.renderAhead);
		for (auto &waitFence : _waitFences)
		{
			VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
			VK_CHECK_RESULT(vkCreateFence(_logicalDevice, &fenceCI, nullptr, &waitFence));
		}

		_presentCompleteSemaphores.resize(_settings.renderAhead);
		for (auto &semaphore : _presentCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(_logicalDevice, &semaphoreCI, nullptr, &semaphore));
		}

		_renderCompleteSemaphores.resize(_settings.renderAhead);
		for (auto &semaphore : _renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(_logicalDevice, &semaphoreCI, nullptr, &semaphore));
		}
	}

	void Main::ReleaseFences()
	{
		for (auto fence : _waitFences)
			vkDestroyFence(_logicalDevice, fence, nullptr);

		for (auto semaphore : _renderCompleteSemaphores)
			vkDestroySemaphore(_logicalDevice, semaphore, nullptr);

		for (auto semaphore : _presentCompleteSemaphores)
			vkDestroySemaphore(_logicalDevice, semaphore, nullptr);
	}
}
