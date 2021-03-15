// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "input/input.h"
#include "renderer/renderer_pch.h"
#include "viewport/viewport.h"

namespace Main {
    enum KeyEventId : uint32_t {
        KEY_TERMINATE,
        KEY_MOVE_UP,
        KEY_MOVE_DOWN,
        KEY_MOVE_LEFT,
        KEY_MOVE_RIGHT,
        KEY_MOVE_FRONT,
        KEY_MOVE_BACK,
        KEY_ON_ROTATE,
        KEY_OFF_ROTATE,
        KEY_ROTATE_X,
        KEY_ROTATE_Y,
    };
    bool                   g_isFrameUpdate = true;
    bool                   g_isCameraRotate = false;

    Render::PassUPtr       g_renderPass;
    Command::PoolUPtr      g_gpuCmdPool;

    Shader::ModuleUPtr     g_vs;
    Shader::ModuleUPtr     g_fs;

    Image::SamplerUPtr     g_sampler;
    Image::Dimension2UPtr  g_texture;
    Buffer::UniformUPtr    g_ub;

    Descriptor::LayoutUPtr g_descLayout;
    Render::PipelineUPtr   g_pipeline;

    Buffer::ObjectUPtr     g_vb;
    Buffer::ObjectUPtr     g_ib;

    Viewport::Camera       g_camera;

    void KeyEvent(uint32_t id) {
        switch(id) {
        case KEY_TERMINATE:  g_isFrameUpdate = false; break;
        case KEY_MOVE_UP:    g_camera.MoveUp(); break;
        case KEY_MOVE_DOWN:  g_camera.MoveDown(); break;
        case KEY_MOVE_LEFT:  g_camera.MoveLeft(); break;
        case KEY_MOVE_RIGHT: g_camera.MoveRight(); break;
        case KEY_MOVE_FRONT: g_camera.MoveFront(); break;
        case KEY_MOVE_BACK:  g_camera.MoveBack(); break;
        case KEY_ON_ROTATE:  g_isCameraRotate = true; break;
        case KEY_OFF_ROTATE: g_isCameraRotate = false; break;
        default:;
        }
    }

    void ValueEvent(uint32_t id, float delta) {
        switch (id) {
        case KEY_ROTATE_X:
            if (g_isCameraRotate) {
                g_camera.RotateX(delta);
            }
            break;
        case KEY_ROTATE_Y:
            if (g_isCameraRotate) {
                g_camera.RotateY(delta);
            }
            break;
        default:;
        }
    }

    void Resize(uint32_t width, uint32_t height) {
        Input::SetDisplaySize(width, height);

        if (VK_NULL_HANDLE == Physical::Instance::Get()) {
            return;
        }

        vkDeviceWaitIdle(Logical::Device::Get());

        g_pipeline.reset();
        g_gpuCmdPool.reset();
        g_renderPass.reset();
        Logical::SwapChain::Destroy();

        BOOL_CHECK(Logical::SwapChain::Create());
        g_renderPass = Render::CreateSimpleRenderPass();
        g_gpuCmdPool = Command::AllocateGPUCommandPool();
        g_pipeline = Render::CreateSimplePipeline(*g_descLayout, *g_vs, *g_fs, *g_renderPass);

        Render::FillSimpleRenderCommand(*g_renderPass, *g_gpuCmdPool, *g_pipeline, *g_descLayout, *g_vb, *g_ib);
    }

    bool Initialize() {
        if (false == Input::Initialize()) {
            return false;
        }
        Input::Keyboard::SetRelease(KEY_TERMINATE, gainput::KeyEscape, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_UP, gainput::KeyQ, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_DOWN, gainput::KeyE, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_LEFT, gainput::KeyA, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_RIGHT, gainput::KeyD, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_FRONT, gainput::KeyW, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_BACK, gainput::KeyS, KeyEvent);
        Input::Mouse::SetRelease(KEY_ON_ROTATE, gainput::MouseButton1, KeyEvent);
        Input::Mouse::SetDelta(KEY_ROTATE_X, gainput::MouseAxisX, ValueEvent);
        Input::Mouse::SetDelta(KEY_ROTATE_Y, gainput::MouseAxisY, ValueEvent);

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
        g_gpuCmdPool = Command::AllocateGPUCommandPool();

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

        struct Vertex {
            float x, y, z, w;
            float r, g, b, a;
            float u, v;
        };
        constexpr Vertex vertices[] = {
            { -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
            {  0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
            {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f },
            { -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
        };
        g_vb = Buffer::CreateObject(
            { (int64_t*)vertices, _countof(vertices) * sizeof(Vertex) },
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
            *g_gpuCmdPool
        );

        constexpr uint16_t indices[] = { 0, 3, 1, 3, 2, 1 };
        g_ib = Buffer::CreateObject(
            { (int64_t*)indices, _countof(indices) * sizeof(uint16_t) },
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
            *g_gpuCmdPool
        );

        Render::FillSimpleRenderCommand(*g_renderPass, *g_gpuCmdPool, *g_pipeline, *g_descLayout, *g_vb, *g_ib);

        return true;
    }

    bool Run(float delta) {
        Input::Update();

        g_camera.Update(delta);

        auto& swapChain = Logical::SwapChain::Get();
        Viewport::SetScreenWidth(static_cast<float>(swapChain.width));
        Viewport::SetScreenHeight(static_cast<float>(swapChain.height));
        Viewport::SetFieldOfView(45.0f);
        Viewport::SetZNear(0.1f);
        Viewport::SetZFar(100.0f);

        const auto projection = Viewport::GetProjection();
        const auto view = g_camera.Get();

        static auto rotate = 0.0f;
        static auto seconds = 0.0f;
        seconds += delta;
        if (10.0f <= seconds)
            rotate += delta;

        const auto model = glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0.0f, 1.0f, 0.0f));
        const auto mvp = projection * view * model;
        g_ub->Flush({ (int64_t*)&mvp, sizeof(glm::mat4) });

        Render::SimpleRenderPresent(*g_gpuCmdPool);

        return g_isFrameUpdate;
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

        Input::Destroy();
    }
}
