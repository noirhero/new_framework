// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#include "stdafx.h"
#include "VkCamera.h"

namespace Vk {
    void Camera::UpdateViewMatrix() {
        glm::mat4 rotM{ glm::identity<glm::mat4>() };

        rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        const auto transM = glm::translate(glm::mat4(1.0f), position * glm::vec3(1.0f, 1.0f, -1.0f));

        if (type == CameraType::FirstPerson) {
            matrices.view = rotM * transM;
        }
        else {
            matrices.view = transM * rotM;
        }

        updated = true;
    }

    void Camera::SetPerspective(float inFov, float aspect, float inZNear, float inZFar) {
        _fov = inFov;
        _zNear = inZNear;
        _zFar = inZFar;
        matrices.perspective = glm::perspective(glm::radians(inFov), aspect, inZNear, inZFar);
    };

    void Camera::UpdateAspectRatio(float aspect) {
        matrices.perspective = glm::perspective(glm::radians(_fov), aspect, _zNear, _zFar);
    }

    void Camera::SetPosition(glm::vec3 inPosition) {
        position = inPosition;
        UpdateViewMatrix();
    }

    void Camera::SetRotation(glm::vec3 inRotation) {
        rotation = inRotation;
        UpdateViewMatrix();
    };

    void Camera::Rotate(glm::vec3 delta) {
        rotation += delta;
        UpdateViewMatrix();
    }

    void Camera::SetTranslation(glm::vec3 translation) {
        position = translation;
        UpdateViewMatrix();
    };

    void Camera::Translate(glm::vec3 delta) {
        position += delta;
        UpdateViewMatrix();
    }

    void Camera::Update(float deltaTime) {
        updated = false;
        if (type == CameraType::FirstPerson) {
            if (Moving()) {
                glm::vec3 camFront;
                camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
                camFront.y = sin(glm::radians(rotation.x));
                camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
                camFront = glm::normalize(camFront);

                const float moveSpeed = deltaTime * movementSpeed;

                if (keys.up)
                    position += camFront * moveSpeed;
                if (keys.down)
                    position -= camFront * moveSpeed;
                if (keys.left)
                    position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
                if (keys.right)
                    position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

                UpdateViewMatrix();
            }
        }
    }

    bool Camera::UpdatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime) {
        bool retVal = false;

        if (type == CameraType::FirstPerson) {
            // Use the common console thumbstick layout		
            // Left = view, right = move

            const float deadZone = 0.0015f;
            const float range = 1.0f - deadZone;

            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
            camFront.y = sin(glm::radians(rotation.x));
            camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
            camFront = glm::normalize(camFront);

            const float moveSpeed = deltaTime * movementSpeed * 2.0f;
            const float rotSpeed = deltaTime * rotationSpeed * 50.0f;

            // Move
            if (fabsf(axisLeft.y) > deadZone) {
                const float pos = (fabsf(axisLeft.y) - deadZone) / range;
                position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
                retVal = true;
            }
            if (fabsf(axisLeft.x) > deadZone) {
                const float pos = (fabsf(axisLeft.x) - deadZone) / range;
                position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
                retVal = true;
            }

            // Rotate
            if (fabsf(axisRight.x) > deadZone) {
                const float pos = (fabsf(axisRight.x) - deadZone) / range;
                rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
                retVal = true;
            }
            if (fabsf(axisRight.y) > deadZone) {
                const float pos = (fabsf(axisRight.y) - deadZone) / range;
                rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
                retVal = true;
            }
        }
        else {
            // todo: move code from example base class for look-at
        }

        if (retVal) {
            UpdateViewMatrix();
        }

        return retVal;
    }

    glm::vec3 Camera::GetCameraPosition() const {
        return {
            -position.z * glm::sin(glm::radians(rotation.y)) * glm::cos(glm::radians(rotation.x)),
            -position.z * glm::sin(glm::radians(rotation.x)),
            position.z * glm::cos(glm::radians(rotation.y)) * glm::cos(glm::radians(rotation.x))
        };
    }

}
