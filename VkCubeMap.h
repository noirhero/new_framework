// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

#include "VulkanModel.h"
#include "VkTexture.h"
#include "VkBuffer.h"

namespace Vk {
    class Main;

    struct SkyboxUniformData {
        glm::mat4 projection{ glm::identity<glm::mat4>() };
        glm::mat4 model{ glm::identity<glm::mat4>() };
        glm::mat4 view{ glm::identity<glm::mat4>() };
        glm::vec3 camPos{};
    };

    class CubeMap {
    public:
        bool                            Initialize(const Main& main, std::string&& environmentMapPath);
        void                            Release(VkDevice device);

        constexpr TextureCubeMap&       GetEnvironment() { return _environmentCube; }
        constexpr TextureCubeMap&       GetIrradiance() { return _irradianceCube; }
        constexpr TextureCubeMap&       GetPrefiltered() { return _prefilteredCube; }
        constexpr float                 GetPrefilteredCubeMipLevels() const { return _prefilteredCubeMipLevels; }

        constexpr Model&                GetSkybox() { return _skybox; }
        constexpr Buffers&              GetSkyboxUniformBuffers() { return _skyboxUniBufs; }
        VkDescriptorSet*                GetSkyboxDescSets() { return _skyboxDescSets.data(); }

        void                            CreateAndSetupSkyboxDescriptorSet(const Main& main, Buffers& shaderParamUniBufs, VkDescriptorPool descPool, VkDescriptorSetLayout descSetLayout);
        void                            PrepareSkyboxPipeline(const Main& main, VkGraphicsPipelineCreateInfo& info);
        void                            UpdateSkyboxUniformData(const glm::mat4& view, const glm::mat4& perspective);
        void                            OnSkyboxUniformBuffrSet(uint32_t currentBuffer);
        void                            RenderSkybox(uint32_t currentBuffer, VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout);

    private:
        TextureCubeMap                  _environmentCube;
        TextureCubeMap                  _irradianceCube;
        TextureCubeMap                  _prefilteredCube;

        float                           _prefilteredCubeMipLevels = 0.0f;

        Model                           _skybox;
        Buffers                         _skyboxUniBufs;
        SkyboxUniformData               _skyboxUniData;
        std::vector<VkDescriptorSet>    _skyboxDescSets;
        VkPipeline                      _skyboxPipeline = VK_NULL_HANDLE;
    };
}
