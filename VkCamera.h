// Copyright 2018-2019 TAP, Inc. All Rights Reserved.

#pragma once

namespace Vk
{
	class Camera
	{
	private:
		float fov = 0.0f;
		float znear = 0.0f;
		float zfar = 0.0f;

		void updateViewMatrix();

	public:
		enum class CameraType : uint8_t { LookAt, FirstPerson };
		CameraType type = CameraType::LookAt;

		glm::vec3 rotation = glm::vec3();
		glm::vec3 position = glm::vec3();

		float rotationSpeed = 1.0f;
		float movementSpeed = 1.0f;

		bool updated = false;

		struct
		{
			glm::mat4 perspective;
			glm::mat4 view;
		} matrices;

		struct
		{
			bool left = false;
			bool right = false;
			bool up = false;
			bool down = false;
		} keys;

		bool moving() const { return keys.left || keys.right || keys.up || keys.down; }
		float getNearClip() const { return znear; }
		float getFarClip()const { return zfar; }

		void setPerspective(float inFov, float aspect, float inZNear, float inZFar);
		void updateAspectRatio(float aspect);
		void setPosition(glm::vec3 inPosition);
		void setRotation(glm::vec3 inRotation);
		void rotate(glm::vec3 delta);
		void setTranslation(glm::vec3 translation);
		void translate(glm::vec3 delta);
		void update(float deltaTime);

		// Update camera passing separate axis data (gamepad)
		// Returns true if view or position has been changed
		bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime);

        glm::vec3 GetCameraPosition() const;
	};
}
