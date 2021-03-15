// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "input.h"

namespace Input {
    enum class Type : uint8_t {
        Keyboard
    };

    using ReleaseCallbacks = std::unordered_map<uint32_t, ReleaseCallback>;
    using PressCallbacks   = std::unordered_map<uint32_t, PressCallback>;
    using InputManagerUPtr = std::unique_ptr<gainput::InputManager>;
    using InputMapUPtr     = std::unique_ptr<gainput::InputMap>;

    InputManagerUPtr       g_mng;
    InputMapUPtr           g_map;
    int32_t                g_width = 0;
    int32_t                g_height = 0;

    gainput::DeviceId      g_keyboardId = gainput::InvalidDeviceId;
    ReleaseCallbacks       g_keyboardReleaseCallbacks;
    PressCallbacks         g_keyboardPressCallbacks;

    bool Initialize() {
        g_mng = std::make_unique<gainput::InputManager>();
        g_map = std::make_unique<gainput::InputMap>(*g_mng);

        g_keyboardId = g_mng->CreateDevice<gainput::InputDeviceKeyboard>(gainput::InvalidDeviceId, gainput::InputDevice::DV_RAW);

        return true;
    }

    void Destroy() {
        g_keyboardReleaseCallbacks.clear();

        g_map.reset();
        g_mng.reset();
    }

    void Update() {
        g_mng->SetDisplaySize(g_width, g_height);
        g_mng->Update();

        for (auto& keyReleasePair : g_keyboardReleaseCallbacks) {
            const auto id = keyReleasePair.first;
            if (g_map->GetBoolWasDown(id)) {
                keyReleasePair.second(id);
            }
        }

        for (auto& keyPressPair : g_keyboardPressCallbacks) {
            const auto id = keyPressPair.first;
            if (g_map->GetBoolIsNew(id)) {
                keyPressPair.second(id);
            }
        }
    }

#if defined(_WINDOWS)
    void HandleMessage(const MSG& msg) {
        g_mng->HandleMessage(msg);
    }
#endif

    void SetDisplaySize(uint32_t width, uint32_t height) {
        g_width = static_cast<int32_t>(width);
        g_height = static_cast<int32_t>(height);
    }

    void SetKeyboardRelease(uint32_t id, gainput::Key key, ReleaseCallback callback) {
        g_keyboardReleaseCallbacks.erase(id);
        g_keyboardReleaseCallbacks.emplace(id, callback);

        g_map->MapBool(id, g_keyboardId, key);
    }

    void SetKeyboardPress(uint32_t id, gainput::Key key, PressCallback callback) {
        g_keyboardPressCallbacks.erase(id);
        g_keyboardPressCallbacks.emplace(id, callback);

        g_map->MapBool(id, g_keyboardId, key);
    }
}
