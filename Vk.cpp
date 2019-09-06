// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "Vk.h"

#include "VkMain.h"
#include "VkMesh.h"
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

	uint32_t currentBuffer = 0;
	CommandBuffer _cmdBuffers;
	FrameBuffer _frameBuffers;

	std::vector<VkFence> waitFences; // Command buffer execution fences
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkSemaphore> presentCompleteSemaphores; // Queue ordering semaphores

	const uint32_t renderAhead = 2;
	uint32_t frameIndex = 0;

	Settings& GetSettings()
	{
		return _settings;
	}

	void CreateFences()
	{
		waitFences.resize(renderAhead);
		for (auto &waitFence : waitFences)
		{
			VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
			VK_CHECK_RESULT(vkCreateFence(_main.GetDevice(), &fenceCI, nullptr, &waitFence));
		}

		presentCompleteSemaphores.resize(renderAhead);
		for (auto &semaphore : presentCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(_main.GetDevice(), &semaphoreCI, nullptr, &semaphore));
		}

		renderCompleteSemaphores.resize(renderAhead);
		for (auto &semaphore : renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(_main.GetDevice(), &semaphoreCI, nullptr, &semaphore));
		}
	}

	void ReleaseFences()
	{
		for (auto fence : waitFences)
			vkDestroyFence(_main.GetDevice(), fence, nullptr);

		for (auto semaphore : renderCompleteSemaphores)
			vkDestroySemaphore(_main.GetDevice(), semaphore, nullptr);

		for (auto semaphore : presentCompleteSemaphores)
			vkDestroySemaphore(_main.GetDevice(), semaphore, nullptr);
	}

	bool Initialize()
	{
		return _main.Initialize(_settings);
	}

	void Release()
	{
		vkDeviceWaitIdle(_main.GetDevice());

		_scene.Release(_main.GetDevice());
		_frameBuffers.Release(_settings, _main.GetDevice());
		_cmdBuffers.Release(_main.GetDevice(), _main.GetCommandPool());

		ReleaseFences();

		_main.Release();
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
		CreateFences();

		_main.Prepare(_settings, instance, window);

		_frameBuffers.Initialize(_settings, _main.GetVulkanDevice(), _main.GetVulkanSwapChain(), _main.GetDepthFormat(), _main.GetRenderPass());
		_cmdBuffers.Initialize(_main.GetDevice(), _main.GetCommandPool(), _main.GetSwapChainImageCount());

		_scene.Initialize(_main, _settings);
		_scene.RecordBuffers(_main, _settings, _cmdBuffers, _frameBuffers);

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

		_frameBuffers.Release(_settings, _main.GetDevice());
		_frameBuffers.Initialize(_settings, _main.GetVulkanDevice(), _main.GetVulkanSwapChain(), _main.GetDepthFormat(), _main.GetRenderPass());

		vkDeviceWaitIdle(_main.GetDevice());

		const auto aspect = _settings.width / static_cast<float>(_settings.height);
		_camera.updateAspectRatio(aspect);

		prepared = true;
	}

	void render()
	{
		if (false == prepared)
			return;

		VK_CHECK_RESULT(vkWaitForFences(_main.GetDevice(), 1, &waitFences[frameIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(_main.GetDevice(), 1, &waitFences[frameIndex]));

		const auto acquire = _main.AcquireNextImage(currentBuffer, presentCompleteSemaphores[frameIndex]);
		if (VK_ERROR_OUT_OF_DATE_KHR == acquire || VK_SUBOPTIMAL_KHR == acquire)
			WindowResize();
		else
			VK_CHECK_RESULT(acquire);

		UpdateUniformBuffers();
		_scene.OnUniformBufferSets(currentBuffer);

		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[frameIndex];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[frameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pCommandBuffers = &_cmdBuffers.GetPtr()[currentBuffer];
		submitInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkQueueSubmit(_main.GetGPUQueue(), 1, &submitInfo, waitFences[frameIndex]));

		const auto present = _main.QueuePresent(currentBuffer, renderCompleteSemaphores[frameIndex]);
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
		frameIndex %= renderAhead;
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
			short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			_camera.translate(glm::vec3(0.0f, 0.0f, -(float)wheelDelta * 0.005f * _camera.movementSpeed));
			break;
		}
		case WM_MOUSEMOVE:
		{
			HandleMouseMove(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		case WM_SIZE:
			if ((prepared) && (wParam != SIZE_MINIMIZED))
			{
				if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
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
