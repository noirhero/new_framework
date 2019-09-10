// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkScene.h"

#include "VkCommon.h"
#include "VkMain.h"
#include "VkDevice.h"
#include "VkSwapChain.h"
#include "VkUtils.h"

namespace Vk {
    enum class PBRWorkflow : uint8_t {
        METALLIC_ROUGHNESS = 0,
        SPECULAR_GLOSINESS = 1
    };

    struct MaterialConstantData {
        glm::vec4 baseColorFactor{};
        glm::vec4 emissiveFactor{};
        glm::vec4 diffuseFactor{};
        glm::vec4 specularFactor{};
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

    constexpr bool displayBackground = true;
    const std::string assetpath{ "./../data/" };

    Texture2D empty;
    Texture2D lutBrdf;

    void CheckToDataPath() {
        struct stat info {};

        if (0 != stat(assetpath.c_str(), &info)) {
            std::string msg = "Could not locate asset path in \"" + assetpath + "\".\nMake sure binary is run from correct relative directory!";
            std::cerr << msg << std::endl;
            exit(-1);
        }
    }

    void LoadEmptyTexture(Main& main, std::string&& filename) {
        if (VK_NULL_HANDLE != empty.image)
            empty.destroy();

        empty.loadFromFile(filename, VK_FORMAT_R8G8B8A8_UNORM, &main.GetVulkanDevice(), main.GetGPUQueue());
    }

    void GenerateBRDFLUT(Main& main) {
        if (VK_NULL_HANDLE != lutBrdf.image)
            lutBrdf.destroy();

        VkDevice device = main.GetDevice();
        VkQueue queue = main.GetGPUQueue();
        VkPipelineCache pipelineCache = main.GetPipelineCache();
        VulkanDevice& vulkanDevice = main.GetVulkanDevice();

        const auto tStart = std::chrono::high_resolution_clock::now();

        lutBrdf = GenerateBRDFLUT(device, queue, pipelineCache, vulkanDevice);

        const auto tEnd = std::chrono::high_resolution_clock::now();
        const auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        std::cout << "Generating BRDF LUT took " << tDiff << " ms" << std::endl;
    }

    void LoadScene(Model& scene, Main& main, std::string&& filename) {
        std::cout << "Loading scene from " << filename << std::endl;

        scene.destroy(main.GetDevice());
        scene.loadFromFile(filename, &main.GetVulkanDevice(), main.GetGPUQueue());
    }

    void SetupNodeDescriptorSet(Node& node, VkDevice device, const VkDescriptorSetAllocateInfo& descSetInfo) {
        if (node.mesh) {
            VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descSetInfo, &node.mesh->uniformBuffer.descriptorSet));

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = node.mesh->uniformBuffer.descriptorSet;
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.pBufferInfo = &node.mesh->uniformBuffer.descriptor;

            vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
        }

