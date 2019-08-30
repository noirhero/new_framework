// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	bool Initialize();
	void Release();

	HWND SetupWindow(HINSTANCE instance, WNDPROC wndProc);
	void Prepare();
	void RenderLoop();

	void HandleMessage(HWND handle, uint32_t msg, WPARAM wParam, LPARAM lParam);
}
