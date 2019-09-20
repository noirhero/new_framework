// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk {
    class Camera {
    private:
        float _fov = 0.0f;
        float _zNear = 0.0f;
        float _zFar = 0.0f;

        void UpdateViewMatrix();

    public:
        enum class CameraType : uint8_t { LookAt, FirstPerson };
        CameraType type = CameraType::LookAt;

        glm::vec3 rotation{};
        glm::vec3 position{};

        float rotationSpeed = 1.0f;
        float movementSpeed = 1.0f;

        bool updated = false;

        struct {
            glm::mat4 perspective{ glm::identity<glm::mat4>() };
            glm::mat4 view{ glm::identity<glm::mat4>() };
        } matrices;

        struct {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        } keys;

        bool Moving() const { return keys.left || keys.right || keys.up || keys.down; }
        float GetNearClip() const { return _zNear; }
        float GetFarClip()const { return _zFar; }

        void SetPerspective(float inFov, float aspect, float inZNear, float inZFar);
        void UpdateAspectRatio(float aspect);
        void SetPosition(glm::vec3 inPosition);
        void SetRotation(glm::vec3 inRotation);
        void Rotate(glm::vec3 delta);
        void SetTranslation(glm::vec3 translation);
        void Translate(glm::vec3 delta);
        void Update(float deltaTime);

        // Update camera passing separate axis data (game pad)
        // Returns true if view or position has been changed
        bool UpdatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime);

        glm::vec3 GetCameraPosition() const;
    };
}
