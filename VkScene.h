// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkModel.h"
#include "VkCubeMap.h"
#include "VkBuffer.h"

namespace Vk {
    struct Settings;
    class Main;
    class CommandBuffer;
    class FrameBuffer;

    struct SceneShaderValues {
        glm::vec4 lightDir{};
        float exposure = 4.5f;
        float gamma = 2.2f;
        float prefilteredCubeMipLevels = 0.0f;
        float scaleIBLAmbient = 1.0f;
        float debugViewInputs = 0;
        float debugViewEquation = 0;
    };

    struct SceneUniformData {
        glm::mat4 projection{ glm::identity<glm::mat4>() };
        glm::mat4 model{ glm::identity<glm::mat4>() };
        glm::mat4 view{ glm::identity<glm::mat4>() };
        glm::vec3 camPos{};
    };

    struct DescriptorSetLayouts {
        VkDescriptorSetLayout scene = VK_NULL_HANDLE;
        VkDescriptorSetLayout material = VK_NULL_HANDLE;
        VkDescriptorSetLayout node = VK_NULL_HANDLE;
    };

    struct Pipelines {
        VkPipeline pbr = VK_NULL_HANDLE;
        VkPipeline pbrAlphaBlend = VK_NULL_HANDLE;
    };

    using VkDescriptorSets = std::vector<VkDescriptorSet>;

    class Scene {
    public:
        bool                        Initialize(Main& main, const Settings& settings);
        void                        Release(VkDevice device);

        void                        UpdateUniformDatas(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& cameraPos, const glm::vec4& lightDir);
        void                        RecordBuffers(Main& main, const Settings& settings, CommandBuffer& cmdBuffers, FrameBuffer& frameBuffers);

        void                        OnUniformBufferSets(uint32_t currentBuffer);

    private:
        CubeMap                     _cubeMap;
        Model                       _scene;

        SceneShaderValues           _sceneShaderValue;
        SceneUniformData            _sceneUniData;
        Buffers                     _sceneUniBufs;
        Buffers                     _sceneShaderValueUniBufs;

        VkDescriptorPool            _descriptorPool = VK_NULL_HANDLE;
        DescriptorSetLayouts        _descriptorSetLayouts;
        VkDescriptorSets            _sceneDescSets;

        VkPipelineLayout            _pipelineLayout = VK_NULL_HANDLE;
        Pipelines                   _pipelines;
    };
}
