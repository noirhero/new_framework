// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkTexture.h"

namespace Vk
{
    struct VulkanDevice;

    constexpr void CheckResult(const VkResult res) {
        if(VK_SUCCESS != res) {
            std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
            assert(res == VK_SUCCESS);
        }
    }

	VkPipelineShaderStageCreateInfo loadShader(VkDevice device, const std::string& filename, VkShaderStageFlagBits stage);

	void readDirectory(const std::string& directory, const std::string &pattern, std::map<std::string, std::string> &filelist, bool recursive);

	Texture2D GenerateBRDFLUT(VkDevice device, VkQueue queue, VkPipelineCache pipelineCache, VulkanDevice& vulkanDevice);
}
