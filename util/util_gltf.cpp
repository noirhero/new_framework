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

    Model::~Model() {
        delete root;
    }

    void ImportMesh(Mesh& dest, const tinygltf::Mesh& src, const tinygltf::Model& model) {
        for (const auto& srcPrimitive : src.primitives) {
            dest.primitives.emplace_back();
            auto& destPrimitive = dest.primitives.back();

            destPrimitive.materialIndex = srcPrimitive.material;

            // Positions.
            const auto& posAttrPair = srcPrimitive.attributes.find("POSITION");
            assert(srcPrimitive.attributes.end() != posAttrPair);

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
            const auto& norAttrPair = srcPrimitive.attributes.find("NORMAL");
            if (srcPrimitive.attributes.end() != norAttrPair) {
                const auto& norAccessor = model.accessors[norAttrPair->second];
                assert(posAccessor.count == norAccessor.count);

                const auto& norView = model.bufferViews[norAccessor.bufferView];
                const auto* srcNormals = reinterpret_cast<const float*>(&model.buffers[norView.buffer].data[norAccessor.byteOffset + norView.byteOffset]);

                destPrimitive.normals.resize(norAccessor.count);
                const auto norMemSize = sizeof(glm::vec3) * norAccessor.count;
                memcpy_s(destPrimitive.normals.data(), norMemSize, srcNormals, norMemSize);
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

        for (const auto i : src.children) {
            ImportNode(*dest, model.nodes[i], model);
        }

        return dest;
    }

    ModelUPtr Load(std::string&& path) {
        auto model = std::make_unique<Model>(new Node);

        tinygltf::TinyGLTF context;
        std::string err;
        std::string war;

        tinygltf::Model gltfModel;
        if (false == context.LoadASCIIFromFile(&gltfModel, &err, &war, path)) {
            return model;
        }

        // Meshes.
    	for(const auto& gltfMesh : gltfModel.meshes) {
            model->meshes.emplace_back();
            auto& mesh = model->meshes.back();

            ImportMesh(mesh, gltfMesh, gltfModel);
    	}

        // Nodes.
        const auto& scene = gltfModel.scenes[0 <= gltfModel.defaultScene ? gltfModel.defaultScene : 0];
        for (const auto i : scene.nodes) {
            ImportNode(*model->root, gltfModel.nodes[i], gltfModel);
        }

        return model;
    }
}
