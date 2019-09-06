// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "Vk.h"

#include "VkMain.h"
#include "VkPipelineCache.h"
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

	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	bool prepared = false;
	bool paused = false;

	glm::vec2 mousePos;
	MouseButtons mouseButtons;
	bool rotateModel = false;
	glm::vec3 modelrot = glm::vec3(0.0f);
	glm::vec3 modelPos = glm::vec3(0.0f);
	Camera camera;

	LightSource lightSource;

	Settings _settings;

	Main _main;

	uint32_t currentBuffer = 0;
	CommandBuffer _cmdBuffers;
	FrameBuffer _frameBuffers;

	std::vector<VkFence> waitFences;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkSemaphore> presentCompleteSemaphores;

	const uint32_t renderAhead = 2;
	uint32_t frameIndex = 0;

	int32_t animationIndex = 0;
	float animationTimer = 0.0f;
	bool animate = true;

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

		releaseAssets(_main.GetDevice());

		_cmdBuffers.Release(_main.GetDevice(), _main.GetCommandPool());

		for (auto fence : waitFences)
			vkDestroyFence(_main.GetDevice(), fence, nullptr);

		for (auto semaphore : renderCompleteSemaphores)
			vkDestroySemaphore(_main.GetDevice(), semaphore, nullptr);

		for (auto semaphore : presentCompleteSemaphores)
			vkDestroySemaphore(_main.GetDevice(), semaphore, nullptr);

		//delete ui;

		// Clean up Vulkan resources
		_frameBuffers.Release(_settings, _main.GetDevice());

		_main.Release();
	}

	void updateUniformBuffers()
	{
		UpdateSceneUniformBuffer(camera.matrices.view, camera.matrices.perspective, glm::vec3(
			-camera.position.z * sin(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x)),
			-camera.position.z * sin(glm::radians(camera.rotation.x)),
			camera.position.z * cos(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x))
		));
	}

	/*
		Prepare and initialize uniform buffers containing shader parameters
	*/
	void prepareUniformBuffers()
	{
		PrepreSceneUniformBuffes(_main.GetVulkanDevice(), _main.GetVulkanSwapChain());

		updateUniformBuffers();
	}

	void Prepare(HINSTANCE windowInstance, HWND window)
	{
		_main.Prepare(_settings, windowInstance, window);

		_frameBuffers.Initialize(_settings, _main.GetVulkanDevice(), _main.GetVulkanSwapChain(), _main.GetDepthFormat(), _main.GetRenderPass());

		// Command buffer execution fences
		waitFences.resize(renderAhead);
		for (auto &waitFence : waitFences)
		{
			VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
			VK_CHECK_RESULT(vkCreateFence(_main.GetDevice(), &fenceCI, nullptr, &waitFence));
		}

		// Queue ordering semaphores
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

		// Command buffers
		_cmdBuffers.Initialize(_main.GetDevice(), _main.GetCommandPool(), _main.GetSwapChainImageCount());

		loadAssets(_main.GetVulkanDevice(), _main.GetGPUQueue(), _main.GetPipelineCache());
		generateBRDFLUT(_main.GetVulkanDevice(), _main.GetGPUQueue(), _main.GetPipelineCache());
		prepareUniformBuffers();
		setupDescriptors(_main.GetDevice(), _main.GetVulkanSwapChain());
		preparePipelines(_settings, _main.GetDevice(), _main.GetRenderPass(), _main.GetPipelineCache());

		//ui = new UI(vulkanDevice, renderPass, queue, pipelineCache, settings.sampleCount);
		//updateOverlay();

		recordCommandBuffers(_settings, _main.GetRenderPass(), _cmdBuffers, _frameBuffers);

		prepared = true;

		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(45.0f, (float)_settings.width / (float)_settings.height, 0.1f, 256.0f);
		camera.rotationSpeed = 0.25f;
		camera.movementSpeed = 0.1f;
		camera.setPosition({ 0.0f, 0.0f, 1.0f });
		camera.setRotation({ 0.0f, 0.0f, 0.0f });
	}

	void WindowResize()
	{
		if (!prepared)
			return;

		prepared = false;

		vkDeviceWaitIdle(_main.GetDevice());

		_settings.width = destWidth;
		_settings.height = destHeight;

		_main.RecreateSwapChain(_settings);

		_frameBuffers.Release(_settings, _main.GetDevice());
		_frameBuffers.Initialize(_settings, _main.GetVulkanDevice(), _main.GetVulkanSwapChain(), _main.GetDepthFormat(), _main.GetRenderPass());

		vkDeviceWaitIdle(_main.GetDevice());

		camera.updateAspectRatio((float)_settings.width / (float)_settings.height);

		prepared = true;
	}

	void updateParams()
	{
		UpdateLightSourceDirection(glm::vec4(
			sin(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)),
			sin(glm::radians(lightSource.rotation.y)),
			cos(glm::radians(lightSource.rotation.x)) * cos(glm::radians(lightSource.rotation.y)),
			0.0f));
	}

	void render()
	{
		if (!prepared)
			return;

		//updateOverlay();

		VK_CHECK_RESULT(vkWaitForFences(_main.GetDevice(), 1, &waitFences[frameIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(_main.GetDevice(), 1, &waitFences[frameIndex]));

		const auto acquire = _main.AcquireNextImage(currentBuffer, presentCompleteSemaphores[frameIndex]);
		if (VK_ERROR_OUT_OF_DATE_KHR == acquire || VK_SUBOPTIMAL_KHR == acquire)
			WindowResize();
		else
			VK_CHECK_RESULT(acquire);

		// Update UBOs
		updateUniformBuffers();
		UpdateUniformBuffetSet(currentBuffer);

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

		if (!paused)
		{
			if (rotateModel)
			{
				modelrot.y += frameTimer * 35.0f;
				if (modelrot.y > 360.0f)
				{
					modelrot.y -= 360.0f;
				}
			}
			//if ((animate) && (models.scene.animations.size() > 0)) {
			//	animationTimer += frameTimer * 0.75f;
			//	if (animationTimer > models.scene.animations[animationIndex].end) {
			//		animationTimer -= models.scene.animations[animationIndex].end;
			//	}
			//	models.scene.updateAnimation(animationIndex, animationTimer);
			//}

			updateParams();

			if (rotateModel)
			{
				updateUniformBuffers();
			}
		}
		if (camera.updated)
		{
			updateUniformBuffers();
		}
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

				camera.update(frameTimer);

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

		//ImGuiIO& io = ImGui::GetIO();
		//bool handled = io.WantCaptureMouse;

		//if (handled) {
		//	mousePos = glm::vec2((float)x, (float)y);
		//	return;
		//}

		//if (handled) {
		//	mousePos = glm::vec2((float)x, (float)y);
		//	return;
		//}

		if (true == mouseButtons.left)
			camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));

		if (mouseButtons.right)
			camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f * camera.movementSpeed));

		if (mouseButtons.middle)
			camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));

		mousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
	}

	void HandleMessage(HWND handle, uint32_t msg, WPARAM wParam, LPARAM lParam)
	{
		if (VK_NULL_HANDLE == _main.GetDevice())
			return;

		switch (msg)
		{
		case WM_CLOSE:
			prepared = false;
			DestroyWindow(handle);
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(handle, NULL);
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

			if (camera.firstperson)
			{
				switch (wParam)
				{
				case KEY_W:
					camera.keys.up = true;
					break;
				case KEY_S:
					camera.keys.down = true;
					break;
				case KEY_A:
					camera.keys.left = true;
					break;
				case KEY_D:
					camera.keys.right = true;
					break;
				}
			}

			break;
		case WM_KEYUP:
			if (camera.firstperson)
			{
				switch (wParam)
				{
				case KEY_W:
					camera.keys.up = false;
					break;
				case KEY_S:
					camera.keys.down = false;
					break;
				case KEY_A:
					camera.keys.left = false;
					break;
				case KEY_D:
					camera.keys.right = false;
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
			camera.translate(glm::vec3(0.0f, 0.0f, -(float)wheelDelta * 0.005f * camera.movementSpeed));
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
