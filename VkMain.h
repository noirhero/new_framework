// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkDebug.h"
#include "VkInstance.h"
#include "VkPhysicalDevice.h"
#include "VkCommand.h"
#include "VkRenderPass.h"
#include "VkPipelineCache.h"

namespace Vk {
    struct VulkanDevice;
    class VulkanSwapChain;

    struct Settings {
        bool validation = false;
        bool fullscreen = false;
        bool vsync = false;
        bool multiSampling = true;
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT;
        uint32_t width = 300;
        uint32_t height = 300;
        uint32_t renderAhead = 2;
    };

    using VkFences = std::vector<VkFence>;
    using VkSemaphores = std::vector<VkSemaphore>;

    class Main {
    public:
        bool                    Initialize();
        void                    Prepare(HINSTANCE instance, HWND window);
        void                    Release();

        void                    RecreateSwapChain();
        VkResult                AcquireNextImage(uint32_t& currentBuffer, uint32_t frameIndex);
        VkResult                QueuePresent(uint32_t currentBuffer, uint32_t frameIndex);

        Settings&               GetSettings() { return _settings; }
        const Settings&         GetSettings() const { return _settings; }

        VulkanDevice&           GetVulkanDevice() const { return *_device; }
        VulkanSwapChain&        GetVulkanSwapChain() const { return *_swapChain; }

        VkDevice                GetDevice() const { return _logicalDevice; }
        VkQueue                 GetGPUQueue() const { return _gpuQueue; }
        VkCommandPool           GetCommandPool() const { return _cmdPool.Get(); }
        VkRenderPass            GetRenderPass() const { return _renderPass.Get(); }
        VkPipelineCache         GetPipelineCache() const { return _pipelineCache.Get(); }
        constexpr VkFormat      GetDepthFormat() const { return _depthFormat; }

        const CommandBuffer&    GetCommandBuffer() const { return _cmdBufs; }
        const FrameBuffer&      GetFrameBuffer() const { return _frameBufs; }

    private:
        bool                    InitializePhysicalGroup();
        void                    ReleasePhysicalGroup();

        bool                    InitializeLogicalGroup();
        void                    ReleaseLogicalGroup();

        void                    CreateFences();
        void                    ReleaseFences();

        Settings                _settings;

        Instance                _inst;
        VkInstance              _instanceHandle = VK_NULL_HANDLE;

        Debug                   _debug;

        PhysicalDevice          _physDevice;
        VkPhysicalDevice        _selectDevice = VK_NULL_HANDLE;

        VulkanDevice*           _device = nullptr;
        VkDevice                _logicalDevice = VK_NULL_HANDLE;
        VkQueue                 _gpuQueue = VK_NULL_HANDLE;

        VkFormat                _depthFormat = VK_FORMAT_UNDEFINED;
        VulkanSwapChain*        _swapChain = nullptr;

        CommandPool             _cmdPool;
        RenderPass              _renderPass;
        PipelineCache           _pipelineCache;

        CommandBuffer           _cmdBufs;
        FrameBuffer             _frameBufs;

        VkFences                _waitFences;
        VkSemaphores            _renderCompleteSemaphores;
        VkSemaphores            _presentCompleteSemaphores;
    };
}
