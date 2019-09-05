// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class CommandPool
	{
	public:
		bool				Initialize(uint32_t queueNodeIdx, VkDevice device);
		void				Release(VkDevice device);

		VkCommandPool		Get() const { return _cmdPool; }

	private:
		VkCommandPool		_cmdPool = VK_NULL_HANDLE;
	};

	using VkCommandBuffers = std::vector<VkCommandBuffer>;
	class CommandBuffer
	{
	public:
		bool					Initialize(VkDevice device, VkCommandPool cmdPool, uint32_t count);
		void					Release(VkDevice device, VkCommandPool cmdPool);

		uint32_t				Count() const { return static_cast<uint32_t>(_cmdBufs.size()); }
		VkCommandBuffer			Get(uint32_t i) const { return _cmdBufs[i]; }
		const VkCommandBuffer*	GetPtr() const { return _cmdBufs.data(); }

	private:
		VkCommandBuffers		_cmdBufs;
	};
}
