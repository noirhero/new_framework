// Copyright 2018-202 TAP, Inc. All Rights Reserved.

#pragma once

namespace Main {
    void Resize(uint32_t width, uint32_t height);

    bool Initialize();
    bool Run(float delta);
    void Finalize();
}
