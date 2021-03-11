// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "renderer/renderer.h"
#include "renderer/renderer_pch.h"

namespace Main {
    Render::SimpleRenderPassUPtr g_renderPass;
    Logical::CommandPoolUPtr     g_gpuCmdPool;

    Shader::ModuleUPtr           g_vs;
    Shader::ModuleUPtr           g_fs;

    void Resize(uint32_t /*width*/, uint32_t /*height*/) {
        //Renderer::Resize(width, height);
    }

    bool Initialize() {
        if (false == Physical::Instance::Create()) {
            return false;
        }
#if defined(_DEBUG)
        if (false == Renderer::Debugger::Initialize(Physical::Instance::Get())) {
            return false;
        }
#endif
        if (false == Surface::Windows::Create()) {
            return false;
        }

        Physical::Device::Collect(Surface::Get());
        if (false == Logical::Device::Create()) {
            return false;
        }

        if (false == Logical::SwapChain::Create()) {
            return false;
        }

        g_renderPass = Render::AllocateSimpleRenderPass();
        g_gpuCmdPool = Logical::AllocateGPUCommandPool();

        g_vs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_vert.spv"s);
        g_fs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_frag.spv"s);;

        //if (false == Renderer::Initialize()) {
        //    return false;
        //}

        return true;
    }

    bool Run(float /*delta*/) {
        //Renderer::Run(delta);
        return true;
    }

    void Finalize() {
        g_fs.reset();
        g_vs.reset();
        g_gpuCmdPool.reset();
        g_renderPass.reset();
        Logical::SwapChain::Destroy();
        Logical::Device::Destroy();
        Surface::Destroy();
#if defined(_DEBUG)
        Renderer::Debugger::Destroy(Physical::Instance::Get());
#endif
        Physical::Instance::Destroy();

        //Renderer::Release();
    }
}
