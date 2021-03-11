// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Shader {
    class Module;
}

namespace Descriptor {
    class Layout;
}

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

        VkRenderPass             Get() const noexcept { return _handle; }

    private:
        VkRenderPass             _handle = VK_NULL_HANDLE;
        SwapChainFrameBufferUPtr _frameBuffer;
    };
    using RenderPassUPtr = std::unique_ptr<RenderPass>;

    RenderPassUPtr CreateSimpleRenderPass();
}

namespace Render {
    class PipelineCache {
    public:
        PipelineCache(VkPipelineCache handle) : _handle(handle) {}
        ~PipelineCache();

    private:
        VkPipelineCache _handle = VK_NULL_HANDLE;
    };
    using PipelineCacheUPtr = std::unique_ptr<PipelineCache>;
}

namespace Render {
    class PipelineLayout {
    public:
        PipelineLayout(VkPipelineLayout handle) : _handle(handle) {}
        ~PipelineLayout();

    private:
        VkPipelineLayout _handle = VK_NULL_HANDLE;
    };
    using PipelineLayoutUPtr = std::unique_ptr<PipelineLayout>;
}

namespace Render {
    class Pipeline {
    public:
        Pipeline(VkPipeline handle, PipelineLayoutUPtr layout, PipelineCacheUPtr cache) : _handle(handle), _layout(std::move(layout)), _cache(std::move(cache)) {}
        ~Pipeline();

    private:
        VkPipeline         _handle = VK_NULL_HANDLE;

        PipelineLayoutUPtr _layout;
        PipelineCacheUPtr  _cache;
    };
    using PipelineUPtr = std::unique_ptr<Pipeline>;

    PipelineUPtr CreateSimplePipeline(const Descriptor::Layout& descLayout, const Shader::Module& vs, const Shader::Module& fs, const RenderPass& renderPass);
}

namespace Render {
    
}
