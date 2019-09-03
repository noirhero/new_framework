// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	Settings& GetSettings();

	bool Initialize();
	void Release();

	void Prepare(HINSTANCE windowInstance, HWND window);
	void RenderLoop(HWND window);

	void HandleMessage(HWND handle, uint32_t msg, WPARAM wParam, LPARAM lParam);
}
