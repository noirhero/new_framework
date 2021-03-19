// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Command {
    class Pool;
}

namespace Image {
    class Sampler;
    class Dimension2;
}

namespace Buffer {
    class Object;
}

namespace GLTF {
    struct Node;
}

namespace Data::Sampler {
    bool               Initialize();
    void               Destroy();
    Image::Sampler&    Get(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW);
}

namespace Data::Texture2D {
    bool               Initialize(Command::Pool& cmdPool);
    void               Destroy();
    Image::Dimension2& Get(std::span<const uint8_t>&& pixels, uint32_t width, uint32_t height, Command::Pool& cmdPool);
    Image::Dimension2& Get(std::string&& fileName, Command::Pool& cmdPool);
}

namespace Data {
    struct AABB {
        glm::vec3          min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3          max = glm::vec3(std::numeric_limits<float>::lowest());
    };

    struct Material {
        Image::Dimension2* albedo = nullptr;
        Image::Sampler*    albedoSampler = nullptr;
        Image::Dimension2* metallicRoughness = nullptr;
        Image::Sampler*    metallicRoughnessSampler = nullptr;
    };

    struct Subset {
        uint32_t indexOffset = 0;
        uint32_t numTri = 0;
        Material material;
    };
    using Subsets = std::vector<Subset>;

    enum class VertexType : uint8_t {
        None, Pos, Nor, UV0
    };
    struct VertexInfo {
        uint32_t   offset = 0;
        uint32_t   stride = 0;
        VertexType type = VertexType::None;
    };
    using VertexInfos = std::vector<VertexInfo>;
	struct VertexDecl {
        uint32_t    stride = 0;
        VertexInfos infos;
	};

    struct Mesh {
        Buffer::Object* vb = nullptr;
        Buffer::Object* ib = nullptr;
        uint32_t        vertexCount = 0;
        uint32_t        indexCount = 0;
        VertexDecl      vertexDecl;
        Subsets         subsets;
    };

    struct Bone {
        int32_t   parent = INVALID_IDX;
        int32_t   depth = 0;
        AABB      bBox;
        glm::mat4 local{ 1.0f };
    };
    using Bones = std::vector<Bone>;

    struct Model {
        AABB     bBox;
        Mesh     mesh;
        Bones    bones;

        Model() = default;
        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;
    };
}
