// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

using VkCommandBuffers = std::vector<VkCommandBuffer>;
using VkFrameBuffers = std::vector<VkFramebuffer>;

namespace Vk
{
	struct VulkanDevice;
	class VulkanSwapChain;

	struct Target
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct DepthStencil
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
	};

	struct MultisampleTarget
	{
		Target color;
		Target depth;
	};

	class CommandPool
	{
	public:
		bool					Initialize(uint32_t queueNodeIdx, VkDevice device);
		void					Release(VkDevice device);

		VkCommandPool			Get() const { return _cmdPool; }

	private:
		VkCommandPool			_cmdPool = VK_NULL_HANDLE;
	};

	class CommandBuffer
	{
	public:
		bool					Initialize(VkDevice device, VkCommandPool cmdPool, uint32_t count);
		void					Release(VkDevice device, VkCommandPool cmdPool);

		uint32_t				Count() const { return static_cast<uint32_t>(_cmdBufs.size()); }
		VkCommandBuffer			Get(uint32_t i) const { return _cmdBufs[i]; }
		const VkCommandBuffer*	GetPtr() const { return _cmdBufs.data(); }

	private:
		VkCommandBuffers		_cmdBufs;
	};

	class FrameBuffer
	{
	public:
		bool					Initialize(const Settings& settings, VulkanDevice& vulkanDevice, VulkanSwapChain& swapChain, VkFormat depthFormat, VkRenderPass renderPass);
		void					Release(const Settings& settings, VkDevice device);

		VkFramebuffer			Get(uint32_t i) const { return _frameBufs[i]; }

	private:
		MultisampleTarget		_multiSampleTarget;
		DepthStencil			_depthStencil;
		VkFrameBuffers			_frameBufs;
	};
}
