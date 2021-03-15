// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Input {
    using ReleaseCallback = std::function<void(uint32_t)>;
    using PressCallback = std::function<void(uint32_t)>;

    bool Initialize();
    void Destroy();
    void Update();

#if defined(_WINDOWS)
    void HandleMessage(const MSG& msg);
#endif

    void SetDisplaySize(uint32_t width, uint32_t height);

    void SetKeyboardRelease(uint32_t id, gainput::Key key, ReleaseCallback callback);
    void SetKeyboardPress(uint32_t id, gainput::Key key, PressCallback callback);
}
