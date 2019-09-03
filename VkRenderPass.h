// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
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
