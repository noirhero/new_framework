// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "model.h"

#include "../renderer/renderer_pch.h"

namespace Model {
    // Sampler.
    struct SamplerKey {
        VkFilter             minFilter;
        VkFilter             magFilter;
        VkSamplerAddressMode modeU;
        VkSamplerAddressMode modeV;
        VkSamplerAddressMode modeW;

        bool operator==(const SamplerKey&) const;
    };
    const auto SamplerKeyHashFn = [](const SamplerKey& key)->size_t {
	    return key.magFilter + key.minFilter + key.modeU + key.modeV + key.modeW;
    };
    bool SamplerKey::operator==(const SamplerKey& other) const {
        return SamplerKeyHashFn(*this) == SamplerKeyHashFn(other);
    }
    using Samplers = std::unordered_map<SamplerKey, Image::SamplerUPtr, decltype(SamplerKeyHashFn)>;

    Samplers           g_samplers;
    Image::SamplerUPtr g_defaultSampler;

    bool Sampler::Initialize() {
        g_defaultSampler = Image::CreateSimpleLinearSampler();
        return true;
    }

    void Sampler::Destroy() {
        g_samplers.clear();
        g_defaultSampler.reset();
    }

    Image::Sampler& Sampler::Get(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW) {
        const auto findIterator = g_samplers.find({ minFilter, magFilter, modeU, modeV, modeW });
        if (g_samplers.end() == findIterator) {
            const auto result = g_samplers.emplace(SamplerKey{ minFilter, magFilter, modeU, modeV, modeW }, Image::CreateSampler(minFilter, magFilter, modeU, modeV, modeW));
            if (false == result.second) {
                return *g_defaultSampler;
            }
            return *result.first->second;
        }
        return *findIterator->second;
    }

    // Texture2D.
    using Texture2Ds = std::unordered_map<std::string, Image::Dimension2UPtr>;
    Texture2Ds            g_texture2Ds;
    Image::Dimension2UPtr g_defaultTexture2D;

    bool Texture2D::Initialize(Command::Pool& cmdPool) {
        g_defaultTexture2D = Image::CreateFourPixel2D(255, 89, 180, 255, cmdPool);
        return true;
    }

    void Texture2D::Destroy() {
        g_texture2Ds.clear();
        g_defaultTexture2D.reset();
    }

    Image::Dimension2& Texture2D::Get(std::string&& key, std::span<const uint8_t>&& pixels, uint32_t width, uint32_t height, Command::Pool& cmdPool) {
        const auto findIterator = g_texture2Ds.find(key);
        if (g_texture2Ds.end() == findIterator) {
            const auto result = g_texture2Ds.emplace(key, Image::CreateSrcTo2D(std::move(pixels), { width, height, 1 }, cmdPool));
            if (false == result.second) {
                return *g_defaultTexture2D;
            }
            return *result.first->second;
        }
        return *findIterator->second;
    }
}
