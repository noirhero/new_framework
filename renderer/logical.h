// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

struct SwapChainInfo {
    VkSurfaceFormats              supportFormats;
    VkSurfaceCapabilitiesKHR      capabilities{};
    VkPresentModes                presentModes;
    uint32_t                      presentQueueIndex = 0;
    uint32_t                      imageCount = 0;
    uint32_t                      width = INVALID_IDX;
    uint32_t                      height = INVALID_IDX;
    VkFormat                      format = VK_FORMAT_UNDEFINED;
    VkFormat                      depthFormat = VK_FORMAT_UNDEFINED;
    VkPresentModeKHR              presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    VkSwapchainKHR                handle = VK_NULL_HANDLE;
    VkImages                      images;
    VkImageViews                  imageViews;

    VkImage                       depthImage = VK_NULL_HANDLE;
    VmaAllocation                 depthAlloc = VK_NULL_HANDLE;
    VkImageView                   depthImageView = VK_NULL_HANDLE;
};

namespace Logical::Device {
    VkDevice Get();
    VkQueue  GetGPUQueue();
    VkQueue  GetPresentQueue();

    bool     Create();
    void     Destroy();
}

namespace Logical::SwapChain {
    SwapChainInfo& Get();

    bool           Create();
    void           Destroy();
}
