// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "renderer/renderer.h"
#include "renderer/renderer_pch.h"

namespace Main {
    Render::RenderPassUPtr    g_renderPass;
    Logical::CommandPoolUPtr  g_gpuCmdPool;

    Shader::ModuleUPtr        g_vs;
    Shader::ModuleUPtr        g_fs;

    Image::SamplerUPtr        g_sampler;
    Image::Dimension2UPtr     g_texture;
    Buffer::UniformUPtr g_ub;

    Descriptor::LayoutUPtr   g_descLayout;

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

        g_renderPass = Render::CreateSimpleRenderPass();
        g_gpuCmdPool = Logical::AllocateGPUCommandPool();

        g_vs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_vert.spv"s);
        g_fs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_frag.spv"s);

        g_sampler = Image::CreateSimpleLinearSampler();
        g_texture = Image::CreateSimple2D(Path::GetResourcePathAnsi() + "images/learning_vulkan.ktx"s, *g_gpuCmdPool);
        g_ub = Buffer::CreateSimpleUniformBuffer(sizeof(glm::mat4));

        g_descLayout = Descriptor::CreateSimpleLayout();
        g_descLayout->UpdateBegin();
        g_descLayout->AddUpdate(*g_ub);
        g_descLayout->AddUpdate(*g_sampler, *g_texture);
        g_descLayout->UpdateImmediately();

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
        g_descLayout.reset();
        g_ub.reset();
        g_texture.reset();
        g_sampler.reset();
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
