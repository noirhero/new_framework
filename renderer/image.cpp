// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "image.h"

#include "allocator_cpu.h"
#include "allocator_vma.h"
#include "physical.h"
#include "logical.h"

namespace Image {
    using namespace Renderer;

    // Sampler.
    Sampler::~Sampler() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroySampler(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    SamplerUPtr CreateSimpleLinearSampler() {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.maxAnisotropy = Physical::Device::GetGPU().maxAnisotropy;
        info.anisotropyEnable = 1.0f < info.maxAnisotropy ? VK_TRUE : VK_FALSE;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.unnormalizedCoordinates = VK_FALSE;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.mipLodBias = 0.0f;
        info.minLod = 0.0f;
        info.maxLod = std::numeric_limits<float>::max();

        VkSampler sampler = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateSampler(Logical::Device::Get(), &info, Allocator::CPU(), &sampler)) {
            return nullptr;
        }

        return std::make_unique<Sampler>(sampler);
    }

    // 2D texture.
    Dimension2::~Dimension2() {
        auto* device = Logical::Device::Get();

        if (VK_NULL_HANDLE != _view) {
            vkDestroyImageView(device, _view, Allocator::CPU());
        }

        if (VK_NULL_HANDLE != _image) {
            vmaDestroyImage(Allocator::VMA(), _image, _alloc);
        }
    }

    VkDescriptorImageInfo Dimension2::Information(VkSampler sampler) const noexcept {
        return { sampler, _view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    }

    Dimension2UPtr CreateSimple2D(std::string&& path, Logical::CommandPool& cmdPool) {
        auto image = gli::load(path);
        if (image.empty()) {
            return nullptr;
        }

        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = image.size();
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingCreateInfo{};
        stagingCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stagingCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        struct StagingImage {
            VkBuffer          buffer = VK_NULL_HANDLE;
            VmaAllocation     alloc = VK_NULL_HANDLE;
            VmaAllocationInfo info{};

            ~StagingImage() {
                if (VK_NULL_HANDLE != buffer) {
                    vmaDestroyBuffer(Allocator::VMA(), buffer, alloc);
                }
            }
        };
        StagingImage staging;
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &stagingInfo, &stagingCreateInfo, &staging.buffer, &staging.alloc, &staging.info)) {
            return nullptr;
        }
        memcpy_s(staging.info.pMappedData, image.size(), image.data(), image.size());

        const auto dimension = image.extent();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = dimension.x;
        imageInfo.extent.height = dimension.y;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo imageCreateInfo{};
        imageCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage texture = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &imageInfo, &imageCreateInfo, &texture, &alloc, nullptr)) {
            return nullptr;
        }

        auto* cmdBuf = cmdPool.ImmediatelyBegin();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = texture;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = dimension.x;
        region.imageExtent.height = dimension.y;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(cmdBuf, staging.buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = texture;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        cmdPool.ImmediatelyEndAndSubmit();

        // Image view.
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.image = texture;

        VkImageView view = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateImageView(Logical::Device::Get(), &imageViewInfo, Allocator::CPU(), &view)) {
            vmaDestroyImage(Allocator::VMA(), texture, alloc);
            return nullptr;
        }

        return std::make_unique<Dimension2>(texture, alloc, view);
    }
}
