// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Logical {
    class CommandPool;
}

namespace Shader {
    class Module;
}

namespace Descriptor {
    class Layout;
}

namespace Buffer {
    class Object;
}

namespace Render {
    class SwapChainFrameBuffer {
    public:
        SwapChainFrameBuffer(VkFrameBuffers&& buffers) : _buffers(std::move(buffers)) {}
        ~SwapChainFrameBuffer();

        VkFrameBuffers Get() const noexcept { return _buffers; }

    private:
        VkFrameBuffers _buffers;
    };
    using SwapChainFrameBufferUPtr = std::unique_ptr<SwapChainFrameBuffer>;

    SwapChainFrameBufferUPtr AllocateSwapChainFrameBuffer(VkRenderPass renderPass);
}

namespace Render {
    class Pass {
    public:
        Pass(VkRenderPass handle, SwapChainFrameBufferUPtr frameBuffer) : _handle(handle), _frameBuffer(std::move(frameBuffer)) {}
        ~Pass();

        VkRenderPass             Get() const noexcept { return _handle; }
        VkFrameBuffers           GetFrameBuffers() const noexcept;

    private:
        VkRenderPass             _handle = VK_NULL_HANDLE;
        SwapChainFrameBufferUPtr _frameBuffer;
    };
    using PassUPtr = std::unique_ptr<Pass>;

    PassUPtr CreateSimpleRenderPass();
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

        VkPipelineLayout Get() const noexcept { return _handle; }

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

        VkPipeline         Get() const noexcept { return _handle; }
        VkPipelineLayout   GetLayout() const noexcept;

    private:
        VkPipeline         _handle = VK_NULL_HANDLE;

        PipelineLayoutUPtr _layout;
        PipelineCacheUPtr  _cache;
    };
    using PipelineUPtr = std::unique_ptr<Pipeline>;

    PipelineUPtr CreateSimplePipeline(const Descriptor::Layout& descLayout, const Shader::Module& vs, const Shader::Module& fs, const Pass& renderPass);
}

namespace Render {
    void FillSimpleRenderCommand(Pass& renderPass, Logical::CommandPool& cmdPool, Pipeline& pipeline, Descriptor::Layout& descLayout, Buffer::Object& vb, Buffer::Object& ib);
    void SimpleRenderPresent(Logical::CommandPool& cmdPool);
}
