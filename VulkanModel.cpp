// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VulkanModel.h"

#pragma warning(disable : 4100)
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <tinygltf/tiny_gltf.h>
#pragma warning(default : 4100)

// ERROR is already defined in wingdi.h and collides with a define in the Draco headers
#if defined(_WIN32) && defined(ERROR) && defined(TINYGLTF_ENABLE_DRACO) 
#undef ERROR
#pragma message ("ERROR constant already defined, undefining")
#endif

#include "VkUtils.h"
#include "VulkanDevice.h"

namespace Vk {
    // BoundingBox
    BoundingBox BoundingBox::GetAABB(glm::mat4 m) {
        glm::vec3 min = glm::vec3(m[3]);
        glm::vec3 max = min;
        glm::vec3 v0, v1;

        glm::vec3 right = glm::vec3(m[0]);
        v0 = right * this->_min.x;
        v1 = right * this->_max.x;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        glm::vec3 up = glm::vec3(m[1]);
        v0 = up * this->_min.y;
        v1 = up * this->_max.y;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        glm::vec3 back = glm::vec3(m[2]);
        v0 = back * this->_min.z;
        v1 = back * this->_max.z;
        min += glm::min(v0, v1);
        max += glm::max(v0, v1);

        return { min, max };
    }

    // Texture
    void ModelTexture::UpdateDescriptor() {
        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = imageLayout;
    }

    void ModelTexture::Destroy() {
        vkDestroyImageView(device->logicalDevice, view, nullptr);
        vkDestroyImage(device->logicalDevice, image, nullptr);
        vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
        vkDestroySampler(device->logicalDevice, sampler, nullptr);
    }

