// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkBuffer.h"

#include "VkUtils.h"
#include "VulkanDevice.h"

namespace Vk
{
	void Buffer::create(VulkanDevice *inDevice, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map)
	{
		this->device = inDevice->logicalDevice;
		inDevice->createBuffer(usageFlags, memoryPropertyFlags, size, &buffer, &memory);
		descriptor = { buffer, 0, size };
		if (map) {
			CheckResult(vkMapMemory(inDevice->logicalDevice, memory, 0, size, 0, &mapped));
		}
	}

	void Buffer::destroy()
	{
		if (mapped) {
			unmap();
		}
		vkDestroyBuffer(device, buffer, nullptr);
		vkFreeMemory(device, memory, nullptr);
		buffer = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
	}

	void Buffer::map()
	{
		CheckResult(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped));
	}

	void Buffer::unmap()
	{
		if (mapped) {
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	void Buffer::flush(VkDeviceSize size)
	{
		VkMappedMemoryRange mappedRange{};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.size = size;
		CheckResult(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
	}
}
