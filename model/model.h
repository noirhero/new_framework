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

namespace Model::Sampler {
    bool               Initialize();
    void               Destroy();
    Image::Sampler&    Get(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode modeU, VkSamplerAddressMode modeV, VkSamplerAddressMode modeW);
}

namespace Model::Texture2D {
    bool               Initialize(Command::Pool& cmdPool);
    void               Destroy();
    Image::Dimension2& Get(std::string&& key, std::span<const uint8_t>&& pixels, uint32_t width, uint32_t height, Command::Pool& cmdPool);
}

namespace Model {
    struct Material {
        Image::Sampler*    sampler = nullptr;
        Image::Dimension2* albedo = nullptr;
        Image::Dimension2* roughnessMetallic = nullptr;
    };

    struct Subset {
        Material material;
        uint32_t indexOffset = 0;
        uint32_t numTri = 0;
    };
    using Subsets = std::vector<Subset>;

    struct Mesh {
        Buffer::Object* vb = nullptr;
        Buffer::Object* ib = nullptr;
        Subsets         subsets;
    };

    struct Bone {
        int32_t parent = INVALID_IDX;
        int32_t depth = 0;
    };
    using Bones = std::vector<Bone>;
    using Transforms = std::vector<glm::mat4>;

    struct Skeletal {
        Bones      _bones;
        Transforms _locals;
    };

    struct Data {
        Mesh     mesh;
        Skeletal skeletal;
    };

    //Data& Get(std::string&& path);
}
