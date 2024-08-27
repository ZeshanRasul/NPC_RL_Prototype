#include "Camera.h"
#include "GameManager.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
	:
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	MovementSpeed(SPEED),
	MouseSensitivity(SENSITIVITY),
	Zoom(ZOOM)
{
	Position = position;
	WorldUp = up;
	Yaw = yaw;
	Pitch = pitch;
	UpdateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
	:
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	MovementSpeed(SPEED),
	MouseSensitivity(SENSITIVITY),
	Zoom(ZOOM)
{
	Position = glm::vec3(posX, posY, posZ);
	WorldUp = glm::vec3(upX, upY, upZ);
	Yaw = yaw;
	Pitch = pitch;
	UpdateCameraVectors();

}

void Camera::FollowTarget(const glm::vec3& targetPosition, const glm::vec3& targetFront, float distanceBehind, float heightOffset)
{
	glm::vec3 offset = -targetFront * distanceBehind;
	offset.y += heightOffset;
	Position = targetPosition + offset;
	UpdateCameraVectors();
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;

	if (direction == FORWARD)
		Position += Front * velocity;
	if (direction == BACKWARD)
		Position -= Front * velocity;
	if (direction == LEFT)
		Position -= Right * velocity;
	if (direction == RIGHT)
		Position += Right * velocity;
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch)
{
	xOffset *= MouseSensitivity;
	yOffset *= MouseSensitivity;

	Yaw += xOffset;
	Pitch += yOffset;

	if (constrainPitch)
	{
		if (Mode == FLY)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		} 
		else if (Mode == PLAYER_AIM)
		{
			if (Pitch > 19.0f)
				Pitch = 19.0f;
			if (Pitch < -19.0f)
				Pitch = -19.0f;
		}
	}

	UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
	if (Mode == FLY)
	{
		Zoom -= (float)yOffset;
		if (Zoom < 1.0f)
			Zoom = 1.0f;
		if (Zoom > 45.0f)
			Zoom = 45.0f;
	}
	else if (Mode == PLAYER_FOLLOW || Mode == ENEMY_FOLLOW)
	{
		Offset -= (float)yOffset;
	}

}

void Camera::UpdateCameraVectors()
{
	glm::vec3 front;
	front.x = glm::cos(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));
	front.y = glm::sin(glm::radians(Pitch));
	front.z = glm::sin(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));

	Front = glm::normalize(front);
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));
}