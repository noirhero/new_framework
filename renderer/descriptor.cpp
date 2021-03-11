// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "descriptor.h"

#include "renderer_common.h"
#include "allocator_cpu.h"
#include "logical.h"
#include "image.h"
#include "buffer.h"

namespace Descriptor {
    using namespace Renderer;

    // Descriptor set.
    Set::~Set() {
        if(VK_NULL_HANDLE != _handle) {
            vkFreeDescriptorSets(Logical::Device::Get(), _pool, 1, &_handle);
    	}
    }

    // Descriptor ppol.
    Pool::~Pool() {
        _set.reset();

        if (VK_NULL_HANDLE != _handle) {
            vkDestroyDescriptorPool(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    VkDescriptorSet Pool::GetSet() const noexcept{
    	if(nullptr == _set) {
            return VK_NULL_HANDLE;
    	}
        return _set->Get();
    }

    // Descriptor set layout.
    Layout::~Layout() {
        _pool.reset();

        if (VK_NULL_HANDLE != _handle) {
            vkDestroyDescriptorSetLayout(Logical::Device::Get(), _handle, Allocator::CPU());
        }
    }

    void Layout::UpdateBegin() {
        _updateInfos.clear();
    }

    void Layout::AddUpdate(const Image::Sampler& sampler, const Image::Dimension2& image) {
        _updateInfos.emplace_back(image.Information(sampler.Get()), VkDescriptorBufferInfo{});
    }

    void Layout::AddUpdate(const Buffer::Uniform& uniform) {
        _updateInfos.emplace_back(VkDescriptorImageInfo{}, uniform.Information());
    }

    void Layout::UpdateImmediately() {
        auto* set = _pool->GetSet();

        std::vector<VkWriteDescriptorSet> writes;
        for (const auto& info : _updateInfos) {
            const auto dstBinding = static_cast<uint32_t>(writes.size());
            if (VK_NULL_HANDLE != info.imageInfo.sampler) {
                writes.emplace_back(
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, dstBinding, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &info.imageInfo, nullptr, nullptr
                );
            }
            else if (VK_NULL_HANDLE != info.bufferInfo.buffer) {
                writes.emplace_back(
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, dstBinding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &info.bufferInfo, nullptr
                );
            }
        }

        const auto writeCount = static_cast<uint32_t>(writes.size());
        vkUpdateDescriptorSets(Logical::Device::Get(), writeCount, writes.data(), 0, nullptr);
    }

    LayoutUPtr CreateSimpleLayout() {
        auto* device = Logical::Device::Get();

        // Layout.
        VkDescriptorSetLayoutBinding bindings[2/* uniform buffer, image */]{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = _countof(bindings);
        layoutCreateInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorSetLayout(device, &layoutCreateInfo, Allocator::CPU(), &layout)) {
            return nullptr;
        }

        // Pool.
        std::vector<VkDescriptorPoolSize> descriptorSizeInfos;
        descriptorSizeInfos.emplace_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
        descriptorSizeInfos.emplace_back(VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 });

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = 1;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.poolSizeCount = static_cast<decltype(info.poolSizeCount)>(descriptorSizeInfos.size());
        info.pPoolSizes = descriptorSizeInfos.data();

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkCreateDescriptorPool(device, &info, Allocator::CPU(), &pool)) {
            vkDestroyDescriptorSetLayout(device, layout, Allocator::CPU());
            return nullptr;
        }

        // Set.
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet descSet = VK_NULL_HANDLE;
        if (VK_SUCCESS != vkAllocateDescriptorSets(device, &allocInfo, &descSet)) {
            vkDestroyDescriptorPool(device, pool, Allocator::CPU());
            vkDestroyDescriptorSetLayout(device, layout, Allocator::CPU());
            return nullptr;
        }

        return std::make_unique<Layout>(layout, std::make_unique<Pool>(pool, std::make_unique<Set>(pool, descSet)));
    }
}
