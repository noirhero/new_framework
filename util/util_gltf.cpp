// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "util_gltf.h"

#include "../renderer/renderer_pch.h"
#include "../data/resource.h"

namespace GLTF {
    using Datas = std::unordered_map<std::string, Data::Model*>;
    Datas       g_datas;
    Data::Model g_defaultModel;

    using BufferUPtrs = std::unordered_map<std::string, Buffer::ObjectUPtr>;
    BufferUPtrs g_vertexBuffers;
    BufferUPtrs g_indexBuffers;

    using Vec3s = std::vector<glm::vec3>;
    using Vec2s = std::vector<glm::vec2>;
    using uint16_ts = std::vector<uint16_t>;
    struct ConvertData {
        Command::Pool&         cmdPool;
        const tinygltf::Model& src;
        Data::Model&           dest;
        Vec3s                  positions;
        Vec3s                  normals;
        Vec2s                  uvs;
        uint16_ts              indices;
    };

    void ConvertTransform(glm::mat4& dest, const tinygltf::Node& src) {
        glm::vec3 scale{ 1.0f };
        if (3 == src.scale.size()) {
            scale = glm::make_vec3(src.scale.data());
        }

        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        if (4 == src.rotation.size()) {
            rotation = glm::make_quat(src.rotation.data());
        }

        glm::vec3 translation{ 0.0f };
        if (3 == src.translation.size()) {
            translation = glm::make_vec3(src.translation.data());
        }

        if (16 == src.matrix.size()) {
            dest = glm::make_mat4(src.matrix.data());
        }
        else {
            constexpr auto identity = glm::mat4(1.0f);
            dest = glm::scale(identity, scale) * glm::mat4(rotation) * glm::translate(identity, translation);
        }
    }

    int32_t GetBoneIndex(ConvertData& temp, const Data::Bone& bone) {
        int32_t index = 0;
        for (const auto& eachBone : temp.dest.bones) {
            if (&eachBone == &bone) {
                return index;
            }
            ++index;
        }
        return INVALID_IDX;
    }

    void FillVertexInformation(Data::VertexInfos& dest, uint32_t stride, Data::VertexType type) {
        for (const auto& info : dest) {
            if (info.type == type) {
                return;
            }
        }

        dest.emplace_back(0, stride, type);
    }

