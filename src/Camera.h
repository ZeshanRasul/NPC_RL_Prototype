#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

enum CameraMode {
	FLY,
	PLAYER_FOLLOW,
	ENEMY_FOLLOW,
	PLAYER_AIM,
	MODE_COUNT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 7.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	glm::vec3 Offset;

	CameraMode Mode = PLAYER_FOLLOW;

	float Yaw;
	float Pitch;

	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	float playerCamRearOffset = 15.0f;
	float playerCamHeightOffset = 5.0f;
	float enemyCamRearOffset = 15.0f;
	float enemyCamHeightOffset = 5.0f;

	Camera(glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

	glm::mat4 GetViewMatrix() const { return lookAt(Position, Position + Front, Up); }

	glm::mat4 GetViewMatrixPlayerFollow(const glm::vec3& targetPos, const glm::vec3& targetUp) const { return lookAt(Position, targetPos, targetUp); }
	glm::mat4 GetViewMatrixEnemyFollow(const glm::vec3& targetPos, const glm::vec3& targetUp) const { return lookAt(Position, targetPos, targetUp); }

	void FollowTarget(const glm::vec3& targetPosition, const glm::vec3& playerFront, float distanceBehind, float heightOffset);

	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(float yOffset);

	void UpdateCameraVectors();
private:
};