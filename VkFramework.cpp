// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkFramework.h"

#include "VkMain.h"
#include "VkScene.h"
#include "VkCamera.h"

namespace Vk
{
	struct MouseButtons
	{
		bool left = false;
		bool right = false;
		bool middle = false;
	};

	struct LightSource
	{
		glm::vec3 color = glm::vec3(1.0f);
		glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
	};

	float fpsTimer = 0.0f;
	float frameTimer = 1.0f;
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	uint32_t currentBuffer = 0;
	uint32_t frameIndex = 0;

	uint32_t destWidth = 0;
	uint32_t destHeight = 0;
	bool resizing = false;
	bool prepared = false;
	bool paused = false;

	glm::vec2 mousePos = { 0.0f, 0.0f };
	MouseButtons mouseButtons;

	Camera _camera;
	LightSource _lightSource;
	Settings _settings;
	Main _main;
	Scene _scene;

	Settings& GetSettings()
	{
		return _settings;
	}

	bool Initialize()
	{
		return _main.Initialize(_settings);
	}

	void Release()
	{
		vkDeviceWaitIdle(_main.GetDevice());

		_scene.Release(_main.GetDevice());

		_main.Release(_settings);
	}

	void UpdateUniformBuffers()
	{
		_scene.UpdateUniformDatas(
			_camera.matrices.view,
			_camera.matrices.perspective,
			glm::vec3(
			-_camera.position.z * glm::sin(glm::radians(_camera.rotation.y)) * glm::cos(glm::radians(_camera.rotation.x)),
			-_camera.position.z * glm::sin(glm::radians(_camera.rotation.x)),
			_camera.position.z * glm::cos(glm::radians(_camera.rotation.y)) * glm::cos(glm::radians(_camera.rotation.x))),
			glm::vec4(
				glm::sin(glm::radians(_lightSource.rotation.x)) * glm::cos(glm::radians(_lightSource.rotation.y)),
				glm::sin(glm::radians(_lightSource.rotation.y)),
				glm::cos(glm::radians(_lightSource.rotation.x)) * glm::cos(glm::radians(_lightSource.rotation.y)),
				0.0f)
		);
	}

	void Prepare(HINSTANCE instance, HWND window)
	{
		_main.Prepare(_settings, instance, window);

		_scene.Initialize(_main, _settings);
		_scene.RecordBuffers(_main, _settings, _main.GetCommandBuffer(), _main.GetFrameBuffer());

		prepared = true;

		_camera.type = Camera::CameraType::lookat;
		_camera.setPerspective(45.0f, _settings.width / static_cast<float>(_settings.height), 0.1f, 256.0f);
		_camera.rotationSpeed = 0.25f;
		_camera.movementSpeed = 0.1f;
		_camera.setPosition({ 0.0f, 0.0f, 1.0f });
		_camera.setRotation({ 0.0f, 0.0f, 0.0f });
	}

	void WindowResize()
	{
		if (false == prepared)
			return;

		prepared = false;
		vkDeviceWaitIdle(_main.GetDevice());

		_settings.width = destWidth;
		_settings.height = destHeight;
		_main.RecreateSwapChain(_settings);

		vkDeviceWaitIdle(_main.GetDevice());

		const auto aspect = _settings.width / static_cast<float>(_settings.height);
		_camera.updateAspectRatio(aspect);

		prepared = true;
	}

	void render()
	{
		if (false == prepared)
			return;

		const auto acquire = _main.AcquireNextImage(currentBuffer, frameIndex);
		if (VK_ERROR_OUT_OF_DATE_KHR == acquire || VK_SUBOPTIMAL_KHR == acquire)
			WindowResize();
		else
			VK_CHECK_RESULT(acquire);

		UpdateUniformBuffers();
		_scene.OnUniformBufferSets(currentBuffer);

		const auto present = _main.QueuePresent(currentBuffer, frameIndex);
		if (false == (VK_SUCCESS == present || VK_SUBOPTIMAL_KHR == present))
		{
			if (VK_ERROR_OUT_OF_DATE_KHR == present)
			{
				WindowResize();
				return;
			}
			else
			{
				VK_CHECK_RESULT(present);
			}
		}

		frameIndex += 1;
		frameIndex %= _settings.renderAhead;
	}

	void RenderLoop(HWND window)
	{
		destWidth = _settings.width;
		destHeight = _settings.height;

		MSG msg = {};
		while (WM_QUIT != msg.message)
		{
			if (TRUE == PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				const auto tStart = std::chrono::high_resolution_clock::now();

				if (FALSE == IsIconic(window))
				{
					render();
				}
				++frameCounter;

				const auto tEnd = std::chrono::high_resolution_clock::now();
				const auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
				frameTimer = static_cast<float>(tDiff) / 1000.0f;

				_camera.update(frameTimer);

				fpsTimer += (float)tDiff;
				if (fpsTimer > 1000.0f)
				{
					lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
					fpsTimer = 0.0f;
					frameCounter = 0;
				}
			}
		}
	}

	void HandleMouseMove(int32_t x, int32_t y)
	{
		const auto dx = static_cast<int32_t>(mousePos.x - x);
		const auto dy = static_cast<int32_t>(mousePos.y - y);

		if (true == mouseButtons.left)
			_camera.rotate(glm::vec3(dy * _camera.rotationSpeed, -dx * _camera.rotationSpeed, 0.0f));

		if (true == mouseButtons.right)
			_camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f * _camera.movementSpeed));

		if (true == mouseButtons.middle)
			_camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));

		mousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
	}

	void HandleMessage(HWND window, uint32_t msg, WPARAM wParam, LPARAM lParam)
	{
		if (VK_NULL_HANDLE == _main.GetDevice())
			return;

		switch (msg)
		{
		case WM_CLOSE:
			prepared = false;
			DestroyWindow(window);
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(window, NULL);
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
			case KEY_P:
				paused = !paused;
				break;
			case KEY_ESCAPE:
				PostQuitMessage(0);
				break;
			}

			if (_camera.firstperson)
			{
				switch (wParam)
				{
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
				}
			}

			break;
		case WM_KEYUP:
			if (_camera.firstperson)
			{
				switch (wParam)
				{
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
				}
			}
			break;
		case WM_LBUTTONDOWN:
			mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
			mouseButtons.left = true;
			break;
		case WM_RBUTTONDOWN:
			mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
			mouseButtons.right = true;
			break;
		case WM_MBUTTONDOWN:
			mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
			mouseButtons.middle = true;
			break;
		case WM_LBUTTONUP:
			mouseButtons.left = false;
			break;
		case WM_RBUTTONUP:
			mouseButtons.right = false;
			break;
		case WM_MBUTTONUP:
			mouseButtons.middle = false;
			break;
		case WM_MOUSEWHEEL:
		{
			const auto wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			_camera.translate(glm::vec3(0.0f, 0.0f, -static_cast<float>(wheelDelta) * 0.005f * _camera.movementSpeed));
			break;
		}
		case WM_MOUSEMOVE:
		{
			HandleMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		case WM_SIZE:
			if (true == prepared && SIZE_MINIMIZED != wParam)
			{
				if (true == resizing || SIZE_MAXIMIZED == wParam || SIZE_RESTORED == wParam)
				{
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
		}
	}
}
