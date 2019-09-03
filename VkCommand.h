// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class CommandPool
	{
	public:
		bool			Initialize(uint32_t queueNodeIdx, VkDevice device);
		void			Release(VkDevice device);

		VkCommandPool	Get() const { return _cmdPool; }

	private:
		VkCommandPool	_cmdPool = VK_NULL_HANDLE;
	};
}
