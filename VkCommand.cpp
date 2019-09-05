// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkCommand.h"

#include "VkCommon.h"

namespace Vk
{
	// Command pool
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

	// Command buffer
	bool CommandBuffer::Initialize(VkDevice device, VkCommandPool cmdPool, uint32_t count)
	{
		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = cmdPool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount = count;

		_cmdBufs.resize(count, VK_NULL_HANDLE);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &info, _cmdBufs.data()));

		return true;
	}

	void CommandBuffer::Release(VkDevice device, VkCommandPool cmdPool)
	{
		const uint32_t count = static_cast<uint32_t>(_cmdBufs.size());
		if (0 == count)
			return;

		vkFreeCommandBuffers(device, cmdPool, count, _cmdBufs.data());
		_cmdBufs.clear();
	}
}
