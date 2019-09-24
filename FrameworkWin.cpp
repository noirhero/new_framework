// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "FrameworkWin.h"

#include "VkWin.h"
#include "VkUtils.h"
#include "VkMain.h"
#include "VkScene.h"
#include "VkCamera.h"

#include "Timer.h"
#include "Path.h"

namespace Framework {
    namespace Win {
        constexpr auto KEY_ESCAPE = VK_ESCAPE;
        //constexpr auto KEY_F1  = VK_F1;
        //constexpr auto KEY_F2  = VK_F2;
        //constexpr auto KEY_F3  = VK_F3;
        //constexpr auto KEY_F4  = VK_F4;
        //constexpr auto KEY_F5  = VK_F5;
        constexpr auto KEY_W = 0x57;
        constexpr auto KEY_A = 0x41;
        constexpr auto KEY_S = 0x53;
        constexpr auto KEY_D = 0x44;
        constexpr auto KEY_P = 0x50;
        //constexpr auto KEY_SPACE  = 0x20;
        //constexpr auto KEY_KPADD  = 0x6B;
        //constexpr auto KEY_KPSUB  = 0x6D;
        //constexpr auto KEY_B  = 0x42;
        //constexpr auto KEY_F  = 0x46;
        //constexpr auto KEY_L  = 0x4C;
        //constexpr auto KEY_N  = 0x4E;
        //constexpr auto KEY_O  = 0x4F;
        //constexpr auto KEY_T  = 0x54;

        struct MouseButtons {
            bool left = false;
            bool right = false;
            bool middle = false;
        };

        struct LightSource {
            glm::vec3 color = glm::vec3(1.0f);
            glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
        };

        glm::vec4 GetLightDirection(const LightSource& lightSource) {
            return {
                glm::sin(glm::radians(lightSource.rotation.x)) * glm::cos(glm::radians(lightSource.rotation.y)),
                glm::sin(glm::radians(lightSource.rotation.y)),
                glm::cos(glm::radians(lightSource.rotation.x)) * glm::cos(glm::radians(lightSource.rotation.y)),
                0.0f
            };
        }

        uint32_t currentBuffer = 0;
        uint32_t frameIndex = 0;

        uint32_t destWidth = 0;
        uint32_t destHeight = 0;
        bool resizing = false;
        bool prepared = false;
        bool paused = false;

        glm::vec2 _mousePos{};
        MouseButtons _mouseButtons;
        LightSource _lightSource;
        Timer _timer;

        Vk::Camera _camera;
        Vk::Main _main;
        Vk::Scene _scene;
        Vk::Win _win;

        void HandleMouseMove(int32_t x, int32_t y) {
            const auto dx = static_cast<int32_t>(_mousePos.x - x);
            const auto dy = static_cast<int32_t>(_mousePos.y - y);

            if (true == _mouseButtons.left)
                _camera.Rotate(glm::vec3(dy * _camera.rotationSpeed, -dx * _camera.rotationSpeed, 0.0f));

            if (true == _mouseButtons.right)
                _camera.Translate(glm::vec3(-0.0f, 0.0f, dy * .005f * _camera.movementSpeed));

            if (true == _mouseButtons.middle)
                _camera.Translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));

