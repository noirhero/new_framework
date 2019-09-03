// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkCommand.h"

#include "VkCommon.h"

namespace Vk
{
	bool CommandPool::Initialize(uint32_t queueNodeIdx, VkDevice device)
	{
		VkCommandPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = queueNodeIdx;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK_RESULT(vkCreateCommandPool(device, &info, nullptr, &_cmdPool));

		return false;
	}

	void CommandPool::Release(VkDevice device)
	{
		if (VK_NULL_HANDLE == _cmdPool)
			return;

		vkDestroyCommandPool(device, _cmdPool, nullptr);
		_cmdPool = VK_NULL_HANDLE;
	}
}