    Image::Sampler* ConvertSampler(const tinygltf::Texture& src, ConvertData& temp) {
        if(0 > src.sampler) {
            return &Data::Sampler::Get(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        }

        constexpr auto convertFilterFn = [](int32_t srcFilter) {
            switch (srcFilter) {
            case 9728:
            case 9984:
            case 9985:
                return VK_FILTER_NEAREST;
            //case 9729:
            //case 9986:
            //case 9987:
            //    return VK_FILTER_LINEAR;
            default:
                return VK_FILTER_LINEAR;
            }
        };
        constexpr auto convertWrapModeFn = [](int32_t srcWrapMode) {
            switch (srcWrapMode) {
            case 10497: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case 33071: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case 33648: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            }
        };

        const auto& srcSampler = temp.src.samplers[src.sampler];
        return &Data::Sampler::Get(
            convertFilterFn(srcSampler.minFilter),
            convertFilterFn(srcSampler.minFilter),
            convertWrapModeFn(srcSampler.wrapS),
            convertWrapModeFn(srcSampler.wrapT),
            VK_SAMPLER_ADDRESS_MODE_REPEAT
        );
    }

    Image::Dimension2* ImportTexture(const tinygltf::Texture& src, ConvertData& temp) {
        const auto& srcImage = temp.src.images[src.source];

        if (false == srcImage.uri.empty()) {
            return &Data::Texture2D::Get(srcImage.uri.c_str(), temp.cmdPool);
        }

        return &Data::Texture2D::Get({ srcImage.image.begin(), srcImage.image.end() }, temp.cmdPool);
    }

    void ConvertMaterial(Data::Material& dest, const tinygltf::Material& src, ConvertData& temp) {
        const auto& srcTextures = temp.src.textures;

        // Albedo.
        const auto albedoIterator = src.values.find("baseColorTexture");
        if (src.values.end() != albedoIterator) {
            const auto& srcAlbedo = srcTextures[albedoIterator->second.TextureIndex()];
            dest.albedoSampler = ConvertSampler(srcAlbedo, temp);
            dest.albedo = ImportTexture(srcAlbedo, temp);
        }

        // Metallic roughness.
        const auto metallicRoughnessIterator = src.values.find("metallicRoughnessTexture");
        if (src.values.end() != metallicRoughnessIterator) {
            const auto& srcMetallicRoughness = srcTextures[metallicRoughnessIterator->second.TextureIndex()];
            dest.metallicRoughnessSampler = ConvertSampler(srcMetallicRoughness, temp);
            dest.metallicRoughness = ImportTexture(srcMetallicRoughness, temp);
        }
    }

    void ConvertMesh(Data::Bone& containBone, const tinygltf::Mesh& src, ConvertData& temp) {
        const auto& model = temp.src;
        auto& dest = temp.dest.mesh;
        auto& totalBBox = temp.dest.bBox;

        for (const auto& srcPrimitive : src.primitives) {
            const auto& srcAttributes = srcPrimitive.attributes;

            const auto vertexOffset = static_cast<uint16_t>(temp.positions.size());

            // Positions.
            const auto posIterator = srcAttributes.find("POSITION");
            assert(srcAttributes.end() != posIterator);

            const auto& posAccessor = model.accessors[posIterator->second];
            const auto& posView = model.bufferViews[posAccessor.bufferView];
            const auto* srcPositions = reinterpret_cast<const glm::vec3*>(&model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]);

            const std::span<const glm::vec3> posSpan{ srcPositions, posAccessor.count };
            temp.positions.insert(temp.positions.end(), posSpan.begin(), posSpan.end());

            FillVertexInformation(dest.vertexDecl.infos, sizeof(glm::vec3), Data::VertexType::Pos);

            // Bounding box.
            containBone.bBox.min = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
            containBone.bBox.max = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
            totalBBox.min = glm::min(containBone.bBox.min, totalBBox.min);
            totalBBox.max = glm::max(containBone.bBox.max, totalBBox.max);

            // Normals.
            const auto norIterator = srcAttributes.find("NORMAL");
            if (srcAttributes.end() != norIterator) {
                const auto& norAccessor = model.accessors[norIterator->second];
                assert(posAccessor.count == norAccessor.count);

                const auto& norView = model.bufferViews[norAccessor.bufferView];
                const auto* srcNormals = reinterpret_cast<const glm::vec3*>(&model.buffers[norView.buffer].data[norAccessor.byteOffset + norView.byteOffset]);

                const std::span<const glm::vec3> norSpan{ srcNormals, norAccessor.count };
                temp.normals.insert(temp.normals.end(), norSpan.begin(), norSpan.end());

                FillVertexInformation(dest.vertexDecl.infos, sizeof(glm::vec3), Data::VertexType::Nor);
            }

            // Texture coordinate 0.
            const auto uv0Iterator = srcAttributes.find("TEXCOORD_0");
            if (srcAttributes.end() != uv0Iterator) {
                const auto& uv0Accessor = model.accessors[uv0Iterator->second];
                assert(posAccessor.count == uv0Accessor.count);

                const auto& uv0View = model.bufferViews[uv0Accessor.bufferView];
                const auto* uv0 = reinterpret_cast<const glm::vec2*>(&model.buffers[uv0View.buffer].data[uv0Accessor.byteOffset + uv0View.byteOffset]);

                const std::span<const glm::vec2> uv0Span{ uv0, uv0Accessor.count };
                temp.uvs.insert(temp.uvs.end(), uv0Span.begin(), uv0Span.end());

                FillVertexInformation(dest.vertexDecl.infos, sizeof(glm::vec2), Data::VertexType::UV0);
            }

            // Indices.
            if (0 <= srcPrimitive.indices) {
                const auto& indexAccessor = model.accessors[srcPrimitive.indices];
                const auto& indexView = model.bufferViews[indexAccessor.bufferView];

                dest.subsets.emplace_back();
                auto& subset = dest.subsets.back();
                subset.indexOffset = static_cast<decltype(subset.indexOffset)>(temp.indices.size());

                switch (indexAccessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const auto* srcIndices = reinterpret_cast<const uint32_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        const auto index = static_cast<uint16_t>(srcIndices[i] + vertexOffset);
                        temp.indices.emplace_back(index);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const auto* srcIndices = reinterpret_cast<const uint16_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        const uint16_t index = srcIndices[i] + vertexOffset;
                        temp.indices.emplace_back(index);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const auto* srcIndices = reinterpret_cast<const uint8_t*>(&model.buffers[indexView.buffer].data[indexAccessor.byteOffset + indexView.byteOffset]);
                    for (decltype(indexAccessor.count) i = 0; i < indexAccessor.count; ++i) {
                        const auto index = static_cast<uint16_t>(srcIndices[i] + vertexOffset);
                        temp.indices.emplace_back(index);
                    }
                    break;
                }
                default:;
                }

                const auto numIndices = static_cast<decltype(subset.numTri)>(temp.indices.size());
                subset.numTri = (numIndices - subset.indexOffset) / 3;

                if (0 <= srcPrimitive.material) {
                    ConvertMaterial(subset.material, temp.src.materials[srcPrimitive.material], temp);
                }
            }
        }
    }

    void ConvertNode(Data::Bone& dest, const tinygltf::Node& src, ConvertData& temp) {
        ConvertTransform(dest.local, src);

        if (0 <= src.mesh)
            ConvertMesh(dest, temp.src.meshes[src.mesh], temp);

        for (const auto i : src.children) {
            temp.dest.bones.emplace_back();
            auto& destChildNode = temp.dest.bones.back();

            destChildNode.parent = GetBoneIndex(temp, dest);
            destChildNode.depth = dest.depth + 1;
            ConvertNode(destChildNode, temp.src.nodes[i], temp);
        }
    }

    void ConvertModel(Data::Model& dest, const tinygltf::Model& src, Command::Pool& cmdPool) {
        ConvertData temp{ cmdPool, src, dest };

        dest.bones.emplace_back();
        ConvertNode(dest.bones.back(), src.nodes[0], temp);
        assert(temp.normals.empty() || temp.positions.size() == temp.normals.size());
        assert(temp.uvs.empty() || temp.positions.size() == temp.uvs.size());

        dest.mesh.vertexCount = static_cast<decltype(dest.mesh.vertexCount)>(temp.positions.size());
        dest.mesh.indexCount = static_cast<decltype(dest.mesh.indexCount)>(temp.indices.size());

        uint32_t vertexOffset = 0;
        for (auto& vertexInfo : dest.mesh.vertexDecl.infos) {
            vertexInfo.offset = vertexOffset;
            vertexOffset += vertexInfo.stride;
        }
        dest.mesh.vertexDecl.stride = vertexOffset;

        std::vector<float> collectVertices;
        for (size_t i = 0; i < temp.positions.size(); ++i) {
            for (const auto& vertexInfo : dest.mesh.vertexDecl.infos) {
                switch (vertexInfo.type) {
                case Data::VertexType::Pos: {
                    const auto& pos = temp.positions[i];
                    collectVertices.emplace_back(pos.x);
                    collectVertices.emplace_back(pos.y);
                    collectVertices.emplace_back(pos.z);
                    break;
                }
                case Data::VertexType::Nor: {
                    const auto& nor = temp.normals[i];
                    collectVertices.emplace_back(nor.x);
                    collectVertices.emplace_back(nor.y);
                    collectVertices.emplace_back(nor.z);
                    break;
                }
                case Data::VertexType::UV0: {
                    const auto& uv0 = temp.uvs[i];
                    collectVertices.emplace_back(uv0.x);
                    collectVertices.emplace_back(uv0.y);
                    break;
                }
                default:;
                }
            }
        }
        auto vertexBuffer = Buffer::CreateObject(
            { reinterpret_cast<const uint8_t*>(collectVertices.data()), dest.mesh.vertexCount * dest.mesh.vertexDecl.stride },
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            temp.cmdPool);
        dest.mesh.vb = vertexBuffer.get();
        g_vertexBuffers.emplace(dest.name, std::move(vertexBuffer));

        auto indexBuffer = Buffer::CreateObject(
            { reinterpret_cast<const uint8_t*>(collectVertices.data()), dest.mesh.indexCount * sizeof(uint16_t) },
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            temp.cmdPool);
        dest.mesh.ib = indexBuffer.get();
        g_indexBuffers.emplace(dest.name, std::move(indexBuffer));
    }

    Data::Model* CreateModel(const std::string& filePath, Command::Pool& cmdPool) {
        tinygltf::TinyGLTF context;
        std::string err;
        std::string war;

        tinygltf::Model gltfModel;
        if (false == context.LoadASCIIFromFile(&gltfModel, &err, &war, filePath)) {
            return nullptr;
        }
        assert(false == gltfModel.nodes.empty());

        auto* dest = new Data::Model;
        dest->name = filePath;
        ConvertModel(*dest, gltfModel, cmdPool);
        return dest;
    }

    Data::Model& Get(std::string&& fileName, Command::Pool& cmdPool) {
        const auto filePath = Path::GetResourcePathAnsi() + "models/"s + fileName;
        const auto findIterator = g_datas.find(filePath);
        if (g_datas.end() == findIterator) {
            const auto result = g_datas.emplace(filePath, CreateModel(filePath, cmdPool));
            if (false == result.second) {
                return g_defaultModel;
            }
            return *result.first->second;
        }
        return *findIterator->second;
    }

    void Clear() {
        g_vertexBuffers.clear();
        g_indexBuffers.clear();

        for (const auto& modelPair : g_datas) {
            delete modelPair.second;
        }
        g_datas.clear();
    }
}
