// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkFrameBuffer.h"

#include "VkDevice.h"
#include "VkSwapChain.h"

namespace Vk
{
	MultisampleTarget multisampleTarget;
	DepthStencil depthStencil;
	std::vector<VkFramebuffer>frameBuffers;

	void SetupFrameBuffers(const Settings& settings, VulkanDevice& vulkanDevice, VulkanSwapChain& swapChain, VkFormat depthFormat, VkRenderPass renderPass)
	{
		VkDevice device = vulkanDevice.logicalDevice;

		/*
		MSAA
		*/
		if (true == settings.multiSampling)
		{
			// Check if device supports requested sample count for color and depth frame buffer
			//assert((deviceProperties.limits.framebufferColorSampleCounts >= sampleCount) && (deviceProperties.limits.framebufferDepthSampleCounts >= sampleCount));

			VkImageCreateInfo imageCI{};
			imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = swapChain.colorFormat;
			imageCI.extent.width = settings.width;
			imageCI.extent.height = settings.height;
			imageCI.extent.depth = 1;
			imageCI.mipLevels = 1;
			imageCI.arrayLayers = 1;
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCI.samples = settings.sampleCount;
			imageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &multisampleTarget.color.image));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, multisampleTarget.color.image, &memReqs);
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memReqs.size;
			VkBool32 lazyMemTypePresent;
			memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &lazyMemTypePresent);
			if (!lazyMemTypePresent) {
				memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			}
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &multisampleTarget.color.memory));
			vkBindImageMemory(device, multisampleTarget.color.image, multisampleTarget.color.memory, 0);

			// Create image view for the MSAA target
			VkImageViewCreateInfo imageViewCI{};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.image = multisampleTarget.color.image;
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.format = swapChain.colorFormat;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.layerCount = 1;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &multisampleTarget.color.view));

			// Depth target
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = depthFormat;
			imageCI.extent.width = settings.width;
			imageCI.extent.height = settings.height;
			imageCI.extent.depth = 1;
			imageCI.mipLevels = 1;
			imageCI.arrayLayers = 1;
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCI.samples = settings.sampleCount;
			imageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &multisampleTarget.depth.image));

			vkGetImageMemoryRequirements(device, multisampleTarget.depth.image, &memReqs);
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &lazyMemTypePresent);
			if (!lazyMemTypePresent) {
				memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			}
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &multisampleTarget.depth.memory));
			vkBindImageMemory(device, multisampleTarget.depth.image, multisampleTarget.depth.memory, 0);

			// Create image view for the MSAA target
			imageViewCI.image = multisampleTarget.depth.image;
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.format = depthFormat;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.layerCount = 1;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &multisampleTarget.depth.view));
		}


		// Depth/Stencil attachment is the same for all frame buffers

		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = NULL;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = depthFormat;
		image.extent = { settings.width, settings.height, 1 };
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo mem_alloc = {};
		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = 0;
		mem_alloc.memoryTypeIndex = 0;

		VkImageViewCreateInfo depthStencilView = {};
		depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilView.pNext = NULL;
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = depthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		VkMemoryRequirements memReqs;
		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthStencil.image));
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		mem_alloc.allocationSize = memReqs.size;
		mem_alloc.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

		depthStencilView.image = depthStencil.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view));

		//

		VkImageView attachments[4];
		if (true == settings.multiSampling)
		{
			attachments[0] = multisampleTarget.color.view;
			attachments[2] = multisampleTarget.depth.view;
			attachments[3] = depthStencil.view;
		}
		else
		{
			attachments[1] = depthStencil.view;
		}

		VkFramebufferCreateInfo frameBufferCI{};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.pNext = NULL;
		frameBufferCI.renderPass = renderPass;
		frameBufferCI.attachmentCount = settings.multiSampling ? 4 : 2;
		frameBufferCI.pAttachments = attachments;
		frameBufferCI.width = settings.width;
		frameBufferCI.height = settings.height;
		frameBufferCI.layers = 1;

		// Create frame buffers for every swap chain image
		frameBuffers.resize(swapChain.imageCount);
		for (uint32_t i = 0; i < frameBuffers.size(); i++) {
			if (settings.multiSampling) {
				attachments[1] = swapChain.buffers[i].view;
			}
			else {
				attachments[0] = swapChain.buffers[i].view;
			}
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffers[i]));
		}
	}

	void ReleaseFrameBuffers(const Settings& settings, VkDevice device)
	{
		for (uint32_t i = 0; i < frameBuffers.size(); i++)
			vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		frameBuffers.clear();

		vkDestroyImageView(device, depthStencil.view, nullptr);
		vkDestroyImage(device, depthStencil.image, nullptr);
		vkFreeMemory(device, depthStencil.mem, nullptr);

		if (true == settings.multiSampling)
		{
			vkDestroyImage(device, multisampleTarget.color.image, nullptr);
			vkDestroyImageView(device, multisampleTarget.color.view, nullptr);
			vkFreeMemory(device, multisampleTarget.color.memory, nullptr);
			vkDestroyImage(device, multisampleTarget.depth.image, nullptr);
			vkDestroyImageView(device, multisampleTarget.depth.view, nullptr);
			vkFreeMemory(device, multisampleTarget.depth.memory, nullptr);
		}
	}

	VkFramebuffer GetFrameBuffer(uint32_t index)
	{
		return frameBuffers[index];
	}
}
