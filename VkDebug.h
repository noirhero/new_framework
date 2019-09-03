// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class Debug
	{
	public:
		bool Initialize(VkInstance instance);
		void Release(VkInstance instance);

	private:
		VkDebugReportCallbackEXT _handle = VK_NULL_HANDLE;
	};
}
