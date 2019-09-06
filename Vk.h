// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	Settings&	GetSettings();

	bool		Initialize();
	void		Release();

	void		Prepare(HINSTANCE instance, HWND window);
	void		RenderLoop(HWND window);

	void		HandleMessage(HWND window, uint32_t msg, WPARAM wParam, LPARAM lParam);
}
