// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkWin.h"

#include "VkMain.h"

namespace Vk
{
	const wchar_t* _className = L"NEW_FRAMEWORK";
	const wchar_t* _titleName = L"New Framework";

	void Win::CreateConsole()
	{
		AllocConsole();
		AttachConsole(GetCurrentProcessId());

		FILE *stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w+", stdout);
		freopen_s(&stream, "CONOUT$", "w+", stderr);

		SetConsoleTitle(TEXT("Vulkan validation output"));
	}

	bool Win::SetupWindow(Settings& settings, HINSTANCE instance, WNDPROC wndProc)
	{
		_instance = instance;

		const WNDCLASSEX wndClass =
		{
			sizeof(WNDCLASSEX),
			CS_HREDRAW | CS_VREDRAW,
			wndProc,
			0/*cbClsExtra*/,
			0/*cbWndExtra*/,
			instance,
			LoadIcon(NULL, IDI_APPLICATION),
			LoadCursor(NULL, IDC_ARROW),
			(HBRUSH)GetStockObject(BLACK_BRUSH),
			nullptr/*lpszMenuName*/,
			_className,
			LoadIcon(NULL, IDI_WINLOGO),
		};
		if (0 == RegisterClassEx(&wndClass))
		{
			std::cout << "Could not register window class!\n";
			fflush(stdout);

			return false;
		}

		const uint32_t screenWidth = static_cast<uint32_t>(GetSystemMetrics(SM_CXSCREEN));
		const uint32_t screenHeight = static_cast<uint32_t>(GetSystemMetrics(SM_CYSCREEN));

		if (true == settings.fullscreen)
		{
			if (settings.width != screenWidth && settings.height != screenHeight)
			{
				DEVMODE screenSettings = {};
				screenSettings.dmSize = sizeof(DEVMODE);
				screenSettings.dmPelsWidth = screenWidth;
				screenSettings.dmPelsHeight = screenHeight;
				screenSettings.dmBitsPerPel = 32;
				screenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

				if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN))
				{
					const auto msgBoxStyle = MB_YESNO | MB_ICONEXCLAMATION;
					if (IDNO == MessageBoxA(nullptr, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", msgBoxStyle))
						return false;
					else
						settings.fullscreen = false;
				}
			}
		}

		const auto dwExStyle = settings.fullscreen ? WS_EX_APPWINDOW : (WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
		const auto dwStyle = settings.fullscreen ? (WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN) : (WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		RECT windowRect =
		{
			0/*left*/,
			0/*top*/,
			settings.fullscreen ? (long)screenWidth : (long)settings.width,
			settings.fullscreen ? (long)screenHeight : (long)settings.height,
		};
		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		_handle = CreateWindowEx
		(
			0/*dwExStyle*/,
			_className,
			_titleName,
			dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0/*x*/,
			0/*y*/,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr/*hWndParent*/,
			nullptr/*hMenu*/,
			instance,
			nullptr/*lpParam*/
		);

		if (false == settings.fullscreen)
		{
			const auto x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
			const auto y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
			SetWindowPos(_handle, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}

		if (nullptr == _handle)
		{
			std::cout << "Could not create window!\n";
			fflush(stdout);

			return false;
		}

		ShowWindow(_handle, SW_SHOW);
		SetForegroundWindow(_handle);
		SetFocus(_handle);

		return true;
	}
}
