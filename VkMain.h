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

	using VkFences = std::vector<VkFence>;
	using VkSemaphores = std::vector<VkSemaphore>;

	class Main
	{
	public:
		bool					Initialize(const Settings& settings);
		void					Prepare(Settings& settings, HINSTANCE instance, HWND window);
		void					Release(const Settings& settings);

		void					RecreateSwapChain(Settings& settings);
		VkResult				AcquireNextImage(uint32_t& currentBuffer, uint32_t frameIndex);
		VkResult				QueuePresent(uint32_t currentBuffer, uint32_t frameIndex);

		VulkanDevice&			GetVulkanDevice() const { return *_device; }
		VulkanSwapChain&		GetVulkanSwapChain() const { return *_swapChain; }

		VkDevice				GetDevice() const { return _logicalDevice; }
		VkQueue					GetGPUQueue() const { return _gpuQueue; }
		VkRenderPass			GetRenderPass() const { return _renderPass.Get(); }
		VkPipelineCache			GetPipelineCache() const { return _pipelineCache.Get(); }

		CommandBuffer&			GetCommandBuffer() { return _cmdBufs; }
		FrameBuffer&			GetFrameBuffer() { return _frameBufs; }

	private:
		bool					InitializePhysicalGroup(const Settings& settings);
		void					ReleasePhysicalGroup();

		bool					InitializeLogicalGroup();
		void					ReleaseLogicalGroup();

		void					CreateFences(const Settings& settings);
		void					ReleaseFences();

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

		CommandBuffer			_cmdBufs;
		FrameBuffer				_frameBufs;

		VkFences				_waitFences;
		VkSemaphores			_renderCompleteSemaphores;
		VkSemaphores			_presentCompleteSemaphores;
	};
}
