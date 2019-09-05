// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkDebug.h"
#include "VkInstance.h"
#include "VkPhysicalDevice.h"
#include "VkCommand.h"
#include "VkRenderPass.h"
#include "VkPipelineCache.h"

namespace Vk
{
	struct VulkanDevice;
	class VulkanSwapChain;

	class Main
	{
	public:
		bool					Initialize(Settings& settings);
		void					Prepare(Settings& settings, HINSTANCE instance, HWND window);
		void					Release();

		void					RecreateSwapChain(Settings& settings);
		VkResult				AcquireNextImage(uint32_t& currentBuffer, VkSemaphore semaphore = VK_NULL_HANDLE);
		VkResult				QueuePresent(uint32_t currentBuffer, VkSemaphore semaphore = VK_NULL_HANDLE);

		VulkanDevice&			GetVulkanDevice() const { return *_device; }

		VulkanSwapChain&		GetVulkanSwapChain() const { return *_swapChain; }
		uint32_t				GetSwapChainImageCount() const;

		VkInstance				GetInst() const { return _instanceHandle; }
		VkPhysicalDevice		GetPhysicalDevice() const { return _selectDevice; }
		VkDevice				GetDevice() const { return _logicalDevice; }
		VkQueue					GetGPUQueue() const { return _gpuQueue; }
		VkFormat				GetDepthFormat() const { return _depthFormat; }
		VkCommandPool			GetCommandPool() const { return _cmdPool.Get(); }
		VkRenderPass			GetRenderPass() const { return _renderPass.Get(); }
		VkPipelineCache			GetPipelineCache() const { return _pipelineCache.Get(); }

	private:
		bool					InitializePhysicalGroup(Settings& settings);
		void					ReleasePhysicalGroup();

		bool					InitializeLogicalGroup();
		void					ReleaseLogicalGroup();

		Instance				_inst;
		VkInstance				_instanceHandle = VK_NULL_HANDLE;

		Debug					_debug;

		PhysicalDevice			_physDevice;
		VkPhysicalDevice		_selectDevice = VK_NULL_HANDLE;

		VulkanDevice*			_device = nullptr;
		VkDevice				_logicalDevice = VK_NULL_HANDLE;
		VkQueue					_gpuQueue = VK_NULL_HANDLE;

		VkFormat				_depthFormat = VK_FORMAT_UNDEFINED;
		VulkanSwapChain*		_swapChain = nullptr;

		CommandPool				_cmdPool;
		RenderPass				_renderPass;
		PipelineCache			_pipelineCache;
	};
}
