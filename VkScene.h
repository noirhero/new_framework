// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkModel.h"
#include "VkCubeMap.h"

namespace Vk
{
	struct Settings;
	class Main;
	class CommandBuffer;
	class FrameBuffer;

	enum class PBRWorkflow : uint8_t
	{
		METALLIC_ROUGHNESS = 0,
		SPECULAR_GLOSINESS = 1
	};

	class Scene
	{
	public:
		bool Initialize(Main& main, const Settings& settings);
		void Release(VkDevice device);

		void UpdateUniformDatas(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& cameraPos, const glm::vec4& lightDir);
		void RecordBuffers(Main& main, const Settings& settings, CommandBuffer& cmdBuffers, FrameBuffer& frameBuffers);

		void OnUniformBufferSets(uint32_t currentBuffer);

    private:
        Model   _scene;
        CubeMap _cubeMap;
	};
}
