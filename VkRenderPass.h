// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	struct Settings;

	class RenderPass
	{
	public:
		bool			Initialize(const Settings& settings, VkFormat colorFormat, VkFormat depthFormat, VkDevice device);
		void			Release(VkDevice device);

		VkRenderPass	Get() const { return _renderPass; }

	private:
		VkRenderPass	_renderPass = VK_NULL_HANDLE;
	};
}
