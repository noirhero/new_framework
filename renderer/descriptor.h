// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Image {
    class Sampler;
    class Dimension2;
}
namespace Buffer {
    class Uniform;
}

namespace Descriptor {
    class Set {
    public:
        Set(VkDescriptorPool pool, VkDescriptorSet handle) : _pool(pool), _handle(handle) {}
        ~Set();
        Set& operator=(const Set&) = delete;

        constexpr VkDescriptorSet Get() const noexcept { return _handle; }

    private:
        VkDescriptorPool          _pool = VK_NULL_HANDLE;
        VkDescriptorSet           _handle = VK_NULL_HANDLE;
    };
    using SetUPtr = std::unique_ptr<Set>;
}

namespace Descriptor {
    class Pool {
    public:
        Pool(VkDescriptorPool handle, SetUPtr&& set) : _handle(handle), _set(std::move(set)) {}
        ~Pool();
        Pool& operator=(const Pool&) = delete;

        VkDescriptorSet  GetSet() const noexcept;

    private:
        VkDescriptorPool _handle = VK_NULL_HANDLE;
        SetUPtr          _set;
    };
    using PoolUPtr = std::unique_ptr<Pool>;
}

namespace Descriptor {
    class Layout {
        struct UpdateInfo {
            VkDescriptorImageInfo  imageInfo{};
            VkDescriptorBufferInfo bufferInfo{};
        };
        using UpdateInfos = std::vector<UpdateInfo>;

    public:
        Layout(VkDescriptorSetLayout handle, PoolUPtr&& pool) : _handle(handle), _pool(std::move(pool)) {}
        ~Layout();
        Layout& operator=(const Layout&) = delete;

        VkDescriptorSetLayout Get() const noexcept { return _handle; }
        VkDescriptorSet       GetSet() const noexcept;

        void                  UpdateBegin();
        void                  AddUpdate(const Image::Sampler& sampler, const Image::Dimension2& image);
        void                  AddUpdate(const Buffer::Uniform& uniform);
        void                  UpdateImmediately();

    private:
        VkDescriptorSetLayout _handle = VK_NULL_HANDLE;
        PoolUPtr              _pool;

        UpdateInfos           _updateInfos;
    };
    using LayoutUPtr = std::unique_ptr<Layout>;

    LayoutUPtr CreateSimpleLayout();
}
