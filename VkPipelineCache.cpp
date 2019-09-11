// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkPipelineCache.h"

#include "VkUtils.h"

namespace Vk
{
	bool PipelineCache::Initialize(VkDevice device)
	{
		VkPipelineCacheCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(device, &info, nullptr, &_pipelineCache));

		return true;
	}

	void PipelineCache::Release(VkDevice device)
	{
		if (VK_NULL_HANDLE == _pipelineCache)
			return;

		vkDestroyPipelineCache(device, _pipelineCache, nullptr);
		_pipelineCache = VK_NULL_HANDLE;
	}
}
