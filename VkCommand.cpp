// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkCommand.h"

#include "VkUtils.h"
#include "VkMain.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

namespace Vk {
    // Command pool
    bool CommandPool::Initialize(uint32_t queueNodeIdx, VkDevice device) {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = queueNodeIdx;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK_RESULT(vkCreateCommandPool(device, &info, nullptr, &_cmdPool));

        return (VK_NULL_HANDLE == _cmdPool) ? false : true;
    }

    void CommandPool::Release(VkDevice device) {
        if (VK_NULL_HANDLE == _cmdPool)
            return;

        vkDestroyCommandPool(device, _cmdPool, nullptr);
        _cmdPool = VK_NULL_HANDLE;
    }

    // Command buffer
    bool CommandBuffer::Initialize(VkDevice device, VkCommandPool cmdPool, uint32_t count) {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = cmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = count;

        _cmdBufs.resize(count, VK_NULL_HANDLE);
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &info, _cmdBufs.data()));

        return (VK_NULL_HANDLE == _cmdBufs.front()) ? false : true;
    }

    void CommandBuffer::Release(VkDevice device, VkCommandPool cmdPool) {
        const auto count = static_cast<uint32_t>(_cmdBufs.size());
        if (0 == count)
            return;

        vkFreeCommandBuffers(device, cmdPool, count, _cmdBufs.data());
        _cmdBufs.clear();
    }

    // Frame buffer
    bool FrameBuffer::Initialize(VulkanDevice & vulkanDevice, VulkanSwapChain & swapChain, VkFormat depthFormat, VkRenderPass renderPass, const Settings& settings) {
        _isMultiSampling = settings.multiSampling;

        const auto device = vulkanDevice.logicalDevice;

        /*
        MSAA
        */
        if (true == _isMultiSampling) {
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
            VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &_multiSampleTarget.color.image));

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device, _multiSampleTarget.color.image, &memReqs);

            VkMemoryAllocateInfo memAllocInfo{};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.allocationSize = memReqs.size;

            VkBool32 lazyMemTypePresent;
            memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &lazyMemTypePresent);
            if (!lazyMemTypePresent)
                memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &_multiSampleTarget.color.memory));
            vkBindImageMemory(device, _multiSampleTarget.color.image, _multiSampleTarget.color.memory, 0);

            // Create image view for the MSAA target
            VkImageViewCreateInfo imageViewCI{};
            imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCI.image = _multiSampleTarget.color.image;
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.format = swapChain.colorFormat;
            imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCI.subresourceRange.levelCount = 1;
            imageViewCI.subresourceRange.layerCount = 1;
            VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &_multiSampleTarget.color.view));

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
            VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &_multiSampleTarget.depth.image));

            vkGetImageMemoryRequirements(device, _multiSampleTarget.depth.image, &memReqs);

            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.allocationSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &lazyMemTypePresent);
            if (!lazyMemTypePresent)
                memAllocInfo.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &_multiSampleTarget.depth.memory));
            vkBindImageMemory(device, _multiSampleTarget.depth.image, _multiSampleTarget.depth.memory, 0);

            // Create image view for the MSAA target
            imageViewCI.image = _multiSampleTarget.depth.image;
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.format = depthFormat;
            imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageViewCI.subresourceRange.levelCount = 1;
            imageViewCI.subresourceRange.layerCount = 1;
            VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &_multiSampleTarget.depth.view));
        }


        // Depth/Stencil attachment is the same for all frame buffers

        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.pNext = nullptr;
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
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

        VkImageViewCreateInfo depthStencilView = {};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.pNext = nullptr;
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
        VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &_depthStencil.image));

        vkGetImageMemoryRequirements(device, _depthStencil.image, &memReqs);
        mem_alloc.allocationSize = memReqs.size;
        mem_alloc.memoryTypeIndex = vulkanDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, nullptr, &_depthStencil.mem));
        VK_CHECK_RESULT(vkBindImageMemory(device, _depthStencil.image, _depthStencil.mem, 0));

        depthStencilView.image = _depthStencil.image;
        VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &_depthStencil.view));

        //

        VkImageView attachments[4];
        if (true == settings.multiSampling) {
            attachments[0] = _multiSampleTarget.color.view;
            attachments[2] = _multiSampleTarget.depth.view;
            attachments[3] = _depthStencil.view;
        }
        else {
            attachments[1] = _depthStencil.view;
        }

        VkFramebufferCreateInfo frameBufferCI{};
        frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCI.pNext = nullptr;
        frameBufferCI.renderPass = renderPass;
        frameBufferCI.attachmentCount = settings.multiSampling ? 4 : 2;
        frameBufferCI.pAttachments = attachments;
        frameBufferCI.width = settings.width;
        frameBufferCI.height = settings.height;
        frameBufferCI.layers = 1;

        // Create frame buffers for every swap chain image
        _frameBufs.resize(swapChain.imageCount);
        for (uint32_t i = 0; i < _frameBufs.size(); i++) {
            if (settings.multiSampling)
                attachments[1] = swapChain.buffers[i].view;
            else
                attachments[0] = swapChain.buffers[i].view;

            VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &_frameBufs[i]));
        }

        return true;
    }

    void FrameBuffer::Release(VkDevice device) {
        for (auto& frameBuffer : _frameBufs)
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        _frameBufs.clear();

        const auto deleteFn = [device](VkImage image, VkImageView view, VkDeviceMemory memory) {
            if (VK_NULL_HANDLE != memory)
                vkFreeMemory(device, memory, nullptr);

            if (VK_NULL_HANDLE != view)
                vkDestroyImageView(device, view, nullptr);

            if (VK_NULL_HANDLE != image)
                vkDestroyImage(device, image, nullptr);
        };

        deleteFn(_depthStencil.image, _depthStencil.view, _depthStencil.mem);
        _depthStencil = {};

        if (true == _isMultiSampling) {
            deleteFn(_multiSampleTarget.color.image, _multiSampleTarget.color.view, _multiSampleTarget.color.memory);
            deleteFn(_multiSampleTarget.depth.image, _multiSampleTarget.depth.view, _multiSampleTarget.depth.memory);
            _multiSampleTarget = {};
        }
    }
}
