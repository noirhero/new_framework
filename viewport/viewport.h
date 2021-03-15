// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Viewport {
    void      SetScreenWidth(float width);
    void      SetScreenHeight(float height);
    void      SetFieldOfView(float degree);
    void      SetZNear(float zNear);
    void      SetZFar(float zFar);

    glm::mat4 GetProjection();
}

namespace Viewport {
    enum CameraMoveFlag : uint32_t {
        CAMERA_MOVE_UP    = 1 << 0,
        CAMERA_MOVE_DOWN  = 1 << 1,
        CAMERA_MOVE_LEFT  = 1 << 2,
        CAMERA_MOVE_RIGHT = 1 << 3,
        CAMERA_MOVE_FRONT = 1 << 4,
        CAMERA_MOVE_BACK  = 1 << 5,
    };

    class Camera {
    public:
        glm::mat4 Get() const;

        void      MoveUp()    { _keyFlags |= CAMERA_MOVE_UP; }
        void      MoveDown()  { _keyFlags |= CAMERA_MOVE_DOWN; }
        void      MoveLeft()  { _keyFlags |= CAMERA_MOVE_LEFT; }
        void      MoveRight() { _keyFlags |= CAMERA_MOVE_RIGHT; }
        void      MoveFront() { _keyFlags |= CAMERA_MOVE_FRONT; }
        void      MoveBack()  { _keyFlags |= CAMERA_MOVE_BACK; }
        void      RotateX(float delta) { _rotateXDelta += delta; }
        void      RotateY(float delta) { _rotateYDelta += delta; }
        void      Update(float delta);

    private:
        glm::vec3 _pos = { 0.0f, 0.0f, -5.0f };
        glm::vec3 _at = { 0.0f, 0.0f, 1.0f };
        glm::vec3 _right = { 1.0f, 0.0f, 0.0f };
        glm::vec3 _up = { 0.0f, 1.0f, 0.0f };

        uint32_t  _keyFlags = 0;
        float     _rotateXDelta = 0.0f;
        float     _rotateYDelta = 0.0f;

    public:
        float     moveSpeed = 1.0f;
        float     rotateSpeed = 1.0f;
    };
}
