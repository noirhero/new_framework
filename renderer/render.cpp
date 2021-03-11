// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "render.h"

#include "renderer_common.h"
#include "allocator_cpu.h"
#include "logical.h"
#include "descriptor.h"
#include "shader.h"
#include "buffer.h"

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

    PipelineUPtr CreateSimplePipeline(const Descriptor::Layout& descLayout, const Shader::Module& vs, const Shader::Module& fs, const Pass& renderPass) {
        auto* device = Logical::Device::Get();

        // Layout.
        VkPushConstantRange pushConstantRanges[1]{};
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = 8/*colorFlag + mixerValue*/;

        VkDescriptorSetLayout descLayouts[1] = { descLayout.Get() };

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = _countof(descLayouts);
        pipelineLayoutCreateInfo.pSetLayouts = descLayouts;
        pipelineLayoutCreateInfo.pushConstantRangeCount = _countof(pushConstantRanges);
        pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

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
        VkPipelineShaderStageCreateInfo vertexShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vs.Get();
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fs.Get();
        fragmentShaderStageInfo.pName = "main";

        const VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        constexpr auto vertexStride = sizeof(16/*x,y,z,w*/ + 16/*r,g,b,a*/ + 8/*u, v*/);

        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = vertexStride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

        VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{};
        vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputStateInfo.vertexAttributeDescriptionCount = _countof(attributeDescriptions);
        vertexInputStateInfo.pVertexAttributeDescriptions = attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterStateInfo{};
        rasterStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterStateInfo.depthClampEnable = VK_FALSE;
        rasterStateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterStateInfo.depthBiasEnable = VK_FALSE;
        rasterStateInfo.depthBiasConstantFactor = 0;
        rasterStateInfo.depthBiasClamp = 0;
        rasterStateInfo.depthBiasSlopeFactor = 0;
        rasterStateInfo.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo[1]{};
        colorBlendAttachmentStateInfo[0].colorWriteMask = 0xf;
        colorBlendAttachmentStateInfo[0].blendEnable = VK_FALSE;
        colorBlendAttachmentStateInfo[0].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentStateInfo[0].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentStateInfo[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentStateInfo[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
        colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateInfo.attachmentCount = 1;
        colorBlendStateInfo.pAttachments = colorBlendAttachmentStateInfo;
        colorBlendStateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateInfo.logicOp = VK_LOGIC_OP_NO_OP;
        colorBlendStateInfo.blendConstants[0] = 1.0f;
        colorBlendStateInfo.blendConstants[1] = 1.0f;
        colorBlendStateInfo.blendConstants[2] = 1.0f;
        colorBlendStateInfo.blendConstants[3] = 1.0f;

        const VkDynamicState dynamicState[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.pDynamicStates = dynamicState;
        dynamicStateInfo.dynamicStateCount = _countof(dynamicState);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<decltype(viewport.width)>(Logical::SwapChain::Get().width);
        viewport.height = static_cast<decltype(viewport.height)>(Logical::SwapChain::Get().height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = { Logical::SwapChain::Get().width, Logical::SwapChain::Get().height };

        VkPipelineViewportStateCreateInfo viewportStateInfo{};
        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissor;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
        depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateInfo.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilStateInfo.back.compareMask = 0;
        depthStencilStateInfo.back.reference = 0;
        depthStencilStateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencilStateInfo.back.writeMask = 0;
        depthStencilStateInfo.minDepthBounds = 0;
        depthStencilStateInfo.maxDepthBounds = 0;
        depthStencilStateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateInfo.front = depthStencilStateInfo.back;

        VkPipelineMultisampleStateCreateInfo multiSampleStateInfo{};
        multiSampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multiSampleStateInfo.pSampleMask = nullptr;
        multiSampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multiSampleStateInfo.sampleShadingEnable = VK_FALSE;
        multiSampleStateInfo.alphaToCoverageEnable = VK_FALSE;
        multiSampleStateInfo.alphaToOneEnable = VK_FALSE;
        multiSampleStateInfo.minSampleShading = 0.0;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
    void FillSimpleRenderCommand(Pass& renderPass, Logical::CommandPool& cmdPool, Pipeline& pipeline, Descriptor::Layout& descLayout, Buffer::Object& vb, Buffer::Object& ib) {
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

    void SimpleRenderPresent(Logical::CommandPool& cmdPool) {
        auto* device = Logical::Device::Get();
        auto* gpuQueue = Logical::Device::GetGPUQueue();
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
        vkQueuePresentKHR(gpuQueue, &present);

        vkDestroySemaphore(device, drawingSemaphore, Allocator::CPU());
        vkDestroySemaphore(device, presentSemaphore, Allocator::CPU());
    }
}
