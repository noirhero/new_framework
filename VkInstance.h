// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	class Instance
	{
	public:
		bool		Initialize(Settings& settings);
		void		Release();

		VkInstance	Get() const { return _instance; }

	private:
		VkInstance	_instance = VK_NULL_HANDLE;
	};
}
