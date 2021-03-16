// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace GLTF {
    struct Material {

    };
    using Materials = std::vector<Material>;

    struct AABB {
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
    };

    struct Primitive {
        int32_t                materialIndex = INVALID_IDX;
        std::vector<uint16_t>  indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;

        AABB                   bBox;
    };
    using Primitives = std::vector<Primitive>;

    struct Mesh {
        Materials              materials;
        Primitives             primitives;

        AABB                   bBox;
    };
    using Meshes = std::vector<Mesh>;

    struct Node {
        Node*              parent = nullptr;
        std::vector<Node*> children;

        glm::vec3          scale{ 1.0f };
        glm::quat          rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3          translation{ 0.0f };
        glm::mat4          local = glm::mat4(1.0f);

        uint32_t           meshIndex = INVALID_IDX;

        ~Node();
    };

    struct Model {
        Node*  root = nullptr;
        Meshes meshes;

        Model(Node* inRoot) : root(inRoot) {}
        ~Model();
    };
    using ModelUPtr = std::unique_ptr<Model>;

    ModelUPtr Load(std::string&& path);
}
