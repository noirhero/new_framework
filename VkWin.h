// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	struct Settings;

	namespace Win
	{
		HINSTANCE	GetInstance();
		HWND		GetHandle();

		void		CreateConsole();
		bool		SetupWindow(Settings& settings, HINSTANCE instance, WNDPROC wndProc);
	}
}
