// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	namespace Win
	{
		HINSTANCE	GetInstance();
		HWND		GetHandle();

		void		CreateConsole();
		bool		SetupWindow(Settings& settings, HINSTANCE instance, WNDPROC wndProc);
	}
}
