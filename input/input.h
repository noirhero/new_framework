// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Input {
    using KeyCallback   = std::function<void(uint32_t)>;
    using ValueCallback = std::function<void(uint32_t, float)>;

    bool Initialize();
    void Destroy();
    void Update();

#if defined(_WINDOWS)
    void HandleMessage(const MSG& msg);
#endif

    void SetDisplaySize(uint32_t width, uint32_t height);
}

namespace Input::Keyboard {
    void SetRelease(uint32_t id, gainput::Key key, KeyCallback callback);
    void SetPress(uint32_t id, gainput::Key key, KeyCallback callback);
    void SetDown(uint32_t id, gainput::Key key, KeyCallback callback);
}

namespace Input::Mouse {
    void SetRelease(uint32_t id, gainput::MouseButton key, KeyCallback callback);
    void SetPress(uint32_t id, gainput::MouseButton key, KeyCallback callback);
    void SetDown(uint32_t id, gainput::MouseButton key, KeyCallback callback);
    void SetDelta(uint32_t id, gainput::MouseButton key, ValueCallback callback);
}
