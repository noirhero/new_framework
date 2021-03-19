// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "input/input.h"
#include "renderer/renderer_pch.h"
#include "viewport/viewport.h"
#include "data/resource.h"

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

    Data::Model*           g_model = nullptr;
    Buffer::UniformUPtr    g_ub;

    Descriptor::LayoutUPtr g_descLayout;
    Render::PipelineUPtr   g_pipeline;

    Viewport::Camera       g_camera;
    Viewport::Projection   g_projection;

    void KeyEvent(uint32_t id) {
        switch (id) {
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
                g_camera.RotateX(-delta);
            }
            break;
        case KEY_ROTATE_Y:
            if (g_isCameraRotate) {
                g_camera.RotateY(-delta);
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

        g_pipeline = Render::CreateVertexDeclToPipeline(g_model->mesh.vertexDecl, *g_descLayout, *g_vs, *g_fs, *g_renderPass);
        Render::FillMeshToRenderCommand(*g_renderPass, *g_gpuCmdPool, *g_pipeline, *g_descLayout, g_model->mesh);
    }

    bool Initialize() {
        // Input.
        if (false == Input::Initialize()) {
            return false;
        }
        Input::Keyboard::SetRelease(KEY_TERMINATE, gainput::KeyEscape, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_UP, gainput::KeyE, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_DOWN, gainput::KeyQ, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_LEFT, gainput::KeyA, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_RIGHT, gainput::KeyD, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_FRONT, gainput::KeyW, KeyEvent);
        Input::Keyboard::SetDown(KEY_MOVE_BACK, gainput::KeyS, KeyEvent);
        Input::Mouse::SetPress(KEY_ON_ROTATE, gainput::MouseButton1, KeyEvent);
        Input::Mouse::SetRelease(KEY_OFF_ROTATE, gainput::MouseButton1, KeyEvent);
        Input::Mouse::SetDelta(KEY_ROTATE_X, gainput::MouseAxisX, ValueEvent);
        Input::Mouse::SetDelta(KEY_ROTATE_Y, gainput::MouseAxisY, ValueEvent);

        // Renderer.
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

        // Resource.
        Data::Sampler::Initialize();
        Data::Texture2D::Initialize(*g_gpuCmdPool);

        g_vs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_vert.spv"s);
        g_fs = Shader::Create(Path::GetResourcePathAnsi() + "shaders/draw_frag.spv"s);
        g_ub = Buffer::CreateSimpleUniformBuffer(sizeof(glm::mat4));

        g_model = &GLTF::Get("cube.gltf", *g_gpuCmdPool);
        auto& material = g_model->mesh.subsets[0].material;

        g_descLayout = Descriptor::CreateSimpleLayout();
        g_descLayout->UpdateBegin();
        g_descLayout->AddUpdate(*g_ub);
        g_descLayout->AddUpdate(*material.albedoSampler , *material.albedo);
        g_descLayout->UpdateImmediately();

        g_pipeline = Render::CreateVertexDeclToPipeline(g_model->mesh.vertexDecl, *g_descLayout, *g_vs, *g_fs, *g_renderPass);
        Render::FillMeshToRenderCommand(*g_renderPass, *g_gpuCmdPool, *g_pipeline, *g_descLayout, g_model->mesh);

        g_projection.SetFieldOfView(45.0f);
        g_projection.SetZNear(0.1f);
        g_projection.SetZFar(100.0f);

        return true;
    }

    bool Run(float delta) {
        g_camera.Update(delta);

        static auto rotate = 0.0f;
        static auto seconds = 0.0f;
        seconds += delta;
        if (5.0f <= seconds)
            rotate += delta;
        const auto model = glm::rotate(glm::mat4(1.0f), rotate, glm::vec3(0.0f, 1.0f, 0.0f));

        auto& swapChain = Logical::SwapChain::Get();
        g_projection.SetScreenWidth(static_cast<float>(swapChain.width));
        g_projection.SetScreenHeight(static_cast<float>(swapChain.height));

        const auto mvp = g_projection.Get() * g_camera.Get() * model;
        g_ub->Flush({ reinterpret_cast<const uint8_t*>(&mvp), sizeof(glm::mat4) });

        Render::SimpleRenderPresent(*g_gpuCmdPool);

        return g_isFrameUpdate;
    }

    void Finalize() {
        g_pipeline.reset();
        g_descLayout.reset();
        g_ub.reset();
        GLTF::Clear();
        Data::Texture2D::Destroy();
        Data::Sampler::Destroy();
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
