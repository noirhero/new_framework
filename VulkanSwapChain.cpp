// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include"stdafx.h"
#include "VulkanSwapChain.h"

#include "VkUtils.h"

namespace Vk
{
	void VulkanSwapChain::initSurface(void* platformHandle, void* platformWindow)
	{
		VkResult err = VK_SUCCESS;

		// Create the os-specific surface
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
		surfaceCreateInfo.hwnd = (HWND)platformWindow;
		err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);

		if (err != VK_SUCCESS) {
			std::cerr << "Could not Create surface!" << std::endl;
			exit(err);
		}

		// Get available queue family properties
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

		// Iterate over each queue to learn whether it supports presenting:
		// Find a queue with present support
		// Will be used to present the swap chain images to the windowing system
		std::vector<VkBool32> supportsPresent(queueCount);
		for (uint32_t i = 0; i < queueCount; i++)
		{
			fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
		}

		// Search for a graphics and a present queue in the array of queue
		// families, try to find one that supports both
		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueCount; i++)
		{
			if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				if (graphicsQueueNodeIndex == UINT32_MAX)
				{
					graphicsQueueNodeIndex = i;
				}

				if (supportsPresent[i] == VK_TRUE)
				{
					graphicsQueueNodeIndex = i;
					presentQueueNodeIndex = i;
					break;
				}
			}
		}
		if (presentQueueNodeIndex == UINT32_MAX)
		{
			// If there's no queue that supports both present and graphics
			// try to find a separate present queue
			for (uint32_t i = 0; i < queueCount; ++i)
			{
				if (supportsPresent[i] == VK_TRUE)
				{
					presentQueueNodeIndex = i;
					break;
				}
			}
		}

		// Exit if either a graphics or a presenting queue hasn't been found
		if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
			std::cerr << "Could not find a graphics and/or presenting queue!" << std::endl;
			exit(-1);
		}

		// todo : Add support for separate graphics and presenting queue
		if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
			std::cerr << "Separate graphics and presenting queues are not supported yet!" << std::endl;
			exit(-1);
		}

		queueNodeIndex = graphicsQueueNodeIndex;

		// Get list of supported surface formats
		uint32_t formatCount;
		CheckResult(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
		assert(formatCount > 0);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		CheckResult(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			colorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			bool found_B8G8R8A8_UNORM = false;
			for (auto&& surfaceFormat : surfaceFormats)
			{
				// Prefer SRGB
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
				{
					colorFormat = surfaceFormat.format;
					colorSpace = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
				//if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				//{
				//	colorFormat = surfaceFormat.format;
				//	colorSpace = surfaceFormat.colorSpace;
				//	found_B8G8R8A8_UNORM = true;
				//	break;
				//}
			}

			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if (!found_B8G8R8A8_UNORM)
			{
				colorFormat = surfaceFormats[0].format;
				colorSpace = surfaceFormats[0].colorSpace;
			}
		}

	}

    template<typename T>
    T GetInstanceProcedureAddress(VkInstance instance, const char* fnName) {
        const auto fn = vkGetInstanceProcAddr(instance, fnName);
        if (nullptr == fn)
            exit(1);

        return reinterpret_cast<T>(fn);
    }

    template<typename T>
    T GetDeviceProcedureAddress(VkDevice device, const char* fnName) {
        const auto fn = vkGetDeviceProcAddr(device, fnName);
        if (nullptr == fn)
            exit(1);

        return reinterpret_cast<T>(fn);
	}

	void VulkanSwapChain::connect(VkInstance inInstance, VkPhysicalDevice inPhysicalDevice, VkDevice inDevice)
	{
        fpGetPhysicalDeviceSurfaceSupportKHR = GetInstanceProcedureAddress<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(inInstance, "vkGetPhysicalDeviceSurfaceSupportKHR");
        fpGetPhysicalDeviceSurfaceCapabilitiesKHR = GetInstanceProcedureAddress<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(inInstance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        fpGetPhysicalDeviceSurfaceFormatsKHR = GetInstanceProcedureAddress<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(inInstance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        fpGetPhysicalDeviceSurfacePresentModesKHR = GetInstanceProcedureAddress<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(inInstance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

        fpCreateSwapchainKHR = GetDeviceProcedureAddress<PFN_vkCreateSwapchainKHR>(inDevice, "vkCreateSwapchainKHR");
        fpDestroySwapchainKHR = GetDeviceProcedureAddress<PFN_vkDestroySwapchainKHR>(inDevice, "vkDestroySwapchainKHR");
        fpGetSwapchainImagesKHR = GetDeviceProcedureAddress<PFN_vkGetSwapchainImagesKHR>(inDevice, "vkGetSwapchainImagesKHR");
        fpAcquireNextImageKHR = GetDeviceProcedureAddress<PFN_vkAcquireNextImageKHR>(inDevice, "vkAcquireNextImageKHR");
        fpQueuePresentKHR = GetDeviceProcedureAddress<PFN_vkQueuePresentKHR>(inDevice, "vkQueuePresentKHR");

		instance = inInstance;
		physicalDevice = inPhysicalDevice;
		device = inDevice;
	}

	void VulkanSwapChain::create(uint32_t *width, uint32_t *height, bool vsync)
	{
		VkSwapchainKHR oldSwapchain = swapChain;

		// Get physical device surface properties and formats
		VkSurfaceCapabilitiesKHR surfCaps;
		CheckResult(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));

		// Get available present modes
		uint32_t presentModeCount;
		CheckResult(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
		assert(presentModeCount > 0);

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		CheckResult(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

		// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
		if (surfCaps.currentExtent.width == (uint32_t)-1)
		{
			// If the surface size is undefined, the size is set to
			// the size of the images requested.
			extent.width = *width;
			extent.height = *height;
		}
		else
		{
			// If the surface size is defined, the swap chain size must match
			extent = surfCaps.currentExtent;
			*width = surfCaps.currentExtent.width;
			*height = surfCaps.currentExtent.height;
		}


		// Select a present mode for the swapchain

		// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
		// This mode waits for the vertical blank ("v-sync")
		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

		// If v-sync is not requested, try to find a mailbox mode
		// It's the lowest latency non-tearing present mode available
		if (!vsync)
		{
			for (size_t i = 0; i < presentModeCount; i++)
			{
				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}

		// Determine the number of images
		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
		{
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
		}

		// Find the transformation of the surface
		VkSurfaceTransformFlagsKHR preTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			// We prefer a non-rotated transform
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			preTransform = surfCaps.currentTransform;
		}

		// Find a supported composite alpha format (not all devices support alpha opaque)
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		// Simply select the first composite alpha format available
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		for (auto& compositeAlphaFlag : compositeAlphaFlags) {
			if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
				compositeAlpha = compositeAlphaFlag;
				break;
			};
		}

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = NULL;
		swapchainCI.surface = surface;
		swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
		swapchainCI.imageFormat = colorFormat;
		swapchainCI.imageColorSpace = colorSpace;
		swapchainCI.imageExtent = { extent.width, extent.height };
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCI.queueFamilyIndexCount = 0;
		swapchainCI.pQueueFamilyIndices = NULL;
		swapchainCI.presentMode = swapchainPresentMode;
		swapchainCI.oldSwapchain = oldSwapchain;
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		swapchainCI.clipped = VK_TRUE;
		swapchainCI.compositeAlpha = compositeAlpha;

		// Set additional usage flag for blitting from the swapchain images if supported
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, colorFormat, &formatProps);
		if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR) || (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
			swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		CheckResult(fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

		// If an existing swap chain is re-created, Destroy the old swap chain
		// This also cleans up all the presentable images
		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (uint32_t i = 0; i < imageCount; i++)
			{
				vkDestroyImageView(device, buffers[i].view, nullptr);
			}
			fpDestroySwapchainKHR(device, oldSwapchain, nullptr);
		}
		CheckResult(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

		// Get the swap chain images
		images.resize(imageCount);
		CheckResult(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

		// Get the swap chain buffers containing the image and imageview
		buffers.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
		{
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = NULL;
			colorAttachmentView.format = colorFormat;
			colorAttachmentView.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;

			buffers[i].image = images[i];

			colorAttachmentView.image = buffers[i].image;

			CheckResult(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
		}
	}

	VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex)
	{
		// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
		// With that we don't have to handle VK_NOT_READY
		return fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
	}

	VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = NULL;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;
		// Check if a wait semaphore has been specified to wait for before presenting the image
		if (waitSemaphore != VK_NULL_HANDLE)
		{
			presentInfo.pWaitSemaphores = &waitSemaphore;
			presentInfo.waitSemaphoreCount = 1;
		}
		return fpQueuePresentKHR(queue, &presentInfo);
	}

	void VulkanSwapChain::cleanup()
	{
		if (swapChain != VK_NULL_HANDLE)
		{
			for (uint32_t i = 0; i < imageCount; i++)
			{
				vkDestroyImageView(device, buffers[i].view, nullptr);
			}
		}
		if (surface != VK_NULL_HANDLE)
		{
			fpDestroySwapchainKHR(device, swapChain, nullptr);
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
		surface = VK_NULL_HANDLE;
		swapChain = VK_NULL_HANDLE;
	}
}
