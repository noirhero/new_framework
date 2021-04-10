// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "render.h"

#include "allocator_cpu.h"
#include "logical.h"
#include "command.h"
#include "descriptor.h"
#include "shader.h"
#include "buffer.h"
#include "../data/model.h"

namespace Render {
    using namespace Renderer;

    // Swap chain frame buffer.
    SwapChainFrameBuffer::~SwapChainFrameBuffer() {
        auto* device = Logical::Device::Get();
        for (auto* buffer : _buffers) {
            vkDestroyFramebuffer(device, buffer, Allocator::CPU());
        }
    }

    SwapChainFrameBufferUPtr AllocateSwapChainFrameBuffer(VkRenderPass renderPass) {
        auto* device = Logical::Device::Get();
        auto& swapChain = Logical::SwapChain::Get();

        VkImageView attachments[2]{ VK_NULL_HANDLE, swapChain.depthImageView };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass;
        info.attachmentCount = 2/* color, depth */;
        info.pAttachments = attachments;
        info.width = swapChain.width;
        info.height = swapChain.height;
        info.layers = 1;

        VkFrameBuffers frameBuffers;
        for (auto* imageView : swapChain.imageViews) {
            attachments[0] = imageView;

            VkFramebuffer frameBuffer = VK_NULL_HANDLE;
            if (VK_SUCCESS != vkCreateFramebuffer(device, &info, Allocator::CPU(), &frameBuffer)) {
                for (auto* eachFrameBuffer : frameBuffers) {
                    vkDestroyFramebuffer(device, eachFrameBuffer, Allocator::CPU());
                }

                return nullptr;
            }

            frameBuffers.emplace_back(frameBuffer);
        }

        return std::make_unique<SwapChainFrameBuffer>(std::move(frameBuffers));
    }

    // Simple render pass.
    Pass::~Pass() {
        _frameBuffer.reset();

        if (VK_NULL_HANDLE != _handle) {
            vkDestroyRenderPass(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    std::vector<VkFramebuffer> Pass::GetFrameBuffers() const noexcept {
        if (nullptr == _frameBuffer) {
            return {};
        }
        return _frameBuffer->Get();
    }

    PassUPtr CreateSimpleRenderPass() {
        auto* device = Logical::Device::Get();
        auto& swapChain = Logical::SwapChain::Get();

        constexpr auto descriptionCount = 2/* color, depth */;

        VkAttachmentDescription attachments[descriptionCount]{};
        attachments[0].format = swapChain.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[0].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

        attachments[1].format = swapChain.depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;

        const VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        const VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subPass{};
        subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subPass.colorAttachmentCount = 1;
        subPass.pColorAttachments = &colorRef;
        subPass.pDepthStencilAttachment = &depthRef;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = descriptionCount;
        info.pAttachments = attachments;
        info.subpassCount = 1;
        info.pSubpasses = &subPass;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateRenderPass(device, &info, Allocator::CPU(), &renderPass)) {
            return nullptr;
        }

        auto swapChainFrameBuffer = AllocateSwapChainFrameBuffer(renderPass);
        if (nullptr == swapChainFrameBuffer) {
            vkDestroyRenderPass(device, renderPass, Allocator::CPU());
            return nullptr;
        }

        return std::make_unique<Pass>(renderPass, std::move(swapChainFrameBuffer));
    }

    // Pipeline cache.
    PipelineCache::~PipelineCache() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroyPipelineCache(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    // Pipeline layout.
    PipelineLayout::~PipelineLayout() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroyPipelineLayout(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    // Pipeline.
    Pipeline::~Pipeline() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroyPipeline(Logical::Device::Get(), _handle, Allocator::CPU());
        }

        _layout.reset();
        _cache.reset();
    }

    VkPipelineLayout Pipeline::GetLayout() const noexcept {
        if (nullptr == _layout) {
            return VK_NULL_HANDLE;
        }
        return _layout->Get();;
    }

    constexpr VkPipelineLayoutCreateInfo PipelineLayoutInfo(const VkDescriptorSetLayout* layouts, uint32_t numLayout, const VkPushConstantRange* pushConstants, uint32_t numPushConstant) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0,
            numLayout, layouts, numPushConstant, pushConstants
        };
    }

