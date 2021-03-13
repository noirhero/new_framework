// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "command.h"

#include "allocator_cpu.h"
#include "physical.h"
#include "logical.h"

namespace Command {
    using namespace Renderer;

    // Command buffer.
    Buffer::~Buffer() {
        if (VK_NULL_HANDLE != _buffer) {
            vkFreeCommandBuffers(Logical::Device::Get(), _cmdPool, 1, &_buffer);
        }
    }

    // Command pool.
    Pool::~Pool() {
        auto* device = Logical::Device::Get();

        _immediatelyCmdBuf.reset();

        for (auto* cmdBuf : _swapChainFrameCmdBuffers) {
            vkFreeCommandBuffers(device, _handle, 1, &cmdBuf);
        }

        if (VK_NULL_HANDLE != _handle) {
            vkDestroyCommandPool(device, _handle, Allocator::CPU());
        }
    }

    VkCommandBuffer Pool::ImmediatelyBegin() {
        if (nullptr == _immediatelyCmdBuf) {
            VkCommandBufferAllocateInfo cmdBufInfo{};
            cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufInfo.commandPool = _handle;
            cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufInfo.commandBufferCount = 1;

            VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
            VK_CHECK(vkAllocateCommandBuffers(Logical::Device::Get(), &cmdBufInfo, &cmdBuf));

            _immediatelyCmdBuf = std::make_unique<Buffer>(_handle, cmdBuf);
        }

        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(_immediatelyCmdBuf->Get(), &cmdBufBeginInfo));

        return _immediatelyCmdBuf->Get();
    }

    void Pool::ImmediatelyEndAndSubmit() {
        if (nullptr == _immediatelyCmdBuf) {
            return;
        }

        auto* gpuQueue = Logical::Device::GetGPUQueue();

        vkEndCommandBuffer(_immediatelyCmdBuf->Get());

        const VkCommandBuffer cmdBuffers[1] = { _immediatelyCmdBuf->Get() };
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = cmdBuffers;

        vkQueueSubmit(gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpuQueue);
    }

    VkCommandBuffers Pool::GetSwapChainFrameCommandBuffers() {
        auto* device = Logical::Device::Get();
        auto& swapChain = Logical::SwapChain::Get();

        if (_swapChainFrameCmdBuffers.empty()) {
            for (decltype(swapChain.imageCount) i = 0; i < swapChain.imageCount; ++i) {
                VkCommandBufferAllocateInfo cmdInfo{};
                cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                cmdInfo.commandPool = _handle;
                cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmdInfo.commandBufferCount = 1;

                VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
                VK_CHECK(vkAllocateCommandBuffers(device, &cmdInfo, &cmdBuffer));
                _swapChainFrameCmdBuffers.emplace_back(cmdBuffer);
            }
        }

        return _swapChainFrameCmdBuffers;
    }

    PoolUPtr AllocateGPUCommandPool() {
        auto& physicalDevice = Physical::Device::GetGPU();

        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = physicalDevice.gpuQueueIndex;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool handle = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateCommandPool(Logical::Device::Get(), &info, Allocator::CPU(), &handle)) {
            return nullptr;
        }

        return std::make_unique<Pool>(handle);
    }
}
