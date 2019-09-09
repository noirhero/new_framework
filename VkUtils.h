// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkTexture.h"

namespace Vk
{
	struct VulkanDevice;

	VkPipelineShaderStageCreateInfo loadShader(VkDevice device, std::string filename, VkShaderStageFlagBits stage);

	void readDirectory(const std::string& directory, const std::string &pattern, std::map<std::string, std::string> &filelist, bool recursive);

	Texture2D GenerateBRDFLUT(VkDevice device, VkQueue queue, VkPipelineCache pipelineCache, VulkanDevice& vulkanDevice);
}
