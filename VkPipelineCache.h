// Copyright 2012-2019 GameParadiso, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class PipelineCache
	{
	public:
		bool Initialize(VkDevice device);
		void Release(VkDevice device);

		VkPipelineCache Get() const { return _pipelineCache; }

	private:
		VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
	};
}
