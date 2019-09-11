// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class Debug
	{
	public:
		bool						Initialize(VkInstance instance);
		void						Release(VkInstance instance);

	private:
		VkDebugReportCallbackEXT	_handle = VK_NULL_HANDLE;
	};
}
