// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#include "../pch.h"
#include "viewport.h"

namespace Viewport {
    // Projection.
    constexpr glm::mat4 g_clip(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f);

    void Projection::SetScreenWidth(float width) {
        _width = width;
    }

    void Projection::SetScreenHeight(float height) {
        _height = height;
    }

    void Projection::SetFieldOfView(float degree) {
        _fovy = glm::radians(degree);
    }

    void Projection::SetZNear(float zNear) {
        _zNear = zNear;
    }

    void Projection::SetZFar(float zFar) {
        _zFar = zFar;
    }

    glm::mat4 Projection::Get() const {
        const auto aspect = _width / _height;

        return g_clip * glm::perspective(_fovy, aspect, _zNear, _zFar);
    }

    // Camera.
    glm::mat4 Camera::Get() const {
        const auto lookAt = _pos + _at * 100.0f;
        return glm::lookAt(_pos, lookAt, _up);
    }

    void Camera::Update(float delta) {
        auto speed = delta * moveSpeed;
        if (0 != (_keyFlags & CAMERA_MOVE_UP)) {
            _pos += _up * speed;
        }
        if (0 != (_keyFlags & CAMERA_MOVE_DOWN)) {
            _pos -= _up * speed;
        }
        if (0 != (_keyFlags & CAMERA_MOVE_LEFT)) {
            _pos -= _right * speed;
        }
        if (0 != (_keyFlags & CAMERA_MOVE_RIGHT)) {
            _pos += _right * speed;
        }
        if (0 != (_keyFlags & CAMERA_MOVE_FRONT)) {
            _pos += _at * speed;
        }
        if (0 != (_keyFlags & CAMERA_MOVE_BACK)) {
            _pos -= _at * speed;
        }

        speed = delta * rotateSpeed;
        if (std::numeric_limits<float>::epsilon() < std::fabsf(_rotateXDelta)) {
            _at = glm::rotate(glm::mat4(1.0f), _rotateXDelta * speed, _up) * glm::vec4(_at, 0.0f);
        }
        if (std::numeric_limits<float>::epsilon() < std::fabsf(_rotateYDelta)) {
            const glm::vec3 newAt = glm::rotate(glm::mat4(1.0f), _rotateYDelta * speed, _right) * glm::vec4(_at, 0.0f);
            const auto angle = glm::degrees(glm::acos(glm::dot(newAt, _up)));
            if (5.0f < angle) {
                _at = newAt;
            }
        }
        _right = glm::cross(_at, _up);

        _keyFlags = 0;
        _rotateXDelta = 0.0f;
        _rotateYDelta = 0.0f;
    }
}
