// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "renderer/renderer.h"
#include "renderer/renderer_pch.h"

namespace Main {
    void Resize(uint32_t width, uint32_t height) {
        Renderer::Resize(width, height);
    }

    bool Initialize() {
        if(false == Physical::Instance::Create()) {
            return false;
        }
#if defined(_DEBUG)
        if(false == Renderer::Debugger::Initialize(Physical::Instance::Get())) {
            return false;
        }
#endif
        if(false == Surface::Windows::Create()) {
            return false;
        }

        Physical::Device::Collect(Surface::Get());
        if(false == Logical::Device::Create()) {
            return false;
        }

        auto cmdPool = Logical::AllocateGPUCommandPool();

        if (false == Renderer::Initialize()) {
            return false;
        }

        return true;
    }

    bool Run(float delta) {
        Renderer::Run(delta);
        return true;
    }

    void Finalize() {
        Logical::Device::Destroy();
#if defined(_DEBUG)
        Renderer::Debugger::Destroy(Physical::Instance::Get());
#endif
        Physical::Instance::Destroy();

        Renderer::Release();
    }
}