            _mousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
        }

        void WindowResize() {
            if (false == prepared)
                return;

            prepared = false;

            _main.GetSettings().width = destWidth;
            _main.GetSettings().height = destHeight;

            _main.RecreateSwapChain();
            _scene.RecordBuffers(_main);

            const float aspect = _main.GetSettings().width / static_cast<float>(_main.GetSettings().height);
            _camera.UpdateAspectRatio(aspect);

            prepared = true;
        }

        void HandleMessage(HWND window, uint32_t msg, WPARAM wParam, LPARAM lParam) {
            if (VK_NULL_HANDLE == _main.GetDevice())
                return;

            switch (msg) {
            case WM_CLOSE:
                prepared = false;
                DestroyWindow(window);
                PostQuitMessage(0);
                break;
            case WM_PAINT:
                ValidateRect(window, nullptr);
                break;
            case WM_KEYDOWN:
                switch (wParam) {
                case KEY_P:
                    paused = !paused;
                    break;
                case KEY_ESCAPE:
                    PostQuitMessage(0);
                    break;
                default:
                    break;
                }

                if (Vk::Camera::CameraType::FirstPerson == _camera.type) {
                    switch (wParam) {
                    case KEY_W:
                        _camera.keys.up = true;
                        break;
                    case KEY_S:
                        _camera.keys.down = true;
                        break;
                    case KEY_A:
                        _camera.keys.left = true;
                        break;
                    case KEY_D:
                        _camera.keys.right = true;
                        break;
                    default:
                        break;
                    }
                }

                break;
            case WM_KEYUP:
                if (Vk::Camera::CameraType::FirstPerson == _camera.type) {
                    switch (wParam) {
                    case KEY_W:
                        _camera.keys.up = false;
                        break;
                    case KEY_S:
                        _camera.keys.down = false;
                        break;
                    case KEY_A:
                        _camera.keys.left = false;
                        break;
                    case KEY_D:
                        _camera.keys.right = false;
                        break;
                    default:
                        break;
                    }
                }
                break;
            case WM_LBUTTONDOWN:
                _mousePos = glm::vec2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
                _mouseButtons.left = true;
                break;
            case WM_RBUTTONDOWN:
                _mousePos = glm::vec2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
                _mouseButtons.right = true;
                break;
            case WM_MBUTTONDOWN:
                _mousePos = glm::vec2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
                _mouseButtons.middle = true;
                break;
            case WM_LBUTTONUP:
                _mouseButtons.left = false;
                break;
            case WM_RBUTTONUP:
                _mouseButtons.right = false;
                break;
            case WM_MBUTTONUP:
                _mouseButtons.middle = false;
                break;
            case WM_MOUSEWHEEL:
            {
                const auto wheelDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
                _camera.Translate(glm::vec3(0.0f, 0.0f, -wheelDelta * 0.005f * _camera.movementSpeed));
                break;
            }
            case WM_MOUSEMOVE:
            {
                HandleMouseMove(LOWORD(lParam), HIWORD(lParam));
                break;
            }
            case WM_SIZE:
                if (true == prepared && SIZE_MINIMIZED != wParam) {
                    if (true == resizing || SIZE_MAXIMIZED == wParam || SIZE_RESTORED == wParam) {
                        destWidth = LOWORD(lParam);
                        destHeight = HIWORD(lParam);
                        WindowResize();
                    }
                }
                break;
            case WM_ENTERSIZEMOVE:
                resizing = true;
                break;
            case WM_EXITSIZEMOVE:
                resizing = false;
                break;
            default:
                return;
            }
        }

        LRESULT CALLBACK WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam) {
            HandleMessage(handle, msg, wParam, lParam);

            return DefWindowProc(handle, msg, wParam, lParam);
        }

        bool Initialize(std::string&& assetPath, HINSTANCE instance) {
        #if defined(_DEBUG)
            _main.GetSettings().validation = true;
        #endif

            if (false == Path::SetAssetPath(std::move(assetPath))) {
                return false;
            }
            else if(false == _main.Initialize()) {
                return false;
            }

            _win.SetupWindow(_main.GetSettings(), instance, WndProc);
            if (true == _main.GetSettings().validation) {
                Vk::Win::CreateConsole();
            }

            return true;
        }

        void Release() {
            _scene.Release(_main);
            _main.Release();
        }

        void UpdateUniformBuffers() {
            _scene.UpdateUniformDatas(_camera.matrices.view, _camera.matrices.perspective, _camera.GetCameraPosition(), GetLightDirection(_lightSource));
        }

        void Prepare() {
            _main.Prepare(_win.GetInstance(), _win.GetHandle());

            _scene.Initialize(_main, "environments/papermill.ktx"s);
            _scene.LoadScene(_main, Path::Apply("models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf"s));
            _scene.RecordBuffers(_main);

            prepared = true;

            const float aspect = _main.GetSettings().width / static_cast<float>(_main.GetSettings().height);
            _camera.type = Vk::Camera::CameraType::LookAt;
            _camera.SetPerspective(45.0f, aspect, 0.1f, 256.0f);
            _camera.rotationSpeed = 0.25f;
            _camera.movementSpeed = 0.1f;
            _camera.SetPosition({ 0.0f, 0.0f, 1.0f });
            _camera.SetRotation({ 0.0f, 0.0f, 0.0f });
        }

        void Render() {
            if (false == prepared)
                return;

            const auto acquire = _main.AcquireNextImage(currentBuffer, frameIndex);
            if (VK_ERROR_OUT_OF_DATE_KHR == acquire || VK_SUBOPTIMAL_KHR == acquire)
                WindowResize();
            else
                Vk::CheckResult(acquire);

            UpdateUniformBuffers();
            _scene.OnUniformBufferSets(currentBuffer);

            const auto present = _main.QueuePresent(currentBuffer, frameIndex);
            if (false == (VK_SUCCESS == present || VK_SUBOPTIMAL_KHR == present)) {
                if (VK_ERROR_OUT_OF_DATE_KHR == present) {
                    WindowResize();
                    return;
                }
                else {
                    Vk::CheckResult(present);
                }
            }

            frameIndex += 1;
            frameIndex %= _main.GetSettings().renderAhead;
        }

        void RenderLoop() {
            destWidth = _main.GetSettings().width;
            destHeight = _main.GetSettings().height;

            MSG msg = {};
            while (WM_QUIT != msg.message) {
                if (TRUE == PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else {
                    _timer.Update();

                    if (FALSE == IsIconic(_win.GetHandle())) {
                        Render();
                    }

                    _camera.Update(_timer.Delta());
                }
            }
        }
    }
}