    void ModelTexture::FromgltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler, Vk::VulkanDevice* inDevice, VkQueue copyQueue) {
        this->device = inDevice;

        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;
        if (gltfimage.component == 3) {
            // Most devices don't support RGB only on Vulkan so convert if necessary
            // TODO: Check actual format support and transform only if required
            bufferSize = gltfimage.width * gltfimage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &gltfimage.image[0];
            for (int32_t i = 0; i < gltfimage.width * gltfimage.height; ++i) {
                for (int32_t j = 0; j < 3; ++j) {
                    rgba[j] = rgb[j];
                }
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        }
        else {
            buffer = &gltfimage.image[0];
            bufferSize = gltfimage.image.size();
        }

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        VkFormatProperties formatProperties;

        width = gltfimage.width;
        height = gltfimage.height;
        mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

        vkGetPhysicalDeviceFormatProperties(inDevice->physicalDevice, format, &formatProperties);
        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
        assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs{};

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = bufferSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CheckResult(vkCreateBuffer(inDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
        vkGetBufferMemoryRequirements(inDevice->logicalDevice, stagingBuffer, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = inDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CheckResult(vkAllocateMemory(inDevice->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
        CheckResult(vkBindBufferMemory(inDevice->logicalDevice, stagingBuffer, stagingMemory, 0));

        uint8_t* data;
        CheckResult(vkMapMemory(inDevice->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void**)& data));
        memcpy(data, buffer, bufferSize);
        vkUnmapMemory(inDevice->logicalDevice, stagingMemory);

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = { width, height, 1 };
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        CheckResult(vkCreateImage(inDevice->logicalDevice, &imageCreateInfo, nullptr, &image));
        vkGetImageMemoryRequirements(inDevice->logicalDevice, image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = inDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        CheckResult(vkAllocateMemory(inDevice->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
        CheckResult(vkBindImageMemory(inDevice->logicalDevice, image, deviceMemory, 0));

        VkCommandBuffer copyCmd = inDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = width;
        bufferCopyRegion.imageExtent.height = height;
        bufferCopyRegion.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        inDevice->FlushCommandBuffer(copyCmd, copyQueue, true);

        vkFreeMemory(inDevice->logicalDevice, stagingMemory, nullptr);
        vkDestroyBuffer(inDevice->logicalDevice, stagingBuffer, nullptr);

        // Generate the mip chain (glTF uses jpg and png, so we need to Create this manually)
        VkCommandBuffer blitCmd = inDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        for (uint32_t i = 1; i < mipLevels; i++) {
            VkImageBlit imageBlit{};

            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.layerCount = 1;
            imageBlit.srcSubresource.mipLevel = i - 1;
            imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
            imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
            imageBlit.srcOffsets[1].z = 1;

            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.layerCount = 1;
            imageBlit.dstSubresource.mipLevel = i;
            imageBlit.dstOffsets[1].x = int32_t(width >> i);
            imageBlit.dstOffsets[1].y = int32_t(height >> i);
            imageBlit.dstOffsets[1].z = 1;

            VkImageSubresourceRange mipSubRange = {};
            mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            mipSubRange.baseMipLevel = i;
            mipSubRange.levelCount = 1;
            mipSubRange.layerCount = 1;

            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.srcAccessMask = 0;
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.image = image;
                imageMemoryBarrier.subresourceRange = mipSubRange;
                vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }

            vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                imageMemoryBarrier.image = image;
                imageMemoryBarrier.subresourceRange = mipSubRange;
                vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
        }

        subresourceRange.levelCount = mipLevels;
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        {
            VkImageMemoryBarrier imageMemoryBarrier{};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }

        inDevice->FlushCommandBuffer(blitCmd, copyQueue, true);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = textureSampler.magFilter;
        samplerInfo.minFilter = textureSampler.minFilter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = textureSampler.addressModeU;
        samplerInfo.addressModeV = textureSampler.addressModeV;
        samplerInfo.addressModeW = textureSampler.addressModeW;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.maxAnisotropy = 1.0;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxLod = (float)mipLevels;
        samplerInfo.maxAnisotropy = 8.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;
        CheckResult(vkCreateSampler(inDevice->logicalDevice, &samplerInfo, nullptr, &sampler));

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.levelCount = mipLevels;
        CheckResult(vkCreateImageView(inDevice->logicalDevice, &viewInfo, nullptr, &view));

        descriptor.sampler = sampler;
        descriptor.imageView = view;
        descriptor.imageLayout = imageLayout;

        if (deleteBuffer)
            delete[] buffer;

    }

    // Mesh
    Mesh::Mesh(Vk::VulkanDevice* device, glm::mat4 matrix) {
        this->device = device;
        this->uniformBlock.matrix = matrix;
        CheckResult(device->CreateBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(uniformBlock),
            &uniformBuffer.buffer,
            &uniformBuffer.memory,
            &uniformBlock));
        CheckResult(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
        uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
    };

    Mesh::~Mesh() {
        vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
        vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
        for (Primitive* p : primitives)
            delete p;
    }

    // Node
    glm::mat4 Node::LocalMatrix() {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
    }

    glm::mat4 Node::GetMatrix() {
        glm::mat4 m = LocalMatrix();
        Node* p = parent;
        while (p) {
            m = p->LocalMatrix() * m;
            p = p->parent;
        }
        return m;
    }

    void Node::Update() {
        if (mesh) {
            glm::mat4 m = GetMatrix();
            if (skin) {
                mesh->uniformBlock.matrix = m;
                // Update join matrices
                glm::mat4 inverseTransform = glm::inverse(m);
                size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
                for (size_t i = 0; i < numJoints; i++) {
                    Node* jointNode = skin->joints[i];
                    glm::mat4 jointMat = jointNode->GetMatrix() * skin->inverseBindMatrices[i];
                    jointMat = inverseTransform * jointMat;
                    mesh->uniformBlock.jointMatrix[i] = jointMat;
                }
                mesh->uniformBlock.jointcount = (float)numJoints;
                memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
            }
            else {
                memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
            }
        }

        for (auto& child : children) {
            child->Update();
        }
    }

    Node::~Node() {
        if (mesh) {
            delete mesh;
            mesh = nullptr;
        }

        for (auto& child : children) {
            delete child;
        }
    }

    // Model
    void Model::Destroy(VkDevice inDevice) {
        if (vertices.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(inDevice, vertices.buffer, nullptr);
            vkFreeMemory(inDevice, vertices.memory, nullptr);
        }
        if (indices.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(inDevice, indices.buffer, nullptr);
            vkFreeMemory(inDevice, indices.memory, nullptr);
        }
        for (auto texture : textures) {
            texture.Destroy();
        }
        textures.resize(0);
        textureSamplers.resize(0);
        for (auto node : nodes) {
            delete node;
        }
        materials.resize(0);
        animations.resize(0);
        nodes.resize(0);
        linearNodes.resize(0);
        extensions.resize(0);
        skins.resize(0);
    };

    void Model::LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale) {
        Node* newNode = new Node{};
        newNode->index = nodeIndex;
        newNode->parent = parent;
        newNode->name = node.name;
        newNode->skinIndex = node.skin;
        newNode->matrix = glm::mat4(1.0f);

        // Generate local node matrix
        glm::vec3 translation = glm::vec3(0.0f);
        if (node.translation.size() == 3) {
            translation = glm::make_vec3(node.translation.data());
            newNode->translation = translation;
        }
        if (node.rotation.size() == 4) {
            glm::quat q = glm::make_quat(node.rotation.data());
            newNode->rotation = glm::mat4(q);
        }
        glm::vec3 scale = glm::vec3(1.0f);
        if (node.scale.size() == 3) {
            scale = glm::make_vec3(node.scale.data());
            newNode->scale = scale;
        }
        if (node.matrix.size() == 16) {
            newNode->matrix = glm::make_mat4x4(node.matrix.data());
        };

        // Node with children
        if (!node.children.empty()) {
            for (int i : node.children) {
                LoadNode(newNode, model.nodes[i], i, model, indexBuffer, vertexBuffer, globalscale);
            }
        }

        // Node contains mesh data
        if (node.mesh > -1) {
            const tinygltf::Mesh mesh = model.meshes[node.mesh];
            Mesh* newMesh = new Mesh(device, newNode->matrix);
            for (const auto& primitive : mesh.primitives) {
                auto indexStart = static_cast<uint32_t>(indexBuffer.size());
                auto vertexStart = static_cast<uint32_t>(vertexBuffer.size());
                uint32_t indexCount = 0;
                uint32_t vertexCount = 0;
                glm::vec3 posMin{};
                glm::vec3 posMax{};
                bool hasSkin = false;
                bool hasIndices = primitive.indices > -1;
                // Vertices
                {
                    const float* bufferPos = nullptr;
                    const float* bufferNormals = nullptr;
                    const float* bufferTexCoordSet0 = nullptr;
                    const float* bufferTexCoordSet1 = nullptr;
                    const uint16_t* bufferJoints = nullptr;
                    const float* bufferWeights = nullptr;

                    // Position attribute is required
                    assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                    const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
                    bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                    posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                    posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                    vertexCount = static_cast<uint32_t>(posAccessor.count);

                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                        bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                    }

                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    }
                    if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
                        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                        const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    }

                    // Skinning
                    // Joints
                    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
                        const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
                        bufferJoints = reinterpret_cast<const uint16_t*>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
                    }

                    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                        const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                        bufferWeights = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    }

                    hasSkin = (bufferJoints && bufferWeights);

                    for (size_t v = 0; v < posAccessor.count; v++) {
                        Vertex vert{};
                        vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
                        vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
                        vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * 2]) : glm::vec3(0.0f);
                        vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * 2]) : glm::vec3(0.0f);

                        vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
                        vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
                        vertexBuffer.push_back(vert);
                    }
                }
                // Indices
                if (hasIndices) {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                    indexCount = static_cast<uint32_t>(accessor.count);
                    const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                    switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const auto* buf = static_cast<const uint32_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const auto* buf = static_cast<const uint16_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const auto* buf = static_cast<const uint8_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            indexBuffer.push_back(buf[index] + vertexStart);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }
                }
                auto newPrimitive = new Primitive(indexStart, indexCount, vertexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
                newPrimitive->SetBoundingBox(posMin, posMax);
                newMesh->primitives.push_back(newPrimitive);
            }
            // Mesh BB from BBs of primitives
            for (auto p : newMesh->primitives) {
                if (p->bb.valid && !newMesh->bb.valid) {
                    newMesh->bb = p->bb;
                    newMesh->bb.valid = true;
                }
                newMesh->bb._min = glm::min(newMesh->bb._min, p->bb._min);
                newMesh->bb._max = glm::max(newMesh->bb._max, p->bb._max);
            }
            newNode->mesh = newMesh;
        }
        if (parent) {
            parent->children.push_back(newNode);
        }
        else {
            nodes.push_back(newNode);
        }
        linearNodes.push_back(newNode);
    }

    void Model::LoadSkins(tinygltf::Model& gltfModel) {
        for (tinygltf::Skin& source : gltfModel.skins) {
            Skin* newSkin = new Skin{};
            newSkin->name = source.name;

            // Find skeleton root node
            if (source.skeleton > -1) {
                newSkin->skeletonRoot = NodeFromIndex(source.skeleton);
            }

            // Find joint nodes
            for (int jointIndex : source.joints) {
                Node* node = NodeFromIndex(jointIndex);
                if (node) {
                    newSkin->joints.push_back(NodeFromIndex(jointIndex));
                }
            }

            // Get inverse bind matrices from buffer
            if (source.inverseBindMatrices > -1) {
                const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                newSkin->inverseBindMatrices.resize(accessor.count);
                memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
            }

            skins.push_back(newSkin);
        }
    }

    void Model::LoadTextures(tinygltf::Model& gltfModel, Vk::VulkanDevice* inDevice, VkQueue transferQueue) {
        for (tinygltf::Texture& tex : gltfModel.textures) {
            tinygltf::Image image = gltfModel.images[tex.source];
            TextureSampler textureSampler;
            if (tex.sampler == -1) {
                // No sampler specified, use a default one
                textureSampler.magFilter = VK_FILTER_LINEAR;
                textureSampler.minFilter = VK_FILTER_LINEAR;
                textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
            else {
                textureSampler = textureSamplers[tex.sampler];
            }
            ModelTexture texture;
            texture.FromgltfImage(image, textureSampler, inDevice, transferQueue);
            textures.push_back(texture);
        }
    }

    VkSamplerAddressMode Model::GetVkWrapMode(int32_t wrapMode) {
        switch (wrapMode) {
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
        }
    }

    VkFilter Model::GetVkFilterMode(int32_t filterMode) {
        switch (filterMode) {
        case 9728:
            return VK_FILTER_NEAREST;
        case 9729:
            return VK_FILTER_LINEAR;
        case 9984:
            return VK_FILTER_NEAREST;
        case 9985:
            return VK_FILTER_NEAREST;
        case 9986:
            return VK_FILTER_LINEAR;
        case 9987:
            return VK_FILTER_LINEAR;
        default:
            return VK_FILTER_MAX_ENUM;
        }
    }

    void Model::LoadTextureSamplers(tinygltf::Model& gltfModel) {
        for (const tinygltf::Sampler& smpl : gltfModel.samplers) {
            TextureSampler sampler{};
            sampler.minFilter = GetVkFilterMode(smpl.minFilter);
            sampler.magFilter = GetVkFilterMode(smpl.magFilter);
            sampler.addressModeU = GetVkWrapMode(smpl.wrapS);
            sampler.addressModeV = GetVkWrapMode(smpl.wrapT);
            sampler.addressModeW = sampler.addressModeV;
            textureSamplers.push_back(sampler);
        }
    }

    void Model::LoadMaterials(tinygltf::Model& gltfModel) {
        for (tinygltf::Material& mat : gltfModel.materials) {
            Material material{};
            if (mat.values.find("baseColorTexture") != mat.values.end()) {
                material.baseColorTexture = &textures[mat.values["baseColorTexture"].TextureIndex()];
                material.texCoordSets.baseColor = static_cast<uint8_t>(mat.values["baseColorTexture"].TextureTexCoord());
            }
            if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
                material.metallicRoughnessTexture = &textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
                material.texCoordSets.metallicRoughness = static_cast<uint8_t>(mat.values["metallicRoughnessTexture"].TextureTexCoord());
            }
            if (mat.values.find("roughnessFactor") != mat.values.end()) {
                material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
            }
            if (mat.values.find("metallicFactor") != mat.values.end()) {
                material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
            }
            if (mat.values.find("baseColorFactor") != mat.values.end()) {
                material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
            }
            if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
                material.normalTexture = &textures[mat.additionalValues["normalTexture"].TextureIndex()];
                material.texCoordSets.normal = static_cast<uint8_t>(mat.additionalValues["normalTexture"].TextureTexCoord());
            }
            if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
                material.emissiveTexture = &textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
                material.texCoordSets.emissive = static_cast<uint8_t>(mat.additionalValues["emissiveTexture"].TextureTexCoord());
            }
            if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
                material.occlusionTexture = &textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
                material.texCoordSets.occlusion = static_cast<uint8_t>(mat.additionalValues["occlusionTexture"].TextureTexCoord());
            }
            if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
                tinygltf::Parameter param = mat.additionalValues["alphaMode"];
                if (param.string_value == "BLEND") {
                    material.alphaMode = Material::ALPHAMODE_BLEND;
                }
                if (param.string_value == "MASK") {
                    material.alphaCutoff = 0.5f;
                    material.alphaMode = Material::ALPHAMODE_MASK;
                }
            }
            if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
                material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
            }
            if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
                material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
                material.emissiveFactor = glm::vec4(0.0f);
            }

            // Extensions
            // @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
            if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
                auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
                if (ext->second.Has("specularGlossinessTexture")) {
                    auto index = ext->second.Get("specularGlossinessTexture").Get("index");
                    material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
                    auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
                    material.texCoordSets.specularGlossiness = static_cast<uint8_t>(texCoordSet.Get<int>());
                    material.pbrWorkflows.specularGlossiness = true;
                }
                if (ext->second.Has("diffuseTexture")) {
                    auto index = ext->second.Get("diffuseTexture").Get("index");
                    material.extension.diffuseTexture = &textures[index.Get<int>()];
                }
                if (ext->second.Has("diffuseFactor")) {
                    auto factor = ext->second.Get("diffuseFactor");
                    for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
                        auto val = factor.Get(i);
                        material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                    }
                }
                if (ext->second.Has("specularFactor")) {
                    auto factor = ext->second.Get("specularFactor");
                    for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
                        auto val = factor.Get(i);
                        material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                    }
                }
            }

            materials.push_back(material);
        }
        // Push a default material at the end of the list for meshes with no material assigned
        materials.emplace_back();
    }

    void Model::LoadAnimations(tinygltf::Model& gltfModel) {
        for (tinygltf::Animation& anim : gltfModel.animations) {
            Animation animation{};
            animation.name = anim.name;
            if (anim.name.empty()) {
                animation.name = std::to_string(animations.size());
            }

            // Samplers
            for (auto& samp : anim.samplers) {
                AnimationSampler sampler{};

                if (samp.interpolation == "LINEAR") {
                    sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
                }
                if (samp.interpolation == "STEP") {
                    sampler.interpolation = AnimationSampler::InterpolationType::STEP;
                }
                if (samp.interpolation == "CUBICSPLINE") {
                    sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
                }

                // Read sampler input time values
                {
                    const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
                    const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                    const auto buf = static_cast<const float*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        sampler.inputs.push_back(buf[index]);
                    }

                    for (auto input : sampler.inputs) {
                        if (input < animation.start) {
                            animation.start = input;
                        };
                        if (input > animation.end) {
                            animation.end = input;
                        }
                    }
                }

                // Read sampler output T/R/S values 
                {
                    const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
                    const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                    switch (accessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                        const auto buf = static_cast<const glm::vec3*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            sampler.outputsVec4.emplace_back(buf[index], 0.0f);
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        const auto buf = static_cast<const glm::vec4*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++) {
                            sampler.outputsVec4.push_back(buf[index]);
                        }
                        break;
                    }
                    default: {
                        std::cout << "unknown type" << std::endl;
                        break;
                    }
                    }
                }

                animation.samplers.push_back(sampler);
            }

            // Channels
            for (auto& source : anim.channels) {
                AnimationChannel channel{};

                if (source.target_path == "rotation") {
                    channel.path = AnimationChannel::PathType::ROTATION;
                }
                if (source.target_path == "translation") {
                    channel.path = AnimationChannel::PathType::TRANSLATION;
                }
                if (source.target_path == "scale") {
                    channel.path = AnimationChannel::PathType::SCALE;
                }
                if (source.target_path == "weights") {
                    std::cout << "weights not yet supported, skipping channel" << std::endl;
                    continue;
                }
                channel.samplerIndex = source.sampler;
                channel.node = NodeFromIndex(source.target_node);
                if (!channel.node) {
                    continue;
                }

                animation.channels.push_back(channel);
            }

            animations.push_back(animation);
        }
    }

    void Model::LoadFromFile(const std::string& filename, Vk::VulkanDevice* inDevice, VkQueue transferQueue, float scale) {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF gltfContext;
        std::string error;
        std::string warning;

        this->device = inDevice;

        bool binary = false;
        size_t extpos = filename.rfind('.', filename.length());
        if (extpos != std::string::npos) {
            binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
        }

        bool fileLoaded = binary ? gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, filename) : gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

        std::vector<uint32_t> indexBuffer;
        std::vector<Vertex> vertexBuffer;

        if (fileLoaded) {
            LoadTextureSamplers(gltfModel);
            LoadTextures(gltfModel, inDevice, transferQueue);
            LoadMaterials(gltfModel);
            // TODO: scene handling with no default scene
            const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
            for (int i : scene.nodes) {
                const tinygltf::Node node = gltfModel.nodes[i];
                LoadNode(nullptr, node, i, gltfModel, indexBuffer, vertexBuffer, scale);
            }
            if (!gltfModel.animations.empty()) {
                LoadAnimations(gltfModel);
            }
            LoadSkins(gltfModel);

            for (auto node : linearNodes) {
                // Assign skins
                if (node->skinIndex > -1) {
                    node->skin = skins[node->skinIndex];
                }
                // Initial pose
                if (node->mesh) {
                    node->Update();
                }
            }
        }
        else {
            // TODO: throw
            std::cerr << "Could not load gltf file: " << error << std::endl;
            return;
        }

        extensions = gltfModel.extensionsUsed;

        size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
        size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
        indices.count = static_cast<uint32_t>(indexBuffer.size());

        assert(vertexBufferSize > 0);

        struct StagingBuffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
        } vertexStaging, indexStaging;

        // Create staging buffers
        // Vertex data
        CheckResult(inDevice->CreateBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBufferSize,
            &vertexStaging.buffer,
            &vertexStaging.memory,
            vertexBuffer.data()));
        // Index data
        if (indexBufferSize > 0) {
            CheckResult(inDevice->CreateBuffer(
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indexBufferSize,
                &indexStaging.buffer,
                &indexStaging.memory,
                indexBuffer.data()));
        }

        // Create inDevice local buffers
        // Vertex buffer
        CheckResult(inDevice->CreateBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBufferSize,
            &vertices.buffer,
            &vertices.memory));
        // Index buffer
        if (indexBufferSize > 0) {
            CheckResult(inDevice->CreateBuffer(
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBufferSize,
                &indices.buffer,
                &indices.memory));
        }

        // Copy from staging buffers
        VkCommandBuffer copyCmd = inDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion = {};

        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

        if (indexBufferSize > 0) {
            copyRegion.size = indexBufferSize;
            vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
        }

        inDevice->FlushCommandBuffer(copyCmd, transferQueue, true);

        vkDestroyBuffer(inDevice->logicalDevice, vertexStaging.buffer, nullptr);
        vkFreeMemory(inDevice->logicalDevice, vertexStaging.memory, nullptr);
        if (indexBufferSize > 0) {
            vkDestroyBuffer(inDevice->logicalDevice, indexStaging.buffer, nullptr);
            vkFreeMemory(inDevice->logicalDevice, indexStaging.memory, nullptr);
        }

        GetSceneDimensions();
    }

    void Model::DrawNode(Node* node, VkCommandBuffer commandBuffer) {
        if (node->mesh) {
            for (Primitive* primitive : node->mesh->primitives) {
                vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
            }
        }
        for (auto& child : node->children) {
            DrawNode(child, commandBuffer);
        }
    }

    void Model::Draw(VkCommandBuffer commandBuffer) {
        const VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
        for (auto& node : nodes) {
            DrawNode(node, commandBuffer);
        }
    }

    void Model::CalculateBoundingBox(Node* node, Node* parent) {
        BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

        if (node->mesh) {
            if (node->mesh->bb.valid) {
                node->aabb = node->mesh->bb.GetAABB(node->GetMatrix());
                if (node->children.empty()) {
                    node->bvh._min = node->aabb._min;
                    node->bvh._max = node->aabb._max;
                    node->bvh.valid = true;
                }
            }
        }

        parentBvh._min = glm::min(parentBvh._min, node->bvh._min);
        parentBvh._max = glm::min(parentBvh._max, node->bvh._max);

        for (auto& child : node->children) {
            CalculateBoundingBox(child, node);
        }
    }

    void Model::GetSceneDimensions() {
        // Calculate binary volume hierarchy for all nodes in the scene
        for (auto node : linearNodes) {
            CalculateBoundingBox(node, nullptr);
        }

        dimensions.min = glm::vec3(FLT_MAX);
        dimensions.max = glm::vec3(-FLT_MAX);

        for (auto node : linearNodes) {
            if (node->bvh.valid) {
                dimensions.min = glm::min(dimensions.min, node->bvh._min);
                dimensions.max = glm::max(dimensions.max, node->bvh._max);
            }
        }

        // Calculate scene aabb
        aabb = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
        aabb[3][0] = dimensions.min[0];
        aabb[3][1] = dimensions.min[1];
        aabb[3][2] = dimensions.min[2];
    }

    void Model::UpdateAnimation(uint32_t index, float time) {
        if (index > static_cast<uint32_t>(animations.size()) - 1) {
            std::cout << "No animation with index " << index << std::endl;
            return;
        }
        Animation& animation = animations[index];

        bool updated = false;
        for (auto& channel : animation.channels) {
            AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
            if (sampler.inputs.size() > sampler.outputsVec4.size()) {
                continue;
            }

            for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
                if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
                    float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                    if (u <= 1.0f) {
                        switch (channel.path) {
                        case AnimationChannel::PathType::TRANSLATION: {
                            glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                            channel.node->translation = glm::vec3(trans);
                            break;
                        }
                        case AnimationChannel::PathType::SCALE: {
                            glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
                            channel.node->scale = glm::vec3(trans);
                            break;
                        }
                        case AnimationChannel::PathType::ROTATION: {
                            glm::quat q1;
                            q1.x = sampler.outputsVec4[i].x;
                            q1.y = sampler.outputsVec4[i].y;
                            q1.z = sampler.outputsVec4[i].z;
                            q1.w = sampler.outputsVec4[i].w;
                            glm::quat q2;
                            q2.x = sampler.outputsVec4[i + 1].x;
                            q2.y = sampler.outputsVec4[i + 1].y;
                            q2.z = sampler.outputsVec4[i + 1].z;
                            q2.w = sampler.outputsVec4[i + 1].w;
                            channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
                            break;
                        }
                        }
                        updated = true;
                    }
                }
            }
        }
        if (updated) {
            for (auto& node : nodes) {
                node->Update();
            }
        }
    }

    Node* Model::FindNode(Node* parent, uint32_t index) {
        Node* nodeFound = nullptr;
        if (parent->index == index) {
            return parent;
        }
        for (auto& child : parent->children) {
            nodeFound = FindNode(child, index);
            if (nodeFound) {
                break;
            }
        }
        return nodeFound;
    }

    Node* Model::NodeFromIndex(uint32_t index) {
        Node* nodeFound = nullptr;
        for (auto& node : nodes) {
            nodeFound = FindNode(node, index);
            if (nodeFound) {
                break;
            }
        }
        return nodeFound;
    }
}
