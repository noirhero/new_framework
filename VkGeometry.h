// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VulkanModel.h"
#include "VkCubeMap.h"
#include "VkBuffer.h"

namespace Vk {
	class Main;
	class CommandBuffer;
	class FrameBuffer;

	struct GeometryShaderValues {
		glm::vec4 lightDir{};
		float exposure = 4.5f;
		float gamma = 2.2f;
		float prefilteredCubeMipLevels = 0.0f;
		float scaleIBLAmbient = 1.0f;
		float debugViewInputs = 0;
		float debugViewEquation = 0;
	};

	struct GeometryUniformData {
		glm::mat4 projection{ glm::identity<glm::mat4>() };
		glm::mat4 model{ glm::identity<glm::mat4>() };
		glm::mat4 view{ glm::identity<glm::mat4>() };
		glm::vec3 camPos{};
	};

	using VkDescriptorSets = std::vector<VkDescriptorSet>;

	class Geometry {
	public:
		bool                        Initialize(Main& main);
		void                        Release(VkDevice device);

		void                        UpdateUniformDatas(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& cameraPos, const glm::vec4& lightDir);
		void                        RecordBuffers(Main& main, CommandBuffer& cmdBuffers, FrameBuffer& frameBuffers);

		void                        OnUniformBufferSets(uint32_t currentBuffer);

	private:
		void                        LoadScene(Main& main, std::string&& filename);
		void                        CreateDescriptorPool(Main& main);
		void                        CreateAndSetupSceneDescriptorSet(Main& main);
		void                        CreateAndSetupMaterialDescriptorSet(Main& main);
		void                        CreateAndSetupNodeDescriptorSet(Main& main);
		void                        CreatePipelines(Main& main);

		CubeMap                     _cubeMap;
		Model                       _scene;

		GeometryShaderValues           _sceneShaderValue;
		GeometryUniformData            _sceneUniData;
		Buffers                     _sceneUniBufs;
		Buffers                     _sceneShaderValueUniBufs;

		VkDescriptorPool            _descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout       _sceneDescLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout       _materialDescLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout       _nodeDescLayout = VK_NULL_HANDLE;
		VkDescriptorSets            _sceneDescSets;

		VkPipelineLayout            _pipelineLayout = VK_NULL_HANDLE;
		VkPipeline                  _opaquePipeline = VK_NULL_HANDLE;
		VkPipeline                  _alphaBlendPipeline = VK_NULL_HANDLE;
	};
}
