// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkBuffer.h"

#include "VkCommon.h"
#include "VkDevice.h"

namespace Vk
{
	void Buffer::create(VulkanDevice *device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, bool map)
	{
		this->device = device->logicalDevice;
		device->createBuffer(usageFlags, memoryPropertyFlags, size, &buffer, &memory);
		descriptor = { buffer, 0, size };
		if (map) {
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, memory, 0, size, 0, &mapped));
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
		VK_CHECK_RESULT(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped));
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
		VK_CHECK_RESULT(vkFlushMappedMemoryRanges(device, 1, &mappedRange));
	}
}
