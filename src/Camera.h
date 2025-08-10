#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	STATIONARY
};

enum CameraMode {
	FLY,
	PLAYER_FOLLOW,
	ENEMY_FOLLOW,
	PLAYER_AIM,
	MODE_COUNT
};

const float YAW = -180.0f;
const float PITCH = 0.0f;
const float SPEED = 50.0f;
const float SENSITIVITY = 0.3f;
const float ZOOM = 35.0f;

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

	float playerCamRearOffset = 27.0f;
	float playerCamHeightOffset = 12.0f;
	float playerPosOffset = 15.0f;
	float playerAimRightOffset = 5.0f;
	float enemyCamRearOffset = 17.0f;
	float enemyCamHeightOffset = 15.0f;

	Camera(glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH, glm::vec3 front = glm::vec3(-1.0f, 0.0f, 0.0f));
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

	glm::mat4 GetViewMatrix() const { return lookAt(GetPosition(), GetPosition() + GetFront(), GetUp()); }

	glm::mat4 GetViewMatrixPlayerFollow(const glm::vec3& targetPos, const glm::vec3& targetUp) const { return lookAt(GetPosition(), targetPos, targetUp); }
	glm::mat4 GetViewMatrixEnemyFollow(const glm::vec3& targetPos, const glm::vec3& targetUp) const { return lookAt(GetPosition(), targetPos, targetUp); }

	void FollowTarget(const glm::vec3& targetPosition, const glm::vec3& playerFront, float distanceBehind, float heightOffset);

	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(float yOffset);

	void UpdateCameraVectors();

	glm::vec3 GetPosition() const { return m_position; }
	void SetPosition(glm::vec3 val) { m_position = val; }

	glm::vec3 GetFront() const { return m_front; }
	void SetFront(glm::vec3 val) { m_front = val; }

	glm::vec3 GetUp() const { return m_up; }
	void SetUp(glm::vec3 val) { m_up = val; }

	glm::vec3 GetRight() const { return m_right; }
	void SetRight(glm::vec3 val) { m_right = val; }

	glm::vec3 GetWorldUp() const { return m_worldUp; }
	void SetWorldUp(glm::vec3 val) { m_worldUp = val; }

	glm::vec3 GetOffset() const { return m_offset; }
	void SetOffset(glm::vec3 val) { m_offset = val; }

	CameraMode GetMode() const { return m_mode; }
	void SetMode(CameraMode val) { m_mode = val; }

	float GetYaw() const { return m_yaw; }
	void SetYaw(float val) { m_yaw = val; }

	float GetPitch() const { return m_pitch; }
	void SetPitch(float val) { m_pitch = val; }

	float GetMovementSpeed() const { return m_movementSpeed; }
	void SetMovementSpeed(float val) { m_movementSpeed = val; }

	float GetMouseSensitivity() const { return m_mouseSensitivity; }
	void SetMouseSensitivity(float val) { m_mouseSensitivity = val; }

	float GetZoom() const { return m_zoom; }
	void SetZoom(float val) { m_zoom = val; }

	float GetPlayerCamRearOffset() const { return m_playerCamRearOffset; }
	void SetPlayerCamRearOffset(float val) { m_playerCamRearOffset = val; }

	float GetPlayerCamHeightOffset() const { return m_playerCamHeightOffset; }
	void SetPlayerCamHeightOffset(float val) { m_playerCamHeightOffset = val; }

	float GetPlayerPosOffset() const { return m_playerPosOffset; }
	void SetPlayerPosOffset(float val) { m_playerPosOffset = val; }

	float GetPlayerAimRightOffset() const { return m_playerAimRightOffset; }
	void SetPlayerAimRightOffset(float val) { m_playerAimRightOffset = val; }

	float GetEnemyCamRearOffset() const { return m_enemyCamRearOffset; }
	void SetEnemyCamRearOffset(float val) { m_enemyCamRearOffset = val; }

	float GetEnemyCamHeightOffset() const { return m_enemyCamHeightOffset; }
	void SetEnemyCamHeightOffset(float val) { m_enemyCamHeightOffset = val; }

	bool HasSwitched() const { return hasSwitched; }
	void StorePrevCam(const glm::vec3& prevPos, const glm::vec3& targetPos);
	void LerpCamera();
	glm::mat4 UpdateCameraLerp(const glm::vec3& newPos, const glm::vec3& targetPos, const glm::vec3& front, const glm::vec3& up, float dt);

	glm::vec3 prevCamPos;
	glm::vec3 targetCamPos;
	glm::vec3 prevCamTarget;
	glm::vec3 targetCamTarget;
	glm::vec3 prevCamDir;
	glm::vec3 targetCamDir;
	float prevCamPitch;
	float targetCamPitch;
	bool hasSwitched = false;
	glm::vec3 target;
	bool isBlending = false;
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_up;
	glm::vec3 m_right;
	float m_yaw;
	glm::vec3 m_worldUp;
	glm::vec3 m_offset;
	float m_pitch;
	float m_movementSpeed;
	float m_mouseSensitivity;
	float m_zoom;

	float cameraBlendTime = 1.3f;
	float GetPlayerAimCamRearOffset() const { return m_playerAimCamRearOffset; }
	void SetPlayerAimCamRearOffset(float val) { m_playerAimCamRearOffset = val; }
	float GetPlayerAimCamHeightOffset() const { return m_playerAimCamHeightOffset; }
	void SetPlayerAimCamHeightOffset(float val) { m_playerAimCamHeightOffset = val; }
private:
	CameraMode m_mode = PLAYER_FOLLOW;
	float m_playerCamRearOffset = 33.0f;
	float m_playerAimCamRearOffset = 27.0f;
	float m_playerCamHeightOffset = 15.0f;
	float m_playerAimCamHeightOffset = 12.0f;
	float m_playerPosOffset = 12.0f;
	float m_playerAimRightOffset = 5.0f;
	float m_enemyCamRearOffset = 15.0f;
	float m_enemyCamHeightOffset = 5.0f;

	float cameraBlendTimer = 0.0f;
};