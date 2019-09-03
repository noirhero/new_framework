// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	struct VulkanDevice;
	class VulkanSwapChain;

	struct MultisampleTarget
	{
		struct
		{
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		} color;
		struct
		{
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
		} depth;
	};

	struct DepthStencil
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
	};

	void SetupFrameBuffers(const Settings& settings, VulkanDevice& vulkanDevice, VulkanSwapChain& swapChain, VkFormat depthFormat, VkRenderPass renderPass);
	void ReleaseFrameBuffers(const Settings& settings, VkDevice device);

	VkFramebuffer GetFrameBuffer(uint32_t index);
}
