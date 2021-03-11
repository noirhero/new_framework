// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Render {
    class SwapChainFrameBuffer {
    public:
    	SwapChainFrameBuffer(std::vector<VkFramebuffer>&& buffers) : _buffers(std::move(buffers)) {}
        ~SwapChainFrameBuffer();

    private:
        std::vector<VkFramebuffer> _buffers;
    };
    using SwapChainFrameBufferUPtr = std::unique_ptr<SwapChainFrameBuffer>;

    SwapChainFrameBufferUPtr AllocateSwapChainFrameBuffer(VkRenderPass renderPass);
}

namespace Render {
    class SimpleRenderPass {
    public:
    	SimpleRenderPass(VkRenderPass handle, SwapChainFrameBufferUPtr frameBuffer) : _handle(handle), _frameBuffer(std::move(frameBuffer)) {}
        ~SimpleRenderPass();

    private:
        VkRenderPass             _handle = VK_NULL_HANDLE;
        SwapChainFrameBufferUPtr _frameBuffer;
    };
    using SimpleRenderPassUPtr = std::unique_ptr<SimpleRenderPass>;

    SimpleRenderPassUPtr AllocateSimpleRenderPass();
}
