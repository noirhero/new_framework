// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Command {
    class Buffer {
    public:
        Buffer(VkCommandPool cmdPool, VkCommandBuffer buffer) : _cmdPool(cmdPool), _buffer(buffer) {}
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        VkCommandBuffer Get() const noexcept { return _buffer; }

    private:
        VkCommandPool   _cmdPool = VK_NULL_HANDLE;
        VkCommandBuffer _buffer = VK_NULL_HANDLE;
    };
    using BufferUPtr = std::unique_ptr<Buffer>;
}

namespace Command {
    class Pool {
    public:
        Pool(VkCommandPool cmdPool) : _handle(cmdPool) {}
        ~Pool();

        Pool(const Pool&) = delete;
        Pool& operator=(const Pool&) = delete;

        VkCommandBuffer  ImmediatelyBegin();
        void             ImmediatelyEndAndSubmit();

        VkCommandBuffers GetSwapChainFrameCommandBuffers();

    private:
        VkCommandPool    _handle = VK_NULL_HANDLE;

        BufferUPtr       _immediatelyCmdBuf;
        VkCommandBuffers _swapChainFrameCmdBuffers;
    };
    using PoolUPtr = std::unique_ptr<Pool>;

    PoolUPtr AllocateGPUCommandPool();
}
