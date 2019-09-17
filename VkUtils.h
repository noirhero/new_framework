// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkTexture.h"

namespace Vk
{
#define VK_CHECK_RESULT(f) \
	{ \
		VkResult res = (f); \
		if (res != VK_SUCCESS) \
		{ \
			std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
			assert(res == VK_SUCCESS); \
		} \
	}

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint) \
	{ \
		fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk"#entrypoint)); \
		if (fp##entrypoint == nullptr) \
		{ \
			exit(1); \
		} \
	}

#define GET_DEVICE_PROC_ADDR(dev, entrypoint) \
	{ \
		fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk"#entrypoint)); \
		if (fp##entrypoint == nullptr) \
		{ \
			exit(1); \
		} \
	}
	
	struct VulkanDevice;

	VkPipelineShaderStageCreateInfo loadShader(VkDevice device, const std::string& filename, VkShaderStageFlagBits stage);

	void readDirectory(const std::string& directory, const std::string &pattern, std::map<std::string, std::string> &filelist, bool recursive);

	Texture2D GenerateBRDFLUT(VkDevice device, VkQueue queue, VkPipelineCache pipelineCache, VulkanDevice& vulkanDevice);
}
