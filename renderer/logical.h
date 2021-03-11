// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

struct SwapChainInfo {
    std::vector<VkSurfaceFormatKHR> supportFormats;
    VkSurfaceCapabilitiesKHR        capabilities{};
    std::vector<VkPresentModeKHR>   presentModes;
    uint32_t                        presentQueueIndex = 0;
    uint32_t                        imageCount = 0;
    uint32_t                        width = std::numeric_limits<uint32_t>::max();
    uint32_t                        height = std::numeric_limits<uint32_t>::max();
    VkFormat                        format = VK_FORMAT_UNDEFINED;
    VkFormat                        depthFormat = VK_FORMAT_UNDEFINED;
    VkPresentModeKHR                presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkSurfaceTransformFlagBitsKHR   preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    VkSwapchainKHR                  handle = VK_NULL_HANDLE;
    std::vector<VkImage>            images;
    std::vector<VkImageView>        imageViews;

    VkImage                         depthImage = VK_NULL_HANDLE;
    VmaAllocation                   depthAlloc = VK_NULL_HANDLE;
    VkImageView                     depthImageView = VK_NULL_HANDLE;
};

namespace Logical::Device {
    VkDevice Get();

    bool     Create();
    void     Destroy();
}

namespace Logical::SwapChain {
    SwapChainInfo& Get();

    bool           Create();
    void           Destroy();
}

namespace Logical {
    class CommandPool {
    public:
        CommandPool(VkCommandPool cmdPool) : _handle(cmdPool) {}
        ~CommandPool();

    private:
        VkCommandPool _handle = VK_NULL_HANDLE;
    };
    using CommandPoolUPtr = std::unique_ptr<CommandPool>;

    CommandPoolUPtr AllocateGPUCommandPool();
}
