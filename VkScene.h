// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkModel.h"
#include "VkCubeMap.h"
#include "VkBuffer.h"

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

    struct ShaderValuesParams
    {
        glm::vec4 lightDir = {};
        float exposure = 4.5f;
        float gamma = 2.2f;
        float prefilteredCubeMipLevels = 0.0f;
        float scaleIBLAmbient = 1.0f;
        float debugViewInputs = 0;
        float debugViewEquation = 0;
    };

    struct UniformBufferSet
    {
        Buffer scene;
        Buffer skybox;
        Buffer params;
    };

    struct UBOMatrices
    {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        glm::vec3 camPos;
    };

    struct DescriptorSets
    {
        VkDescriptorSet scene = VK_NULL_HANDLE;
        VkDescriptorSet skybox = VK_NULL_HANDLE;
    };

    struct DescriptorSetLayouts
    {
        VkDescriptorSetLayout scene = VK_NULL_HANDLE;
        VkDescriptorSetLayout material = VK_NULL_HANDLE;
        VkDescriptorSetLayout node = VK_NULL_HANDLE;
    };

    struct PushConstBlockMaterial
    {
        glm::vec4 baseColorFactor;
        glm::vec4 emissiveFactor;
        glm::vec4 diffuseFactor;
        glm::vec4 specularFactor;
        float workflow = 0.0f;
        int colorTextureSet = 0;
        int PhysicalDescriptorTextureSet = 0;
        int normalTextureSet = 0;
        int occlusionTextureSet = 0;
        int emissiveTextureSet = 0;
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.0f;
        float alphaMask = 0.0f;
        float alphaMaskCutoff = 0.0f;
    };

    struct Pipelines
    {
        VkPipeline skybox = VK_NULL_HANDLE;
        VkPipeline pbr = VK_NULL_HANDLE;
        VkPipeline pbrAlphaBlend = VK_NULL_HANDLE;
    };

    using UniformBufferSets = std::vector<UniformBufferSet>;
    using DescriptorSetsArr = std::vector<DescriptorSets>;

    class Scene
    {
    public:
        bool                        Initialize(Main& main, const Settings& settings);
        void                        Release(VkDevice device);

        void                        UpdateUniformDatas(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& cameraPos, const glm::vec4& lightDir);
        void                        RecordBuffers(Main& main, const Settings& settings, CommandBuffer& cmdBuffers, FrameBuffer& frameBuffers);

        void                        OnUniformBufferSets(uint32_t currentBuffer);

    private:
        Model                       _scene;
        CubeMap                     _cubeMap;
        UniformBufferSets           _uniformBuffers;
        ShaderValuesParams          _shaderValuesParams;
        UBOMatrices                 _shaderValuesScene;
        UBOMatrices                 _shaderValuesSkybox;
        DescriptorSetsArr           _descriptorSets;
        VkDescriptorPool            _descriptorPool = VK_NULL_HANDLE;
        DescriptorSetLayouts        _descriptorSetLayouts;
        PushConstBlockMaterial      _pushConstBlockMaterial;
        VkPipelineLayout            _pipelineLayout = VK_NULL_HANDLE;
        Pipelines                   _pipelines;
    };
}
