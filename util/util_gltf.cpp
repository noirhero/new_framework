// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "util_gltf.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tinygltf/tiny_gltf.h>

namespace GLTF {
    Node::~Node() {
        for (auto* child : children) {
            delete child;
        }
    }

    void ImportTransform(Node& dest, const tinygltf::Node& src) {
        if (3 == src.scale.size()) {
            dest.scale = glm::make_vec3(src.scale.data());
        }

        if (4 == src.rotation.size()) {
            dest.rotation = glm::make_quat(src.rotation.data());
        }

        if(3 == src.translation.size()) {
            dest.translation = glm::make_vec3(src.translation.data());
        }

        if (16 == src.matrix.size()) {
            dest.local = glm::make_mat4(src.matrix.data());
        }
    }

    Node* ImportNode(Node& parent, const tinygltf::Node& src, const tinygltf::Model& model) {
        auto* dest = new Node;
        parent.children.emplace_back(dest);
        dest->parent = &parent;

        ImportTransform(*dest, src);

        dest->meshIndex = src.mesh;

        for (const auto i : src.children) {
            ImportNode(*dest, model.nodes[i], model);
        }

        return dest;
    }

    void ImportMesh(Mesh& dest, const tinygltf::Mesh& src, const tinygltf::Model& model) {
        for (const auto& srcPrimitive : src.primitives) {
            dest.primitives.emplace_back();
            auto& destPrimitive = dest.primitives.back();

            const auto& srcAttributes = srcPrimitive.attributes;

            // Material index.
            destPrimitive.materialIndex = srcPrimitive.material;

            // Positions.
            const auto posAttrPair = srcAttributes.find("POSITION");
            assert(srcAttributes.end() != posAttrPair);

            const auto& posAccessor = model.accessors[posAttrPair->second];
            const auto& posView = model.bufferViews[posAccessor.bufferView];
            const auto* srcPositions = reinterpret_cast<const float*>(&model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]);

            destPrimitive.positions.resize(posAccessor.count);
            const auto posMemSize = sizeof(glm::vec3) * posAccessor.count;
            memcpy_s(destPrimitive.positions.data(), posMemSize, srcPositions, posMemSize);

            // Bounding box.
            destPrimitive.bBox.min = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
            destPrimitive.bBox.max = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
            dest.bBox.min = glm::min(destPrimitive.bBox.min, dest.bBox.min);
            dest.bBox.max = glm::max(destPrimitive.bBox.max, dest.bBox.max);

            // Normals.
            const auto norAttrPair = srcAttributes.find("NORMAL");
            if (srcAttributes.end() != norAttrPair) {
                const auto& norAccessor = model.accessors[norAttrPair->second];
                assert(posAccessor.count == norAccessor.count);

                const auto& norView = model.bufferViews[norAccessor.bufferView];
                const auto* srcNormals = reinterpret_cast<const float*>(&model.buffers[norView.buffer].data[norAccessor.byteOffset + norView.byteOffset]);

                destPrimitive.normals.resize(norAccessor.count);
                const auto norMemSize = sizeof(glm::vec3) * norAccessor.count;
                memcpy_s(destPrimitive.normals.data(), norMemSize, srcNormals, norMemSize);
            }

            // Texture coordinate 0.
            const auto uv0Pair = srcAttributes.find("TEXCOORD_0");
            if (srcAttributes.end() != uv0Pair) {
                const auto& uv0Accessor = model.accessors[uv0Pair->second];
                assert(posAccessor.count == uv0Accessor.count);

                const auto& uv0View = model.bufferViews[uv0Accessor.bufferView];
                const auto* uv0 = reinterpret_cast<const float*>(&model.buffers[uv0View.buffer].data[uv0Accessor.byteOffset + uv0View.byteOffset]);

                destPrimitive.uv0.resize(uv0Accessor.count);
                const auto uv0MemSize = sizeof(glm::vec2) * uv0Accessor.count;
                memcpy_s(destPrimitive.uv0.data(), uv0MemSize, uv0, uv0MemSize);
            }

            // Indices.
            if (0 <= srcPrimitive.indices) {
                const auto& indexAccessor = model.accessors[srcPrimitive.indices];
                const auto& indexView = model.bufferViews[indexAccessor.bufferView];

                switch (indexAccessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const auto* srcIndices = reinterpret_cast<const uint32_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        destPrimitive.indices.emplace_back(static_cast<uint16_t>(srcIndices[i]));
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const auto* srcIndices = reinterpret_cast<const uint16_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        destPrimitive.indices.emplace_back(srcIndices[i]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const auto* srcIndices = reinterpret_cast<const uint8_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        destPrimitive.indices.emplace_back(static_cast<uint16_t>(srcIndices[i]));
                    }
                    break;
                }
                default:;
                }
            }
        }
    }

	void ImportSampler(Sampler& dest, const tinygltf::Sampler& src) {
        constexpr auto convertFilterFn = [](int32_t srcFilter) {
            switch (srcFilter) {
            case 9728:
            case 9984:
            case 9985:
                return VK_FILTER_NEAREST;
            case 9729:
            case 9986:
            case 9987:
                return VK_FILTER_LINEAR;
            default:
                return VK_FILTER_MAX_ENUM;
            }
        };
        dest.magFilter = convertFilterFn(src.magFilter);
        dest.minFilter = convertFilterFn(src.minFilter);

        constexpr auto convertWrapModeFn = [](int32_t srcWrapMode) {
            switch (srcWrapMode) {
            case 10497: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case 33071: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case 33648: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default:    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
            }
        };
        dest.modeU = convertWrapModeFn(src.wrapS);
        dest.modeV = convertWrapModeFn(src.wrapT);
    }

    void ImportTexture(Texture& dest, gli::texture&& src) {
        const auto extent = src.extent();
        dest.width = extent.x;
        dest.height = extent.y;

        const auto srcSpan = std::span<const uint8_t>(static_cast<const uint8_t*>(src.data()), src.size());
        dest.buffer.insert(dest.buffer.begin(), srcSpan.begin(), srcSpan.end());
    }

    void ImportTexture(Texture& dest, const std::string& fileName) {
        const auto fullPath = Path::GetResourcePathAnsi() + "images/"s + fileName;

        if (std::string::npos != fileName.find(".png")) {
            int32_t width, height, component;
            const auto* srcBuffer = stbi_load(fullPath.c_str(), &width, &height, &component, 0);
            const auto srcSpan = std::span<const stbi_uc>(srcBuffer, width * height * component);

            dest.width = width;
            dest.height = height;
            dest.buffer.insert(dest.buffer.begin(), srcSpan.begin(), srcSpan.end());
        }
        else if (std::string::npos != fileName.find(".ktx") || std::string::npos != fileName.find(".dds") || std::string::npos != fileName.find(".kmg")) {
            ImportTexture(dest, gli::load(fullPath));
        }
    }

    void ImportTexture(Texture& dest, const tinygltf::Texture& src, const tinygltf::Model& model) {
        const auto& srcImage = model.images[src.source];

        if (false == srcImage.uri.empty()) {
            ImportTexture(dest, srcImage.uri);
        }
        else {
            dest.width = srcImage.width;
            dest.height = srcImage.height;
            dest.buffer.insert(dest.buffer.begin(), srcImage.image.begin(), srcImage.image.end());
        }

        dest.samplerIndex = src.sampler;
    }

	void ImportMaterial(Material& dest, const tinygltf::Material& src) {
        // Albedo.
        const auto albedoPair = src.values.find("baseColorTexture");
        if (src.values.end() != albedoPair) {
            dest.albedoIndex = albedoPair->second.TextureIndex();
            dest.albedoUVIndex = albedoPair->second.TextureTexCoord();
        }

        // Normal.
        const auto normalPair = src.additionalValues.find("normalTexture");
        if (src.additionalValues.end() != normalPair) {
            dest.normalIndex = normalPair->second.TextureIndex();
            dest.normalUVIndex = normalPair->second.TextureTexCoord();
        }

        // Metallic roughness.
        const auto metallicRoughnessPair = src.values.find("metallicRoughnessTexture");
        if (src.values.end() != metallicRoughnessPair) {
            dest.metallicRoughnessIndex = metallicRoughnessPair->second.TextureIndex();
            dest.metallicRoughnessUVIndex = metallicRoughnessPair->second.TextureTexCoord();
        }

        // Alpha mode.
        const auto alphaModePair = src.additionalValues.find("alphaMode");
        if (src.additionalValues.end() != alphaModePair) {
            if ("BLEND"s == alphaModePair->second.string_value) {
                dest.alphaMode = Material::Alpha::Blend;
            }
            else if ("MASK"s == albedoPair->second.string_value) {
                dest.alphaMode = Material::Alpha::Mask;
                dest.alphaCutoff = 0.5f;
            }
        }
        const auto alphaCutOffPair = src.additionalValues.find("alphaCutoff");
        if (src.additionalValues.end() != alphaCutOffPair) {
            dest.alphaCutoff = static_cast<float>(alphaCutOffPair->second.Factor());
        }

        // Roughness factor.
        const auto roughnessFactorPair = src.values.find("roughnessFactor");
        if (src.values.end() != roughnessFactorPair) {
            dest.roughness = static_cast<float>(roughnessFactorPair->second.Factor());
        }

        // Metallic factor.
        const auto metallicFactorPair = src.values.find("metallicFactor");
        if (src.values.end() != metallicFactorPair) {
            dest.metallic = static_cast<float>(metallicFactorPair->second.Factor());
        }
    }

    ModelUPtr Load(std::string&& path) {
        auto model = std::make_unique<Model>();

        tinygltf::TinyGLTF context;
        std::string err;
        std::string war;

        tinygltf::Model gltfModel;
        if (false == context.LoadASCIIFromFile(&gltfModel, &err, &war, path)) {
            return model;
        }

        // Nodes.
        const auto& scene = gltfModel.scenes[0 <= gltfModel.defaultScene ? gltfModel.defaultScene : 0];
        for (const auto i : scene.nodes) {
            ImportNode(model->root, gltfModel.nodes[i], gltfModel);
        }

        // Meshes.
        for (const auto& gltfMesh : gltfModel.meshes) {
            model->meshes.emplace_back();
            ImportMesh(model->meshes.back(), gltfMesh, gltfModel);
        }

        // Samplers.
        for (const auto& gltfTexture : gltfModel.samplers) {
            model->samplers.emplace_back();
            ImportSampler(model->samplers.back(), gltfTexture);
        }

        // Textures.
        for (const auto& gltfTexture : gltfModel.textures) {
            model->textures.emplace_back();
            ImportTexture(model->textures.back(), gltfTexture, gltfModel);
        }

        // Materials.
        for (const auto& gltfMaterial : gltfModel.materials) {
            model->materials.emplace_back();
            ImportMaterial(model->materials.back(), gltfMaterial);
        }

        return model;
    }
}
