// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "RenderSceneSystem.h"

#include "VkMain.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

bool RenderSceneSystem::Initialize(const Vk::Main& main) {
    auto& device = main.GetVulkanDevice();
    auto& swapChain = main.GetVulkanSwapChain();
    const auto depthFormat = main.GetDepthFormat();
    const auto cmdPool = main.GetCommandPool();
    const auto renderPass = main.GetRenderPass();
    const auto& settings = main.GetSettings();

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

void RenderSceneSystem::Release(const Vk::Main& main) {
    const auto device = main.GetVulkanDevice().logicalDevice;
    const auto cmdPool = main.GetCommandPool();

    for(auto& commandBuffer : _commandBuffers) {
        commandBuffer.Release(device, cmdPool);
    }
    _commandBuffers.clear();

    for(auto& frameBuffer : _frameBuffers) {
        frameBuffer.Release(device);
    }
    _frameBuffers.clear();
}
