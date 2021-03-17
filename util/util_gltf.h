// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace GLTF {
    struct Sampler {
        VkFilter               magFilter = VK_FILTER_LINEAR;
        VkFilter               minFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode   modeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode   modeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };
    using Samplers = std::vector<Sampler>;

    struct Texture {
        int32_t                samplerIndex = INVALID_IDX;

        uint32_t               width = 0;
        uint32_t               height = 0;
        std::vector<uint8_t>   buffer;
    };
    using Textures = std::vector<Texture>;

    struct Material {
        enum class Alpha : int32_t {
            Opaque,
            Mask,
            Blend,
        };

        int32_t                albedoIndex = INVALID_IDX;
        int32_t                albedoUVIndex = INVALID_IDX;

        int32_t                normalIndex = INVALID_IDX;
        int32_t                normalUVIndex = INVALID_IDX;

        int32_t                metallicRoughnessIndex = INVALID_IDX;
        int32_t                metallicRoughnessUVIndex = INVALID_IDX;

        Alpha                  alphaMode = Alpha::Opaque;
        float                  alphaCutoff = 0.0f;
        float                  roughness = 0.0f;
        float                  metallic = 0.0f;
    };
    using Materials = std::vector<Material>;

    struct AABB {
        glm::vec3              min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3              max = glm::vec3(std::numeric_limits<float>::lowest());
    };

    struct Primitive {
        int32_t                materialIndex = INVALID_IDX;
        std::vector<uint16_t>  indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uv0;

        AABB                   bBox;
    };
    using Primitives = std::vector<Primitive>;

    struct Mesh {
        Primitives             primitives;
        AABB                   bBox;
    };
    using Meshes = std::vector<Mesh>;

    struct Node {
        Node*                  parent = nullptr;
        std::vector<Node*>     children;

        glm::vec3              scale{ 1.0f };
        glm::quat              rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3              translation{ 0.0f };
        glm::mat4              local = glm::mat4(1.0f);

        int32_t                meshIndex = INVALID_IDX;

        ~Node();
    };

    struct Model {
        Node                   root;
        Meshes                 meshes;
        Samplers               samplers;
        Textures               textures;
        Materials              materials;
    };
    using ModelUPtr = std::unique_ptr<Model>;

    ModelUPtr Load(std::string&& path);
}
