// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#include "stdafx.h"

#include "Vk.h"
#include "VkBody.h"
#include "VkWin.h"

LRESULT CALLBACK WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Vk::HandleMessage(handle, msg, wParam, lParam);

	return DefWindowProc(handle, msg, wParam, lParam);
}

int32_t APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int32_t)
{
	//Vk::Initialize();
	//Vk::SetupWindow(instance, WndProc);
	//Vk::Prepare();
	//Vk::RenderLoop();

	Vk::Settings settngs;

	Vk::Body body;
	if (false == body.Initialize())
		return 0;

#if defined(_DEBUG)
	Vk::Win::CreateConsole();
#endif
	Vk::Win::SetupWindow(settngs, instance, WndProc);

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

		}
	}

	return 0;
}
