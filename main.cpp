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
    Buffer::UniformUPtr       g_ub;

    Descriptor::LayoutUPtr   g_descLayout;
    Render::PipelineUPtr     g_pipeline;

    Buffer::ObjectUPtr       g_vb;
    Buffer::ObjectUPtr       g_ib;

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

        g_pipeline = Render::CreateSimplePipeline(*g_descLayout, *g_vs, *g_fs, *g_renderPass);

        struct VertexWithColor {
            float x, y, z, w;
            float r, g, b, a;
            float u, v;
        };
        constexpr VertexWithColor vertices[] = {
            { -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
            {  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
            {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
            { -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
        };
        g_vb = Buffer::CreateObject(
            { (int64_t*)vertices, _countof(vertices) * sizeof(VertexWithColor) }, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 
            *g_gpuCmdPool
        );

        constexpr uint16_t indices[] = { 0, 1, 3, 3, 1, 2 };
        g_ib = Buffer::CreateObject(
            { (int64_t*)indices, _countof(indices) * sizeof(uint16_t) },
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
            *g_gpuCmdPool
        );

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
        g_ib.reset();
        g_vb.reset();
        g_pipeline.reset();
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
