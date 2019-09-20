// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	struct SwapChainBuffer
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
	};

	class VulkanSwapChain
	{
	private:
		VkInstance instance = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		// Function pointers
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
		PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR = nullptr;
		PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR = nullptr;
		PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR = nullptr;
		PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR = nullptr;
		PFN_vkQueuePresentKHR fpQueuePresentKHR = nullptr;

	public:
		VkFormat colorFormat;
		VkColorSpaceKHR colorSpace;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		uint32_t imageCount = 0;
		std::vector<VkImage> images;
		std::vector<SwapChainBuffer> buffers;
		VkExtent2D extent = {};
		uint32_t queueNodeIndex = UINT32_MAX;

		/** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */
		void initSurface(void* platformHandle, void* platformWindow);

		/**
		* Set inInstance, physical and logical inDevice to use for the swapchain and get all required function pointers
		*
		* @param inInstance Vulkan inInstance to use
		* @param inPhysicalDevice Physical inDevice used to query properties and formats relevant to the swapchain
		* @param inDevice Logical representation of the inDevice to Create the swapchain for
		*
		*/
		void connect(VkInstance inInstance, VkPhysicalDevice inPhysicalDevice, VkDevice inDevice);

		/**
		* Create the swapchain and get it's images with given width and height
		*
		* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
		* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
		* @param vsync (Optional) Can be used to force vsync'd rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
		*/
		void create(uint32_t *width, uint32_t *height, bool vsync = false);

		/**
		* Acquires the next image in the swap chain
		*
		* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
		* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
		*
		* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
		*
		* @return VkResult of the image acquisition
		*/
		VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex);

		/**
		* Queue an image for presentation
		*
		* @param queue Presentation queue for presenting the image
		* @param imageIndex Index of the swapchain image to queue for presentation
		* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
		*
		* @return VkResult of the queue presentation
		*/
		VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);


		/**
		* Destroy and free Vulkan resources used for the swapchain
		*/
		void cleanup();
	};
}
