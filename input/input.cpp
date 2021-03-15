// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "input.h"

namespace Input {
    enum class Type : uint8_t {
        Keyboard
    };

    using ReleaseCallbacks = std::unordered_map<uint32_t, ReleaseCallback>;
    using PressCallbacks   = std::unordered_map<uint32_t, PressCallback>;
    using DownCallbacks    = std::unordered_map<uint32_t, DownCallback>;
    using DeltaCallbacks   = std::unordered_map<uint32_t, ValueCallback>;
    using InputManagerUPtr = std::unique_ptr<gainput::InputManager>;
    using InputMapUPtr     = std::unique_ptr<gainput::InputMap>;

    InputManagerUPtr       g_mng;
    InputMapUPtr           g_map;

    int32_t                g_width = 0;
    int32_t                g_height = 0;

    gainput::DeviceId      g_keyboardId = gainput::InvalidDeviceId;
    gainput::DeviceId      g_mouseId = gainput::InvalidDeviceId;
    ReleaseCallbacks       g_keyReleaseCallbacks;
    PressCallbacks         g_keyPressCallbacks;
    DownCallbacks          g_keyDownCallbacks;
    DeltaCallbacks         g_deltaCallbacks;

    bool Initialize() {
        g_mng = std::make_unique<gainput::InputManager>();
        g_map = std::make_unique<gainput::InputMap>(*g_mng);

        g_keyboardId = g_mng->CreateDevice<gainput::InputDeviceKeyboard>(gainput::InvalidDeviceId, gainput::InputDevice::DV_RAW);
        g_mouseId = g_mng->CreateDevice<gainput::InputDeviceMouse>(gainput::InvalidDeviceId, gainput::InputDevice::DV_RAW);

        return true;
    }

    void Destroy() {
        g_keyReleaseCallbacks.clear();
        g_keyPressCallbacks.clear();
        g_keyDownCallbacks.clear();

        g_map.reset();
        g_mng.reset();
    }

    void Update() {
        g_mng->SetDisplaySize(g_width, g_height);
        g_mng->Update();

        for (auto& keyReleasePair : g_keyReleaseCallbacks) {
            const auto id = keyReleasePair.first;
            if (g_map->GetBoolWasDown(id)) {
                keyReleasePair.second(id);
            }
        }

        for (auto& keyPressPair : g_keyPressCallbacks) {
            const auto id = keyPressPair.first;
            if (g_map->GetBoolIsNew(id)) {
                keyPressPair.second(id);
            }
        }

        for (auto& keyDownPair : g_keyDownCallbacks) {
            const auto id = keyDownPair.first;
            if (g_map->GetBoolPrevious(id)) {
                keyDownPair.second(id);
            }
        }

        for (auto& deltaPair : g_deltaCallbacks) {
            const auto id = deltaPair.first;
            const auto delta = g_map->GetFloatDelta(id);
            if (std::numeric_limits<float>::epsilon() < std::fabsf(delta)) {
                deltaPair.second(id, delta);
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

    template<typename TCallback, typename TCallbacks>
    void SetKeyCallback(uint32_t id, gainput::DeviceButtonId key, TCallback callback, gainput::DeviceId deviceId, TCallbacks& callbacks) {
        callbacks.erase(id);
        callbacks.emplace(id, callback);

        g_map->MapBool(id, deviceId, key);
    }

    template<typename T>
    void SetValueCallback(uint32_t id, gainput::DeviceButtonId key, ValueCallback callback, gainput::DeviceId deviceId, T& callbacks) {
        callbacks.erase(id);
        callbacks.emplace(id, callback);

        g_map->MapFloat(id, deviceId, key);
    }

    // Keyboard.
    void Keyboard::SetRelease(uint32_t id, gainput::Key key, ReleaseCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_keyboardId, g_keyReleaseCallbacks);
    }

    void Keyboard::SetPress(uint32_t id, gainput::Key key, PressCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_keyboardId, g_keyPressCallbacks);
    }

    void Keyboard::SetDown(uint32_t id, gainput::Key key, DownCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_keyboardId, g_keyDownCallbacks);
    }

    // Mouse.
    void Mouse::SetRelease(uint32_t id, gainput::MouseButton key, ReleaseCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_mouseId, g_keyReleaseCallbacks);
    }

    void Mouse::SetPress(uint32_t id, gainput::MouseButton key, PressCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_mouseId, g_keyPressCallbacks);
    }

    void Mouse::SetDown(uint32_t id, gainput::MouseButton key, DownCallback callback) {
        SetKeyCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_mouseId, g_keyDownCallbacks);
    }

    void Mouse::SetDelta(uint32_t id, gainput::MouseButton key, ValueCallback callback) {
        SetValueCallback(id, static_cast<gainput::DeviceButtonId>(key), callback, g_mouseId, g_deltaCallbacks);
    }
}