        for (auto& child : node.children)
            SetupNodeDescriptorSet(*child, device, descSetInfo);
    }

    void SetupDescriptors(Model& scene, CubeMap& cubeMap, Main& main, 
                          VkDescriptorPool& descriptorPool, DescriptorSetLayouts& descriptorSetLayouts,
                          VkDescriptorSets& sceneDescSets, Buffers& sceneUniBufs, Buffers& shaderParamUniBufs) {
        VkDevice device = main.GetDevice();
        const auto imageCount = main.GetVulkanSwapChain().imageCount;

        sceneDescSets.resize(imageCount);

        /*
            Descriptor Pool
        */
        uint32_t imageSamplerCount = 0;
        uint32_t materialCount = 0;
        uint32_t meshCount = 0;

        // Environment samplers (radiance, irradiance, brdf lut)
        imageSamplerCount += 3;

        const std::vector<Model*> modellist = { &cubeMap.GetSkybox(), &scene };
        for (auto& model : modellist) {
            for (auto& material : model->materials) {
                imageSamplerCount += 5;
                materialCount++;
            }
            for (auto node : model->linearNodes) {
                if (node->mesh) {
                    meshCount++;
                }
            }
        }

        const std::vector<VkDescriptorPoolSize> poolSizes =
        {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (4 + meshCount) * imageCount },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageSamplerCount * imageCount }
        };

        VkDescriptorPoolCreateInfo descriptorPoolCI{};
        descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCI.poolSizeCount = 2;
        descriptorPoolCI.pPoolSizes = poolSizes.data();
        descriptorPoolCI.maxSets = (2 + materialCount + meshCount) * imageCount;
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

        /*
            Descriptor sets
        */

        // Scene (matrices and environment maps)
        {
            const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
            {
                { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            };

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
            descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
            descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.scene));

            for (auto i = 0; i < sceneDescSets.size(); i++) {
                VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
                descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                descriptorSetAllocInfo.descriptorPool = descriptorPool;
                descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.scene;
                descriptorSetAllocInfo.descriptorSetCount = 1;
                VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &sceneDescSets[i]));

                std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};

                writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSets[0].descriptorCount = 1;
                writeDescriptorSets[0].dstSet = sceneDescSets[i];
                writeDescriptorSets[0].dstBinding = 0;
                writeDescriptorSets[0].pBufferInfo = &sceneUniBufs[i].descriptor;

                writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSets[1].descriptorCount = 1;
                writeDescriptorSets[1].dstSet = sceneDescSets[i];
                writeDescriptorSets[1].dstBinding = 1;
                writeDescriptorSets[1].pBufferInfo = &shaderParamUniBufs[i].descriptor;

                writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSets[2].descriptorCount = 1;
                writeDescriptorSets[2].dstSet = sceneDescSets[i];
                writeDescriptorSets[2].dstBinding = 2;
                writeDescriptorSets[2].pImageInfo = &cubeMap.GetIrradiance().descriptor;

                writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSets[3].descriptorCount = 1;
                writeDescriptorSets[3].dstSet = sceneDescSets[i];
                writeDescriptorSets[3].dstBinding = 3;
                writeDescriptorSets[3].pImageInfo = &cubeMap.GetPrefiltered().descriptor;

                writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSets[4].descriptorCount = 1;
                writeDescriptorSets[4].dstSet = sceneDescSets[i];
                writeDescriptorSets[4].dstBinding = 4;
                writeDescriptorSets[4].pImageInfo = &lutBrdf.descriptor;

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
            }
        }

        // Material (samplers)
        {
            const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
            {
                { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
                { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
            };

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
            descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
            descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

            VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.material));

            // Per-Material descriptor sets
            for (auto& material : scene.materials) {
                VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
                descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                descriptorSetAllocInfo.descriptorPool = descriptorPool;
                descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.material;
                descriptorSetAllocInfo.descriptorSetCount = 1;
                VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &material.descriptorSet));

                std::vector<VkDescriptorImageInfo> imageDescriptors =
                {
                    empty.descriptor,
                    empty.descriptor,
                    material.normalTexture ? material.normalTexture->descriptor : empty.descriptor,
                    material.occlusionTexture ? material.occlusionTexture->descriptor : empty.descriptor,
                    material.emissiveTexture ? material.emissiveTexture->descriptor : empty.descriptor
                };

                // TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

                if (material.pbrWorkflows.metallicRoughness) {
                    if (material.baseColorTexture)
                        imageDescriptors[0] = material.baseColorTexture->descriptor;

                    if (material.metallicRoughnessTexture)
                        imageDescriptors[1] = material.metallicRoughnessTexture->descriptor;
                }

                if (material.pbrWorkflows.specularGlossiness) {
                    if (material.extension.diffuseTexture)
                        imageDescriptors[0] = material.extension.diffuseTexture->descriptor;

                    if (material.extension.specularGlossinessTexture)
                        imageDescriptors[1] = material.extension.specularGlossinessTexture->descriptor;
                }

                std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
                for (size_t i = 0; i < imageDescriptors.size(); i++) {
                    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    writeDescriptorSets[i].descriptorCount = 1;
                    writeDescriptorSets[i].dstSet = material.descriptorSet;
                    writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
                    writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
                }

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
            }

            // Model node (matrices)
            {
                const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
                {
                    { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
                };

                VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
                descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
                descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

                VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.node));

                // Per-Node descriptor set
                VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
                descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                descriptorSetAllocInfo.descriptorPool = descriptorPool;
                descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.node;
                descriptorSetAllocInfo.descriptorSetCount = 1;

                for (auto& node : scene.nodes)
                    SetupNodeDescriptorSet(*node, device, descriptorSetAllocInfo);
            }

        }
    }

    void PreparePipelines(const Settings& settings, Main& main, CubeMap& cubeMap, DescriptorSetLayouts& descriptorSetLayouts, VkPipelineLayout& pipelineLayout, Pipelines& pipelines) {
        VkDevice device = main.GetDevice();
        VkRenderPass renderPass = main.GetRenderPass();
        VkPipelineCache pipelineCache = main.GetPipelineCache();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
        inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
        rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCI.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState blendAttachmentState{};
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments = &blendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
        depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCI.depthTestEnable = VK_FALSE;
        depthStencilStateCI.depthWriteEnable = VK_FALSE;
        depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateCI.front = depthStencilStateCI.back;
        depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount = 1;

        VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
        multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        if (settings.multiSampling) {
            multisampleStateCI.rasterizationSamples = settings.sampleCount;
        }

        std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicStateCI{};
        dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
        dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

        // Pipeline layout
        const std::vector<VkDescriptorSetLayout> setLayouts =
        {
            descriptorSetLayouts.scene, descriptorSetLayouts.material, descriptorSetLayouts.node
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutCI.pSetLayouts = setLayouts.data();
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.size = sizeof(MaterialConstantData);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineLayoutCI.pushConstantRangeCount = 1;
        pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

        // Vertex bindings an attributes
        const VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes =
        {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6 },
            { 3, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 8 },
            { 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 10 },
            { 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 14 }
        };

        VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
        vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCI.vertexBindingDescriptionCount = 1;
        vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
        vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

        // Pipelines
        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.layout = pipelineLayout;
        pipelineCI.renderPass = renderPass;
        pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        pipelineCI.pVertexInputState = &vertexInputStateCI;
        pipelineCI.pRasterizationState = &rasterizationStateCI;
        pipelineCI.pColorBlendState = &colorBlendStateCI;
        pipelineCI.pMultisampleState = &multisampleStateCI;
        pipelineCI.pViewportState = &viewportStateCI;
        pipelineCI.pDepthStencilState = &depthStencilStateCI;
        pipelineCI.pDynamicState = &dynamicStateCI;

        if (settings.multiSampling)
            multisampleStateCI.rasterizationSamples = settings.sampleCount;

        // Skybox pipeline (background cube)
        cubeMap.PrepareSkyboxPipeline(main, pipelineCI);

        // PBR pipeline
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
        {
            loadShader(device, "pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            loadShader(device, "pbr_khr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
        };
        pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCI.pStages = shaderStages.data();

        depthStencilStateCI.depthWriteEnable = VK_TRUE;
        depthStencilStateCI.depthTestEnable = VK_TRUE;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbr));

        rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
        blendAttachmentState.blendEnable = VK_TRUE;
        blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbrAlphaBlend));

        for (auto shaderStage : shaderStages)
            vkDestroyShaderModule(device, shaderStage.module, nullptr);
    }

    void RenderNode(Node* node, uint32_t cbIndex, Material::AlphaMode alphaMode, CommandBuffer& cmdBuffers, VkDescriptorSets& sceneDescSets, VkPipelineLayout& pipelineLayout) {
        if (node->mesh) {
            // Render mesh primitives
            for (Primitive* primitive : node->mesh->primitives) {
                if (primitive->material.alphaMode == alphaMode) {

                    const std::vector<VkDescriptorSet> descriptorsets =
                    {
                        sceneDescSets[cbIndex],
                        primitive->material.descriptorSet,
                        node->mesh->uniformBuffer.descriptorSet,
                    };
                    vkCmdBindDescriptorSets(cmdBuffers.Get(cbIndex), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorsets.size()), descriptorsets.data(), 0, NULL);

                    // Pass material parameters as push constants
                    MaterialConstantData pushConstBlockMaterial{};
                    pushConstBlockMaterial.emissiveFactor = primitive->material.emissiveFactor;
                    // To save push constant space, availabilty and texture coordiante set are combined
                    // -1 = texture not used for this material, >= 0 texture used and index of texture coordinate set
                    pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                    pushConstBlockMaterial.normalTextureSet = primitive->material.normalTexture != nullptr ? primitive->material.texCoordSets.normal : -1;
                    pushConstBlockMaterial.occlusionTextureSet = primitive->material.occlusionTexture != nullptr ? primitive->material.texCoordSets.occlusion : -1;
                    pushConstBlockMaterial.emissiveTextureSet = primitive->material.emissiveTexture != nullptr ? primitive->material.texCoordSets.emissive : -1;
                    pushConstBlockMaterial.alphaMask = static_cast<float>(primitive->material.alphaMode == Material::ALPHAMODE_MASK);
                    pushConstBlockMaterial.alphaMaskCutoff = primitive->material.alphaCutoff;

                    // TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

                    if (primitive->material.pbrWorkflows.metallicRoughness) {
                        // Metallic roughness workflow
                        pushConstBlockMaterial.workflow = static_cast<float>(PBRWorkflow::METALLIC_ROUGHNESS);
                        pushConstBlockMaterial.baseColorFactor = primitive->material.baseColorFactor;
                        pushConstBlockMaterial.metallicFactor = primitive->material.metallicFactor;
                        pushConstBlockMaterial.roughnessFactor = primitive->material.roughnessFactor;
                        pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.metallicRoughnessTexture != nullptr ? primitive->material.texCoordSets.metallicRoughness : -1;
                        pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                    }

                    if (primitive->material.pbrWorkflows.specularGlossiness) {
                        // Specular glossiness workflow
                        pushConstBlockMaterial.workflow = static_cast<float>(PBRWorkflow::SPECULAR_GLOSINESS);
                        pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.extension.specularGlossinessTexture != nullptr ? primitive->material.texCoordSets.specularGlossiness : -1;
                        pushConstBlockMaterial.colorTextureSet = primitive->material.extension.diffuseTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
                        pushConstBlockMaterial.diffuseFactor = primitive->material.extension.diffuseFactor;
                        pushConstBlockMaterial.specularFactor = glm::vec4(primitive->material.extension.specularFactor, 1.0f);
                    }

                    vkCmdPushConstants(cmdBuffers.Get(cbIndex), pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialConstantData), &pushConstBlockMaterial);

                    if (primitive->hasIndices)
                        vkCmdDrawIndexed(cmdBuffers.Get(cbIndex), primitive->indexCount, 1, primitive->firstIndex, 0, 0);
                    else
                        vkCmdDraw(cmdBuffers.Get(cbIndex), primitive->vertexCount, 1, 0, 0);
                }
            }
        };

        for (auto child : node->children) {
            RenderNode(child, cbIndex, alphaMode, cmdBuffers, sceneDescSets, pipelineLayout);
        }
    }

    bool Scene::Initialize(Main& main, const Settings& settings) {
        CheckToDataPath();

        LoadEmptyTexture(main, assetpath + "textures/empty.ktx");
        GenerateBRDFLUT(main);

        VulkanDevice& vulkanDevice = main.GetVulkanDevice();
        const auto imagecount = main.GetVulkanSwapChain().imageCount;

        LoadScene(_scene, main, assetpath + "models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
        _sceneUniBufs.resize(imagecount);
        for (auto& uniformBuffer : _sceneUniBufs) {
            uniformBuffer.create(&vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(SceneShaderValues));
        }

        _sceneShaderValueUniBufs.resize(imagecount);
        for (auto& uniformBuffer : _sceneShaderValueUniBufs) {
            uniformBuffer.create(&vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(SceneUniformData));
        }

        _cubeMap.Initialize(main, assetpath);

        SetupDescriptors(_scene, _cubeMap, main, _descriptorPool, _descriptorSetLayouts, _sceneDescSets, _sceneUniBufs, _sceneShaderValueUniBufs);
        _cubeMap.CreateAndSetupSkyboxDescriptorSet(main, _sceneShaderValueUniBufs, _descriptorPool, _descriptorSetLayouts.scene);


        PreparePipelines(settings, main, _cubeMap, _descriptorSetLayouts, _pipelineLayout, _pipelines);

        _sceneShaderValue.prefilteredCubeMipLevels = _cubeMap.GetPrefilteredCubeMipLevels();
        return true;
    }

    void Scene::Release(VkDevice device) {
        vkDestroyPipeline(device, _pipelines.pbr, nullptr);
        vkDestroyPipeline(device, _pipelines.pbrAlphaBlend, nullptr);

        vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);

        vkDestroyDescriptorSetLayout(device, _descriptorSetLayouts.scene, nullptr);
        vkDestroyDescriptorSetLayout(device, _descriptorSetLayouts.material, nullptr);
        vkDestroyDescriptorSetLayout(device, _descriptorSetLayouts.node, nullptr);

        vkDestroyDescriptorPool(device, _descriptorPool, nullptr);

        for (auto& buffer : _cubeMap.GetSkyboxUniformBuffers()) {
            buffer.destroy();
        }
        _cubeMap.Release(device);

        for (auto& buffer : _sceneShaderValueUniBufs) {
            buffer.destroy();
        }
        for (auto& buffer : _sceneUniBufs) {
            buffer.destroy();
        }
        _scene.destroy(device);

        lutBrdf.destroy();
        empty.destroy();
    }

    void Scene::UpdateUniformDatas(const glm::mat4& view, const glm::mat4& perspective, const glm::vec3& cameraPos, const glm::vec4& lightDir) {
        // Scene
        _sceneUniData.projection = perspective;
        _sceneUniData.view = view;

        // Center and scale model
        const float scale = (1.0f / std::max(_scene.aabb[0][0], std::max(_scene.aabb[1][1], _scene.aabb[2][2]))) * 0.5f;
        glm::vec3 translate = -glm::vec3(_scene.aabb[3][0], _scene.aabb[3][1], _scene.aabb[3][2]);
        translate += -0.5f * glm::vec3(_scene.aabb[0][0], _scene.aabb[1][1], _scene.aabb[2][2]);

        _sceneUniData.model = glm::mat4(1.0f);
        _sceneUniData.model[0][0] = scale;
        _sceneUniData.model[1][1] = scale;
        _sceneUniData.model[2][2] = scale;
        _sceneUniData.model = glm::translate(_sceneUniData.model, translate);

        _sceneUniData.camPos = cameraPos;

        _sceneShaderValue.lightDir = lightDir;

        // Skybox
        _cubeMap.UpdateSkyboxUniformData(view, perspective);
    }

    void Scene::RecordBuffers(Main& main, const Settings& settings, CommandBuffer& cmdBuffers, FrameBuffer& frameBuffers) {
        VkCommandBufferBeginInfo cmdBufferBeginInfo{};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkClearValue clearValues[3];
        if (settings.multiSampling) {
            clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            clearValues[2].depthStencil = { 1.0f, 0 };
        }
        else {
            clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            clearValues[1].depthStencil = { 1.0f, 0 };
        }

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = main.GetRenderPass();
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = settings.width;
        renderPassBeginInfo.renderArea.extent.height = settings.height;
        renderPassBeginInfo.clearValueCount = settings.multiSampling ? 3 : 2;
        renderPassBeginInfo.pClearValues = clearValues;

        for (decltype(cmdBuffers.Count()) i = 0; i < cmdBuffers.Count(); ++i) {
            renderPassBeginInfo.framebuffer = frameBuffers.Get(i);

            VkCommandBuffer currentCB = cmdBuffers.Get(i);

            VK_CHECK_RESULT(vkBeginCommandBuffer(currentCB, &cmdBufferBeginInfo));
            vkCmdBeginRenderPass(currentCB, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.width = (float)settings.width;
            viewport.height = (float)settings.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(currentCB, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = { settings.width, settings.height };
            vkCmdSetScissor(currentCB, 0, 1, &scissor);

            VkDeviceSize offsets[1] = { 0 };

            if (true == displayBackground) {
                _cubeMap.RenderSkybox(i, currentCB, _pipelineLayout);
            }

            vkCmdBindPipeline(currentCB, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.pbr);

            Model& model = _scene;

            vkCmdBindVertexBuffers(currentCB, 0, 1, &model.vertices.buffer, offsets);
            if (model.indices.buffer != VK_NULL_HANDLE)
                vkCmdBindIndexBuffer(currentCB, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Opaque primitives first
            for (auto node : model.nodes)
                RenderNode(node, static_cast<uint32_t>(i), Material::ALPHAMODE_OPAQUE, cmdBuffers, _sceneDescSets, _pipelineLayout);

            // Alpha masked primitives
            for (auto node : model.nodes)
                RenderNode(node, static_cast<uint32_t>(i), Material::ALPHAMODE_MASK, cmdBuffers, _sceneDescSets, _pipelineLayout);

            // Transparent primitives
            // TODO: Correct depth sorting
            vkCmdBindPipeline(currentCB, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.pbrAlphaBlend);
            for (auto node : model.nodes)
                RenderNode(node, static_cast<uint32_t>(i), Material::ALPHAMODE_BLEND, cmdBuffers, _sceneDescSets, _pipelineLayout);

            vkCmdEndRenderPass(currentCB);
            VK_CHECK_RESULT(vkEndCommandBuffer(currentCB));
        }
    }

    void Scene::OnUniformBufferSets(uint32_t currentBuffer) {
        constexpr auto uniDataSize = sizeof(SceneUniformData);
        memcpy_s(_sceneUniBufs[currentBuffer].mapped, uniDataSize, &_sceneUniData, uniDataSize);

        constexpr auto shaderValueSize = sizeof(SceneShaderValues);
        memcpy_s(_sceneShaderValueUniBufs[currentBuffer].mapped, shaderValueSize, &_sceneShaderValue, shaderValueSize);

        _cubeMap.OnSkyboxUniformBuffrSet(currentBuffer);
    }
}
