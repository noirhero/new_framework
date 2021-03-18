// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "image.h"

#include "allocator_cpu.h"
#include "allocator_vma.h"
#include "physical.h"
#include "logical.h"
#include "command.h"

namespace Image {
    using namespace Renderer;

    // Sampler.
    Sampler::~Sampler() {
        if (VK_NULL_HANDLE != _handle) {
            vkDestroySampler(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    VkSamplerCreateInfo SamplerInfo(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW) {
        const auto maxAnisotropy = Physical::Device::GetGPU().property.limits.maxSamplerAnisotropy;
        const VkBool32 enabledAnisotropy = 1.0f >= maxAnisotropy ? VK_FALSE : VK_TRUE;
        return {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0,
            magFilter, minFilter, VK_SAMPLER_MIPMAP_MODE_LINEAR, modeU, modeV, modeW,
            0.0f, enabledAnisotropy, maxAnisotropy,
            VK_FALSE, VK_COMPARE_OP_ALWAYS, 0.0f, std::numeric_limits<float>::max(),
            VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE
        };
    }

    SamplerUPtr CreateSimpleLinearSampler() {
        const auto info = SamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

        VkSampler sampler = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateSampler(Logical::Device::Get(), &info, Allocator::CPU(), &sampler)) {
            return nullptr;
        }

        return std::make_unique<Sampler>(sampler);
    }

    SamplerUPtr CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW) {
        const auto info = SamplerInfo(minFilter, magFilter, modeU, modeV, modeW);

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

    struct StagingImage {
        VkBuffer          buffer = VK_NULL_HANDLE;
        VmaAllocation     alloc = VK_NULL_HANDLE;
        VmaAllocationInfo info{};

        ~StagingImage() {
            if (VK_NULL_HANDLE != buffer) {
                vmaDestroyBuffer(Allocator::VMA(), buffer, alloc);
            }
        }

        void Copy(VkDeviceSize size, const void* src) const {
            memcpy_s(info.pMappedData, size, src, size);
        }
    };

    constexpr VkBufferCreateInfo BufferInfo(VkDeviceSize size) {
        return { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
    }

    constexpr VmaAllocationCreateInfo AllocInfo(VmaAllocationCreateFlags flags, VmaMemoryUsage usage) {
        return { flags, usage };
    }

    constexpr VkImageCreateInfo ImageInfo(VkFormat format, const VkExtent3D& extent) {
        return {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0, VK_IMAGE_TYPE_2D,
            format, extent, 1, 1, VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED
        };
    }

    constexpr VkImageMemoryBarrier ImageBarrierInfo(VkAccessFlags srcMask, VkAccessFlags destMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image) {
        return {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, srcMask, destMask,
            oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };
    }

    constexpr VkBufferImageCopy CopyRegionInfo(const VkExtent3D& extent) {
        return {
            0, 0, 0,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            { 0, 0, 0 }, extent
        };
    }

    constexpr VkImageViewCreateInfo ImageViewInfo(VkFormat format, VkImage image) {
        return {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, image,
            VK_IMAGE_VIEW_TYPE_2D, format, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };
    }

    Dimension2UPtr CreateSimple2D(std::string&& path, Command::Pool& cmdPool) {
        auto image = gli::load(path);
        if (image.empty()) {
            return nullptr;
        }

        StagingImage staging;
        const auto stagingInfo = BufferInfo(image.size());
        const auto stagingCreateInfo = AllocInfo(VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &stagingInfo, &stagingCreateInfo, &staging.buffer, &staging.alloc, &staging.info)) {
            return nullptr;
        }
        staging.Copy(image.size(), image.data());

        const auto dimension = image.extent();
        const VkExtent3D extent3d = { static_cast<uint32_t>(dimension.x), static_cast<uint32_t>(dimension.y), 1 };

        const auto imageInfo = ImageInfo(VK_FORMAT_R8G8B8A8_UNORM, extent3d);
        const auto imageCreateInfo = AllocInfo(0, VMA_MEMORY_USAGE_GPU_ONLY);

        VkImage texture = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &imageInfo, &imageCreateInfo, &texture, &alloc, nullptr)) {
            return nullptr;
        }

        auto* cmdBuf = cmdPool.ImmediatelyBegin();

        auto barrier = ImageBarrierInfo(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture);
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        const auto region = CopyRegionInfo(extent3d);
        vkCmdCopyBufferToImage(cmdBuf, staging.buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        cmdPool.ImmediatelyEndAndSubmit();

        // Image view.
        const auto imageViewInfo = ImageViewInfo(VK_FORMAT_R8G8B8A8_UNORM, texture);

        VkImageView view = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateImageView(Logical::Device::Get(), &imageViewInfo, Allocator::CPU(), &view)) {
            vmaDestroyImage(Allocator::VMA(), texture, alloc);
            return nullptr;
        }

        return std::make_unique<Dimension2>(texture, alloc, view);
    }

