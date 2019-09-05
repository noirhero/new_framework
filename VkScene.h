// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkCommon.h"

namespace Vk
{
	struct VulkanDevice;
	class VulkanSwapChain;
	class CommandBuffer;

	void loadAssets(VulkanDevice& vulkanDevice, VkQueue queue, VkPipelineCache pipelineCache);
	void releaseAssets(VkDevice device);
	void generateBRDFLUT(VulkanDevice& vulkanDevice, VkQueue queue, VkPipelineCache pipelineCache);
	void generateCubemaps(VulkanDevice& vulkanDevice, VkQueue queue, VkPipelineCache pipelineCache);
	void UpdateSceneUniformBuffer(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& camPos);
	void PrepreSceneUniformBuffes(VulkanDevice& vulkanDevice, VulkanSwapChain& swapChain);
	void setupDescriptors(VkDevice device, VulkanSwapChain& swapChain);
	void preparePipelines(const Settings& settings, VkDevice device, VkRenderPass renderPass, VkPipelineCache pipelineCache);
	void recordCommandBuffers(const Settings& settings, VkRenderPass renderPass, CommandBuffer& cmdBuffers);
	void UpdateLightSourceDirection(const glm::vec4& lightDir);
	void UpdateUniformBuffetSet(uint32_t currentBuffer);
}
