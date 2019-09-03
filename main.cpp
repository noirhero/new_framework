﻿// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

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
#if defined(_DEBUG)
	Vk::GetSettings().validation = true;
#endif

	Vk::Initialize();

	Vk::Win::SetupWindow(Vk::GetSettings(), instance, WndProc);
	if (true == Vk::GetSettings().validation)
		Vk::Win::CreateConsole();

	Vk::Prepare(Vk::Win::GetInstance(), Vk::Win::GetHandle());
	Vk::RenderLoop(Vk::Win::GetHandle());

	Vk::Release();

	return 0;
}
