#include "Camera.h"
#include "GameManager.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch, glm::vec3 front)
	:
	m_front(glm::vec3(-1.0f, 0.0f, 0.0f)),
	m_movementSpeed(SPEED),
	m_mouseSensitivity(SENSITIVITY),
	m_zoom(ZOOM)
{
	SetPosition(position);
	SetWorldUp(up);
	SetYaw(yaw);
	SetPitch(pitch);
	UpdateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
	:
	m_front(glm::vec3(-1.0f, 0.0f, 0.0f)),
	m_movementSpeed(SPEED),
	m_mouseSensitivity(SENSITIVITY),
	m_zoom(ZOOM)
{
	SetPosition(glm::vec3(posX, posY, posZ));
	SetWorldUp(glm::vec3(upX, upY, upZ));
	SetYaw(yaw);
	SetPitch(pitch);
	UpdateCameraVectors();

}

void Camera::FollowTarget(const glm::vec3& targetPosition, const glm::vec3& targetFront, float distanceBehind, float heightOffset)
{
	glm::vec3 offset = -targetFront * distanceBehind;
	offset.y += heightOffset;
	SetPosition(targetPosition + offset);
	UpdateCameraVectors();
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = GetMovementSpeed() * deltaTime;

	if (direction == FORWARD)
		SetPosition(GetPosition() + GetFront() * velocity);
	if (direction == BACKWARD)
		SetPosition(GetPosition() - GetFront() * velocity);
	if (direction == LEFT)
		SetPosition(GetPosition() - GetRight() * velocity);
	if (direction == RIGHT)
		SetPosition(GetPosition() + GetRight() * velocity);
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch)
{
	xOffset *= GetMouseSensitivity();
	yOffset *= GetMouseSensitivity();

	SetYaw(GetYaw() + xOffset);

	if (GetMode() != PLAYER_FOLLOW)
		SetPitch(GetPitch() + yOffset);

	if (constrainPitch)
	{
		if (GetMode() == FLY)
		{
			if (GetPitch() > 89.0f)
				SetPitch(89.0f);
			if (GetPitch() < -89.0f)
				SetPitch(-89.0f);
		}
		else if (GetMode() == PLAYER_AIM || GetMode() == PLAYER_FOLLOW)
		{
			if (GetPitch() > 9.0f)
				SetPitch(9.0f);
			if (GetPitch() < -20.0f)
				SetPitch(-20.0f);
		}
	}

	UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
	if (GetMode() == FLY)
	{
		SetZoom(GetZoom() - (float)yOffset);
		if (GetZoom() < 1.0f)
			SetZoom(1.0f);
		if (GetZoom() > 45.0f)
			SetZoom(45.0f);
	}
	else if (GetMode() == PLAYER_FOLLOW || GetMode() == ENEMY_FOLLOW)
	{
		SetOffset(GetOffset() - (float)yOffset);
	}

}

void Camera::UpdateCameraVectors()
{
	glm::vec3 front;
	front.x = glm::cos(glm::radians(GetYaw())) * glm::cos(glm::radians(GetPitch()));
	front.y = glm::sin(glm::radians(GetPitch()));
	front.z = glm::sin(glm::radians(GetYaw())) * glm::cos(glm::radians(GetPitch()));

	SetFront(glm::normalize(front));
	SetRight(glm::normalize(glm::cross(GetFront(), GetWorldUp())));
	SetUp(glm::normalize(glm::cross(GetRight(), GetFront())));
}