    constexpr VkPipelineShaderStageCreateInfo ShaderStageInfo(const VkShaderModule module, VkShaderStageFlagBits stage, const char* entryName) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
            stage, module, entryName, nullptr
        };
    }

    constexpr VkPipelineVertexInputStateCreateInfo VertexInputStateInfo(const VkVertexInputBindingDescription* descriptions, uint32_t numDesc, const VkVertexInputAttributeDescription* attributes, uint32_t numAttr) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0,
            numDesc, descriptions, numAttr, attributes
        };
    }

    constexpr VkVertexInputBindingDescription VertexInputBindingDesc(uint32_t stride) {
        return { 0, stride, VK_VERTEX_INPUT_RATE_VERTEX };
    }

    constexpr VkPipelineInputAssemblyStateCreateInfo AssemblyInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
        };
    }

    constexpr VkPipelineRasterizationStateCreateInfo RasterizationInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE,
            VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    constexpr VkPipelineColorBlendAttachmentState ColorBlendInfo() {
        return {
            VK_FALSE,
            VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
            0xf
        };
    }

    constexpr VkPipelineColorBlendStateCreateInfo ColorBlendStateInfo(const VkPipelineColorBlendAttachmentState* colorBlends, uint32_t numColorBlend) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, VK_LOGIC_OP_NO_OP,
            numColorBlend, colorBlends,
            { 1.0f, 1.0f, 1.0f, 1.0f }
        };
    }

    constexpr VkPipelineDynamicStateCreateInfo DynamicStateInfo(const VkDynamicState* dynamicStates, uint32_t numDynamicState) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0,
            numDynamicState, dynamicStates
        };
    }

    constexpr VkPipelineViewportStateCreateInfo ViewportInfo(const VkViewport* viewports, uint32_t numViewport, const VkRect2D* scissors, uint32_t numScissor) {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0,
            numViewport, viewports, numScissor, scissors
        };
    }

    constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
            VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE,
            { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
            { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0, 0, 0 },
            0, 0
        };
    }

    constexpr VkPipelineMultisampleStateCreateInfo MultiSampleInfo() {
        return {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
            VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE
        };
    }

    PipelineUPtr CreateSimplePipeline(const Descriptor::Layout& descLayout, const Shader::Module& vs, const Shader::Module& fs, const Pass& renderPass) {
        auto* device = Logical::Device::Get();

        // Layout.
        const VkPushConstantRange pushConstantRanges[1]{
            { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8/*colorFlag + mixerValue*/ },
        };
        const VkDescriptorSetLayout descLayouts[1] = {
            descLayout.Get()
        };
        auto pipelineLayoutCreateInfo = PipelineLayoutInfo(descLayouts, 1, pushConstantRanges, 1);

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, Allocator::CPU(), &layout)) {
            return nullptr;
        }

        // Cache.
        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkPipelineCache cache = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreatePipelineCache(Logical::Device::Get(), &cacheInfo, Allocator::CPU(), &cache)) {
            vkDestroyPipelineLayout(device, layout, Allocator::CPU());
            return nullptr;
        }

        // Pipeline.
        const char* entryName = "main";
        const auto vertexShaderStageInfo = ShaderStageInfo(vs.Get(), VK_SHADER_STAGE_VERTEX_BIT, entryName);
        const auto fragmentShaderStageInfo = ShaderStageInfo(fs.Get(), VK_SHADER_STAGE_FRAGMENT_BIT, entryName);
        const VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        constexpr auto vertexStride = 16/*x,y,z,w*/ + 16/*r,g,b,a*/ + 8/*u, v*/;
        const auto bindingDescription = VertexInputBindingDesc(vertexStride);

        VkVertexInputAttributeDescription attributeDescriptions[3]{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = sizeof(float) * 4;

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = sizeof(float) * 8;
        const auto vertexInputStateInfo = VertexInputStateInfo(&bindingDescription, 1, attributeDescriptions, _countof(attributeDescriptions));

        const VkDynamicState dynamicState[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        const auto dynamicStateInfo = DynamicStateInfo(dynamicState, 2);

        const VkViewport viewport{};
        const VkRect2D scissor{};
        const auto viewportStateInfo = ViewportInfo(&viewport, 1, &scissor, 1);

        const auto inputAssemblyInfo = AssemblyInfo();
        const auto rasterStateInfo = RasterizationInfo();
        const auto colorBlendAttachmentStateInfo = ColorBlendInfo();
        const auto colorBlendStateInfo = ColorBlendStateInfo(&colorBlendAttachmentStateInfo, 1);
        const auto depthStencilStateInfo = DepthStencilInfo();
        const auto multiSampleStateInfo = MultiSampleInfo();

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.layout = layout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.pVertexInputState = &vertexInputStateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &rasterStateInfo;
        pipelineInfo.pColorBlendState = &colorBlendStateInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pMultisampleState = &multiSampleStateInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pViewportState = &viewportStateInfo;
        pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineInfo.pStages = shaderStageInfos;
        pipelineInfo.stageCount = _countof(shaderStageInfos);
        pipelineInfo.renderPass = renderPass.Get();
        pipelineInfo.subpass = 0;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(device, cache, 1, &pipelineInfo, Allocator::CPU(), &pipeline)) {
            vkDestroyPipelineCache(device, cache, Allocator::CPU());
            vkDestroyPipelineLayout(device, layout, Allocator::CPU());

            return nullptr;
        }

        return std::make_unique<Pipeline>(pipeline, std::make_unique<PipelineLayout>(layout), std::make_unique<PipelineCache>(cache));
    }

    PipelineUPtr CreateVertexDeclToPipeline(const Data::VertexDecl& decl, const Descriptor::Layout& descLayout, const Shader::Module& vs, const Shader::Module& fs, const Pass& renderPass) {
        auto* device = Logical::Device::Get();

        // Layout.
        const VkPushConstantRange pushConstantRanges[1]{
            { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8/*colorFlag + mixerValue*/ },
        };
        const VkDescriptorSetLayout descLayouts[1] = {
            descLayout.Get()
        };
        auto pipelineLayoutCreateInfo = PipelineLayoutInfo(descLayouts, 1, pushConstantRanges, 1);

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, Allocator::CPU(), &layout)) {
            return nullptr;
        }

        // Cache.
        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkPipelineCache cache = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreatePipelineCache(Logical::Device::Get(), &cacheInfo, Allocator::CPU(), &cache)) {
            vkDestroyPipelineLayout(device, layout, Allocator::CPU());
            return nullptr;
        }

        // Pipeline.
        const char* entryName = "main";
        const auto vertexShaderStageInfo = ShaderStageInfo(vs.Get(), VK_SHADER_STAGE_VERTEX_BIT, entryName);
        const auto fragmentShaderStageInfo = ShaderStageInfo(fs.Get(), VK_SHADER_STAGE_FRAGMENT_BIT, entryName);
        const VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        const auto bindingDescription = VertexInputBindingDesc(decl.stride);

        VkVertexInputAttributeDescriptions vertexDecl;
        uint32_t location = 0;
        for (const auto& info : decl.infos) {
            vertexDecl.emplace_back(location++, 0, sizeof(glm::vec2) == info.stride ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT, info.offset);
        }
        const auto vertexInputStateInfo = VertexInputStateInfo(&bindingDescription, 1, vertexDecl.data(), static_cast<uint32_t>(vertexDecl.size()));

        const VkDynamicState dynamicState[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        const auto dynamicStateInfo = DynamicStateInfo(dynamicState, 2);

        const VkViewport viewport{};
        const VkRect2D scissor{};
        const auto viewportStateInfo = ViewportInfo(&viewport, 1, &scissor, 1);

        const auto inputAssemblyInfo = AssemblyInfo();
        const auto rasterStateInfo = RasterizationInfo();
        const auto colorBlendAttachmentStateInfo = ColorBlendInfo();
        const auto colorBlendStateInfo = ColorBlendStateInfo(&colorBlendAttachmentStateInfo, 1);
        const auto depthStencilStateInfo = DepthStencilInfo();
        const auto multiSampleStateInfo = MultiSampleInfo();

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.layout = layout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.pVertexInputState = &vertexInputStateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &rasterStateInfo;
        pipelineInfo.pColorBlendState = &colorBlendStateInfo;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pMultisampleState = &multiSampleStateInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pViewportState = &viewportStateInfo;
        pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineInfo.pStages = shaderStageInfos;
        pipelineInfo.stageCount = _countof(shaderStageInfos);
        pipelineInfo.renderPass = renderPass.Get();
        pipelineInfo.subpass = 0;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateGraphicsPipelines(device, cache, 1, &pipelineInfo, Allocator::CPU(), &pipeline)) {
            vkDestroyPipelineCache(device, cache, Allocator::CPU());
            vkDestroyPipelineLayout(device, layout, Allocator::CPU());

            return nullptr;
        }

        return std::make_unique<Pipeline>(pipeline, std::make_unique<PipelineLayout>(layout), std::make_unique<PipelineCache>(cache));
    }

    // Fill render command.
    void FillSimpleRenderCommand(Pass& renderPass, Command::Pool& cmdPool, Pipeline& pipeline, Descriptor::Layout& descLayout, Buffer::Object& vb, Buffer::Object& ib) {
        auto& swapChain = Logical::SwapChain::Get();

        auto frameBuffers = renderPass.GetFrameBuffers();
        assert(swapChain.imageCount == frameBuffers.size());

        auto cmdBuffers = cmdPool.GetSwapChainFrameCommandBuffers();
        assert(swapChain.imageCount == cmdBuffers.size());

        VkDescriptorSet descSets[1] = { descLayout.GetSet() };
        VkBuffer vbs[1] = { vb.Get() };

        const VkDeviceSize offsets[1]{};
        const VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(swapChain.width), static_cast<float>(swapChain.height), 0.0f, 1.0f };
        const VkRect2D scissor{ {0, 0}, {swapChain.width, swapChain.height} };

        const auto mixerValue = 0.3f;
        uint32_t constants[2] = { 0, 0 };
        memcpy_s(&constants[1], sizeof(float), &mixerValue, sizeof(float));

        VkCommandBufferInheritanceInfo cmdBufInheritInfo{};
        cmdBufInheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.pInheritanceInfo = &cmdBufInheritInfo;

        for (decltype(swapChain.imageCount)i = 0; i < swapChain.imageCount; ++i) {
            // Begin.
            auto* cmdBuffer = cmdBuffers[i];
            VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo));

            // Record.
            VkClearValue clearValues[2]{};
            clearValues[0].color = { 0.27f, 0.39f, 0.49f, 1.0f };
            clearValues[1].depthStencil.depth = 1.0f;
            clearValues[1].depthStencil.stencil = 0;

            VkRenderPassBeginInfo renderPassBegin{};
            renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBegin.renderPass = renderPass.Get();
            renderPassBegin.framebuffer = frameBuffers[i];
            renderPassBegin.renderArea.extent.width = swapChain.width;
            renderPassBegin.renderArea.extent.height = swapChain.height;
            renderPassBegin.clearValueCount = 2;
            renderPassBegin.pClearValues = clearValues;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Get());
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(), 0, 1, descSets, 0, nullptr);

            vkCmdPushConstants(cmdBuffer, pipeline.GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), constants);

            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbs, offsets);
            vkCmdBindIndexBuffer(cmdBuffer, ib.Get(), 0, VK_INDEX_TYPE_UINT16);
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
            vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);

            vkCmdEndRenderPass(cmdBuffer);

            // End.
            vkEndCommandBuffer(cmdBuffer);
        }
    }

    void FillMeshToRenderCommand(Pass& renderPass, Command::Pool& cmdPool, Pipeline& pipeline, Descriptor::Layout& descLayout, Data::Mesh& mesh) {
        auto& swapChain = Logical::SwapChain::Get();

        auto frameBuffers = renderPass.GetFrameBuffers();
        assert(swapChain.imageCount == frameBuffers.size());

        auto cmdBuffers = cmdPool.GetSwapChainFrameCommandBuffers();
        assert(swapChain.imageCount == cmdBuffers.size());

        VkDescriptorSet descSets[1] = { descLayout.GetSet() };
        VkBuffer vbs[1] = { mesh.vb->Get() };

        const VkDeviceSize offsets[1]{};
        const VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(swapChain.width), static_cast<float>(swapChain.height), 0.0f, 1.0f };
        const VkRect2D scissor{ {0, 0}, {swapChain.width, swapChain.height} };

        const auto mixerValue = 0.3f;
        uint32_t constants[2] = { 0, 0 };
        memcpy_s(&constants[1], sizeof(float), &mixerValue, sizeof(float));

        VkCommandBufferInheritanceInfo cmdBufInheritInfo{};
        cmdBufInheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;

        VkCommandBufferBeginInfo cmdBufBeginInfo{};
        cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBeginInfo.pInheritanceInfo = &cmdBufInheritInfo;

        for (decltype(swapChain.imageCount)i = 0; i < swapChain.imageCount; ++i) {
            // Begin.
            auto* cmdBuffer = cmdBuffers[i];
            VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo));

            // Record.
            VkClearValue clearValues[2]{};
            clearValues[0].color = { 0.27f, 0.39f, 0.49f, 1.0f };
            clearValues[1].depthStencil.depth = 1.0f;
            clearValues[1].depthStencil.stencil = 0;

            VkRenderPassBeginInfo renderPassBegin{};
            renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBegin.renderPass = renderPass.Get();
            renderPassBegin.framebuffer = frameBuffers[i];
            renderPassBegin.renderArea.extent.width = swapChain.width;
            renderPassBegin.renderArea.extent.height = swapChain.height;
            renderPassBegin.clearValueCount = 2;
            renderPassBegin.pClearValues = clearValues;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Get());
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(), 0, 1, descSets, 0, nullptr);

            vkCmdPushConstants(cmdBuffer, pipeline.GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), constants);

            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbs, offsets);
            vkCmdBindIndexBuffer(cmdBuffer, mesh.ib->Get(), 0, VK_INDEX_TYPE_UINT16);
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
            vkCmdDrawIndexed(cmdBuffer, mesh.indexCount, 1, 0, 0, 0);

            vkCmdEndRenderPass(cmdBuffer);

            // End.
            vkEndCommandBuffer(cmdBuffer);
        }
    }

    void SimpleRenderPresent(Command::Pool& cmdPool) {
        auto* device = Logical::Device::Get();
        auto* gpuQueue = Logical::Device::GetGPUQueue();
        auto* presentQueue = Logical::Device::GetPresentQueue();
        auto& swapChain = Logical::SwapChain::Get();

        // Acquire image index.
        static uint32_t imageIndex = 0;

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore presentSemaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(device, &semaphoreInfo, Allocator::CPU(), &presentSemaphore);

        VkSemaphore drawingSemaphore = VK_NULL_HANDLE;
        vkCreateSemaphore(device, &semaphoreInfo, Allocator::CPU(), &drawingSemaphore);

        if (VK_SUCCESS != vkAcquireNextImageKHR(device, swapChain.handle, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &imageIndex)) {
            return;
        }

        // Submit.
        VkCommandBuffer cmdBuffers[1] = { cmdPool.GetSwapChainFrameCommandBuffers()[imageIndex] };
        const VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = cmdBuffers;
        submitInfo.pWaitSemaphores = &presentSemaphore;
        submitInfo.pWaitDstStageMask = &submitPipelineStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &drawingSemaphore;

        vkQueueSubmit(gpuQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpuQueue);

        // Present.
        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.swapchainCount = 1;
        present.pSwapchains = &swapChain.handle;
        present.pImageIndices = &imageIndex;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &drawingSemaphore;

        // Queue the image for presentation.
        vkQueuePresentKHR(presentQueue, &present);

        vkDestroySemaphore(device, drawingSemaphore, Allocator::CPU());
        vkDestroySemaphore(device, presentSemaphore, Allocator::CPU());
    }
}
