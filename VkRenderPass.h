// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class RenderPass
	{
	public:
		bool			Initialize(VkFormat colorFormat, VkFormat depthFormat, VkDevice device, bool isMultiSampling, VkSampleCountFlagBits sampleCount);
		void			Release(VkDevice device);

		VkRenderPass	Get() const { return _renderPass; }

	private:
		VkRenderPass	_renderPass = VK_NULL_HANDLE;
	};
}
