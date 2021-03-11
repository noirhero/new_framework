// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "render.h"

#include "renderer_common.h"
#include "allocator_cpu.h"
#include "logical.h"

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
    SimpleRenderPass::~SimpleRenderPass() {
        _frameBuffer.reset();

        if (VK_NULL_HANDLE != _handle) {
            vkDestroyRenderPass(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    SimpleRenderPassUPtr AllocateSimpleRenderPass() {
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

        return std::make_unique<SimpleRenderPass>(renderPass, std::move(swapChainFrameBuffer));
    }
}