    Dimension2UPtr CreateFourPixel2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, Command::Pool& cmdPool) {
        constexpr VkExtent3D extent = { 4, 4, 1 };
        constexpr VkDeviceSize bufferSize = extent.width * extent.height * 4/*r, g, b, a*/;
        const uint8_t pixels[bufferSize]{
            r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a,
            r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a,
            r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a,
            r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a,
        };

        StagingImage staging;
        const auto stagingInfo = BufferInfo(bufferSize);
        const auto stagingCreateInfo = AllocInfo(VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &stagingInfo, &stagingCreateInfo, &staging.buffer, &staging.alloc, &staging.info)) {
            return nullptr;
        }
        staging.Copy(bufferSize, pixels);

        const auto imageInfo = ImageInfo(VK_FORMAT_R8G8B8A8_UNORM, extent);
        const auto imageCreateInfo = AllocInfo(0, VMA_MEMORY_USAGE_GPU_ONLY);

        VkImage texture = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &imageInfo, &imageCreateInfo, &texture, &alloc, nullptr)) {
            return nullptr;
        }

        auto* cmdBuf = cmdPool.ImmediatelyBegin();

        auto barrier = ImageBarrierInfo(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture);
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        const auto region = CopyRegionInfo(extent);
        vkCmdCopyBufferToImage(cmdBuf, staging.buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        cmdPool.ImmediatelyEndAndSubmit();

        // Image view.
        const auto imageViewInfo = ImageViewInfo(VK_FORMAT_R8G8B8A8_UNORM, texture);

        VkImageView view = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateImageView(Logical::Device::Get(), &imageViewInfo, Allocator::CPU(), &view)) {
            vmaDestroyImage(Allocator::VMA(), texture, alloc);
            return nullptr;
        }

        return std::make_unique<Dimension2>(texture, alloc, view);
    }

    Dimension2UPtr CreateSrcTo2D(std::span<const uint8_t>&& src, VkExtent3D&& extent, Command::Pool& cmdPool) {
        StagingImage staging;
        const auto stagingInfo = BufferInfo(src.size());
        const auto stagingCreateInfo = AllocInfo(VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        if (VK_SUCCESS != vmaCreateBuffer(Allocator::VMA(), &stagingInfo, &stagingCreateInfo, &staging.buffer, &staging.alloc, &staging.info)) {
            return nullptr;
        }
        staging.Copy(src.size(), src.data());

        const auto imageInfo = ImageInfo(VK_FORMAT_R8G8B8A8_UNORM, extent);
        const auto imageCreateInfo = AllocInfo(0, VMA_MEMORY_USAGE_GPU_ONLY);

        VkImage texture = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        if (VK_SUCCESS != vmaCreateImage(Allocator::VMA(), &imageInfo, &imageCreateInfo, &texture, &alloc, nullptr)) {
            return nullptr;
        }

        auto* cmdBuf = cmdPool.ImmediatelyBegin();

        auto barrier = ImageBarrierInfo(0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture);
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        const auto region = CopyRegionInfo(extent);
        vkCmdCopyBufferToImage(cmdBuf, staging.buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        cmdPool.ImmediatelyEndAndSubmit();

        // Image view.
        const auto imageViewInfo = ImageViewInfo(VK_FORMAT_R8G8B8A8_UNORM, texture);

        VkImageView view = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateImageView(Logical::Device::Get(), &imageViewInfo, Allocator::CPU(), &view)) {
            vmaDestroyImage(Allocator::VMA(), texture, alloc);
            return nullptr;
        }

        return std::make_unique<Dimension2>(texture, alloc, view);
    }
}
