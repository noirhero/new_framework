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
    class RenderPass {
    public:
    	RenderPass(VkRenderPass handle, SwapChainFrameBufferUPtr frameBuffer) : _handle(handle), _frameBuffer(std::move(frameBuffer)) {}
        ~RenderPass();

    private:
        VkRenderPass             _handle = VK_NULL_HANDLE;
        SwapChainFrameBufferUPtr _frameBuffer;
    };
    using RenderPassUPtr = std::unique_ptr<RenderPass>;

    RenderPassUPtr CreateSimpleRenderPass();
}
