// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

// Changing this value here also requires changing it in the vertex shader
constexpr auto MAX_NUM_JOINTS = 128u;

namespace tinygltf {
    struct Image;
    class Node;
    class Model;
}

namespace Vk {
    struct VulkanDevice;
    struct Node;

    struct BoundingBox {
        glm::vec3 _min = { 0.0f, 0.0f, 0.0f };
        glm::vec3 _max = { 0.0f, 0.0f, 0.0f };
        bool valid = false;

        BoundingBox() = default;
        BoundingBox(glm::vec3 min, glm::vec3 max) : _min(min), _max(max) {}

        BoundingBox GetAABB(glm::mat4 m);
    };

    /*
        glTF texture sampler
    */
    struct TextureSampler {
        VkFilter magFilter = VK_FILTER_NEAREST;
        VkFilter minFilter = VK_FILTER_NEAREST;
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };

    /*
        glTF texture loading class
    */
    struct ModelTexture {
        Vk::VulkanDevice* device = nullptr;
        VkImage image = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 0;
        uint32_t layerCount = 0;
        VkDescriptorImageInfo descriptor{};
        VkSampler sampler = VK_NULL_HANDLE;

        void UpdateDescriptor();
        void Destroy();

        /*
            Load a texture from a glTF image (stored as vector of chars loaded via stb_image)
            Also generates the mip chain as glTF images are stored as jpg or png without any mips
        */
        void FromgltfImage(tinygltf::Image& gltfimage, TextureSampler textureSampler, Vk::VulkanDevice* inDevice, VkQueue copyQueue);
    };

    /*
        glTF material class
    */
    struct Material {
        enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff = 1.0f;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        glm::vec4 emissiveFactor = glm::vec4(1.0f);
        ModelTexture* baseColorTexture = nullptr;
        ModelTexture* metallicRoughnessTexture = nullptr;
        ModelTexture* normalTexture = nullptr;
        ModelTexture* occlusionTexture = nullptr;
        ModelTexture* emissiveTexture = nullptr;
        struct TexCoordSets {
            uint8_t baseColor = 0;
            uint8_t metallicRoughness = 0;
            uint8_t specularGlossiness = 0;
            uint8_t normal = 0;
            uint8_t occlusion = 0;
            uint8_t emissive = 0;
        } texCoordSets;
        struct Extension {
            ModelTexture* specularGlossinessTexture = nullptr;
            ModelTexture* diffuseTexture = nullptr;
            glm::vec4 diffuseFactor = glm::vec4(1.0f);
            glm::vec3 specularFactor = glm::vec3(0.0f);
        } extension;
        struct PbrWorkflows {
            bool metallicRoughness = true;
            bool specularGlossiness = false;
        } pbrWorkflows;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    /*
        glTF primitive
    */
    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t vertexCount;
        Material& material;
        bool hasIndices;

        BoundingBox bb;

        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
            hasIndices = indexCount > 0;
        };

        void SetBoundingBox(glm::vec3 min, glm::vec3 max) {
            bb._min = min;
            bb._max = max;
            bb.valid = true;
        }
    };

    /*
        glTF mesh
    */
    struct Mesh {
        Vk::VulkanDevice* device = nullptr;

        std::vector<Primitive*> primitives;

        BoundingBox bb;
        BoundingBox aabb;

        struct UniformBuffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkDescriptorBufferInfo descriptor{};
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            void* mapped = nullptr;
        } uniformBuffer;

        struct UniformBlock {
            glm::mat4 matrix{ glm::identity<glm::mat4>() };
            glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
            float jointcount{ 0 };
        } uniformBlock;

        Mesh(Vk::VulkanDevice* device, glm::mat4 matrix);
        ~Mesh();

        void SetBoundingBox(glm::vec3 min, glm::vec3 max) {
            bb._min = min;
            bb._max = max;
            bb.valid = true;
        }
    };

    /*
        glTF skin
    */
    struct Skin {
        std::string name;
        Node* skeletonRoot = nullptr;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Node*> joints;
    };

    /*
        glTF node
    */
    struct Node {
        Node* parent = nullptr;
        uint32_t index = 0;
        std::vector<Node*> children;
        glm::mat4 matrix{ glm::identity<glm::mat4>() };
        std::string name;
        Mesh* mesh = nullptr;
        Skin* skin = nullptr;
        int32_t skinIndex = -1;
        glm::vec3 translation{};
        glm::vec3 scale{ 1.0f };
        glm::quat rotation{};
        BoundingBox bvh;
        BoundingBox aabb;

        glm::mat4 LocalMatrix();
        glm::mat4 GetMatrix();

        void Update();

        ~Node();
    };

    /*
        glTF animation channel
    */
    struct AnimationChannel {
        enum PathType { TRANSLATION, ROTATION, SCALE };
        PathType path;
        Node* node = nullptr;
        uint32_t samplerIndex = 0;
    };

    /*
        glTF animation sampler
    */
    struct AnimationSampler {
        enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
        InterpolationType interpolation;
        std::vector<float> inputs;
        std::vector<glm::vec4> outputsVec4;
    };

    /*
        glTF animation
    */
    struct Animation {
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float start = std::numeric_limits<float>::max();
        float end = std::numeric_limits<float>::min();
    };

    /*
        glTF model loading and rendering class
    */
    struct Model {
        Vk::VulkanDevice* device = nullptr;

        struct Vertex {
            glm::vec3 pos{};
            glm::vec3 normal{};
            glm::vec2 uv0{};
            glm::vec2 uv1{};
            glm::vec4 joint0{};
            glm::vec4 weight0{};
        };

        struct Vertices {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
        } vertices;

        struct Indices {
            int count;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = NULL;
        } indices;

        glm::mat4 aabb{ glm::identity<glm::mat4>() };

        std::vector<Node*> nodes;
        std::vector<Node*> linearNodes;

        std::vector<Skin*> skins;

        std::vector<ModelTexture> textures;
        std::vector<TextureSampler> textureSamplers;
        std::vector<Material> materials;
        std::vector<Animation> animations;
        std::vector<std::string> extensions;

        struct Dimensions {
            glm::vec3 min = glm::vec3(FLT_MAX);
            glm::vec3 max = glm::vec3(-FLT_MAX);
        } dimensions;

        void Destroy(VkDevice inDevice);
        void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale);
        void LoadSkins(tinygltf::Model& gltfModel);
        void LoadTextures(tinygltf::Model& gltfModel, Vk::VulkanDevice* inDevice, VkQueue transferQueue);
        VkSamplerAddressMode GetVkWrapMode(int32_t wrapMode);
        VkFilter GetVkFilterMode(int32_t filterMode);
        void LoadTextureSamplers(tinygltf::Model& gltfModel);
        void LoadMaterials(tinygltf::Model& gltfModel);
        void LoadAnimations(tinygltf::Model& gltfModel);
        void LoadFromFile(const std::string& filename, Vk::VulkanDevice* inDevice, VkQueue transferQueue, float scale = 1.0f);
        void DrawNode(Node* node, VkCommandBuffer commandBuffer);
        void Draw(VkCommandBuffer commandBuffer);
        void CalculateBoundingBox(Node* node, Node* parent);
        void GetSceneDimensions();
        void UpdateAnimation(uint32_t index, float time);

        /*
            Helper functions
        */
        Node* FindNode(Node* parent, uint32_t index);
        Node* NodeFromIndex(uint32_t index);
    };
}
