// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VkTexture.h"
#include "VkBuffer.h"

namespace Vk
{
    struct Model;
    class Main;

    class CubeMap
    {
    public:
        bool                        Initialize(Main& main, const std::string& assetpath);
        void                        Release(VkDevice device);

        constexpr TextureCubeMap&   GetEnvironment() { return _environmentCube; }
        constexpr TextureCubeMap&   GetIrradiance() { return _irradianceCube; }
        constexpr TextureCubeMap&   GetPrefiltered() { return _prefilteredCube; }
        constexpr float             GetPrefilteredCubeMipLevels() const { return _prefilteredCubeMipLevels; }

        Model&                      GetSkybox() const;
        Buffers&                    GetSkyboxUniformBuffers() const;
        VkDescriptorSet*            GetSkyboxDescSets() const;

        void                        CreateAndSetupSkyboxDescriptorSet(Main& main, Buffers& shaderParamUniBufs, VkDescriptorPool descPool, VkDescriptorSetLayout descSetLayout);
        void                        PrepareSkyboxPipeline(Main& main, VkGraphicsPipelineCreateInfo& info);
        void                        UpdateSkyboxUniformData(const glm::mat4& view, const glm::mat4& perspective);
        void                        OnSkyboxUniformBuffrSet(uint32_t currentBuffer);
        void                        RenderSkybox(uint32_t currentBuffer, VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout);

    private:
        TextureCubeMap              _environmentCube;
        TextureCubeMap              _irradianceCube;
        TextureCubeMap              _prefilteredCube;

        float                       _prefilteredCubeMipLevels = 0.0f;
    };
}
