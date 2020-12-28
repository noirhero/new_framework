// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#include "pch.h"
#include "main.h"

#include "Renderer/renderer.h"

namespace Main {
    void Resize(uint32_t /*width*/, uint32_t /*height*/) {
        // Todo : Implement.
    }

    bool Initialize() {
        if(false == Renderer::Initialize()) {
            return false;
        }

        return true;
    }

    bool Run() {
        return true;
    }

    void Finalize() {
        Renderer::Release();
    }
}
