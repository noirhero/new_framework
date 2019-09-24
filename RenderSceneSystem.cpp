// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "RenderSceneSystem.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

bool RenderSceneSystem::Initialize(HINSTANCE instance, HWND handle) {
    if (false == _main.Initialize()) {
        return false;
    }
    _main.Prepare(instance, handle);

    auto& device = _main.GetVulkanDevice();
    auto& swapChain = _main.GetVulkanSwapChain();
    const auto depthFormat = _main.GetDepthFormat();
    const auto cmdPool = _main.GetCommandPool();
    const auto renderPass = _main.GetRenderPass();
    const auto& settings = _main.GetSettings();

    const auto numCpu = std::thread::hardware_concurrency();
    for (std::remove_const<decltype(numCpu)>::type i = 0; i < numCpu; ++i) {
        if(false == _frameBuffers.emplace_back().Initialize(device, swapChain, depthFormat, renderPass, settings)) {
            return false;
        }

        if(false == _commandBuffers.emplace_back().Initialize(device.logicalDevice, cmdPool, swapChain.imageCount)) {
            return false;
        }
    }

    return true;
}

void RenderSceneSystem::Release() {
    const auto device = _main.GetVulkanDevice().logicalDevice;
    const auto cmdPool = _main.GetCommandPool();

    for(auto& commandBuffer : _commandBuffers) {
        commandBuffer.Release(device, cmdPool);
    }

    for(auto& frameBuffer : _frameBuffers) {
        frameBuffer.Release(device);
    }

    _main.Release();
}

