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
    using Samplers = std::unordered_map <SamplerKey, Image::SamplerUPtr, decltype(SamplerKeyHashFn)>;

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
        const auto findPair = g_samplers.find({ minFilter, magFilter, modeU, modeV, modeW });
        if (g_samplers.end() == findPair) {
            const auto result = g_samplers.emplace(SamplerKey{ minFilter, magFilter, modeU, modeV, modeW }, Image::CreateSampler(minFilter, magFilter, modeU, modeV, modeW));
            if (false == result.second) {
                return *g_defaultSampler;
            }
            return *result.first->second;
        }
        return *findPair->second;
    }
}
