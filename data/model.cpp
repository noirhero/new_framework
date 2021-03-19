// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "model.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tinygltf/tiny_gltf.h>

#include "../renderer/renderer_pch.h"

namespace Data {
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
            auto sampler = Image::CreateSampler(minFilter, magFilter, modeU, modeV, modeW);
            if (nullptr == sampler) {
                return *g_defaultSampler;
            }

            const auto result = g_samplers.emplace(SamplerKey{ minFilter, magFilter, modeU, modeV, modeW }, std::move(sampler));
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
    uint32_t              g_pixelGenerateIndex = 0;

    bool Texture2D::Initialize(Command::Pool& cmdPool) {
        g_defaultTexture2D = Image::CreateFourPixel2D(255, 89, 180, 255, cmdPool);
        return true;
    }

    void Texture2D::Destroy() {
        g_texture2Ds.clear();
        g_defaultTexture2D.reset();
    }

    Image::Dimension2& Texture2D::Get(std::span<const uint8_t>&& pixels, uint32_t width, uint32_t height, Command::Pool& cmdPool) {
        const auto key = fmt::format("PixelGenerate_{}", g_pixelGenerateIndex++);
        const auto result = g_texture2Ds.emplace(key, Image::CreateSrcTo2D(std::move(pixels), { width, height, 1 }, cmdPool));
		if (false == result.second) {
			return *g_defaultTexture2D;
		}
		return *result.first->second;
    }

    Image::Dimension2UPtr ImportTexture(const std::string& filePath, Command::Pool& cmdPool) {
        if (std::string::npos != filePath.find(".png")) {
            int32_t width, height, component;
            const auto* srcBuffer = stbi_load(filePath.c_str(), &width, &height, &component, STBI_rgb_alpha);

            return Image::CreateSrcTo2D(
                { srcBuffer, static_cast<size_t>(width * height * STBI_rgb_alpha) },
                { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 }, 
                cmdPool);
        }

        if (std::string::npos != filePath.find(".ktx") || std::string::npos != filePath.find(".dds") || std::string::npos != filePath.find(".kmg")) {
            const auto gliImage = gli::load(filePath);
            const auto dimension = gliImage.extent();
            const auto width = static_cast<uint32_t>(dimension.x);
            const auto height = static_cast<uint32_t>(dimension.x);

            return Image::CreateSrcTo2D({ static_cast<const uint8_t*>(gliImage.data()), gliImage.size() }, { width, height, 1 }, cmdPool);
        }

        return nullptr;
    }

    Image::Dimension2& Texture2D::Get(std::string&& fileName, Command::Pool& cmdPool) {
        const auto filePath = Path::GetResourcePathAnsi() + "images/"s + fileName;
        const auto findIterator = g_texture2Ds.find(filePath);
        if (g_texture2Ds.end() == findIterator) {
            auto texture = ImportTexture(filePath, cmdPool);
            if (nullptr == texture)
                return *g_defaultTexture2D;

            const auto result = g_texture2Ds.emplace(filePath, std::move(texture));
            if (false == result.second) {
                return *g_defaultTexture2D;
            }
            return *result.first->second;
        }
        return *findIterator->second;
    }
}
