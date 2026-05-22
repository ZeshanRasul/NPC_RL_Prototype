#pragma once

#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "Components/CombatComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Model/GLTFPrimitive.h"

enum PlayerState
{
	MOVING,
	AIMING,
	SHOOTING,
	PLAYER_STATE_COUNT
};

class Player : public GameObject
{
public:
	Player(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, float yaw);

	~Player()
	{
		m_model->cleanup();
	}

	// --- Animation interface (delegates to m_skinnedMesh) ---

	std::vector<glm::mat2x4> getJointDualQuats()
	{
		return m_skinnedMesh.GetJointDualQuats();
	}

	int GetJointDualQuatsSize()
	{
		return m_skinnedMesh.GetJointDualQuatsSize();
	}

	std::string GetNodeName(int nodeNum)
	{
		return m_skinnedMesh.GetNodeName(nodeNum);
	}

	void ResetNodeData()
	{
		m_skinnedMesh.ResetNodeData();
	}

	void BlendAnimationFrame(int animNum, float time, float blendFactor)
	{
		m_skinnedMesh.BlendAnimationFrame(animNum, time, blendFactor);
	}

	void PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
	{
		m_skinnedMesh.PlayAnimation(animNum, speedDivider, blendFactor, playBackwards);
	}

	float GetAnimationEndTime(int animNum)
	{
		return m_skinnedMesh.GetAnimationEndTime(animNum);
	}

	std::string GetClipName(int animNum)
	{
		return m_skinnedMesh.GetClipName(animNum);
	}

	// --- Rendering mesh data (GL VAOs/VBOs, stays on Player) ---

	struct GLTFMesh {
		std::vector<GLTFPrimitive> primitives;
	};

	std::vector<GLTFMesh> meshData;
	std::vector<GLuint> glTextures;

	void SetupGLTFMeshes(tinygltf::Model* model);
	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos);
	std::vector<GLuint> LoadGLTFTextures(tinygltf::Model* model);

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}

	// --- GameObject interface ---

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
		glm::vec3 camPos) override;

	void Update(float dt, bool isPaused, bool isTimeScaled);

	glm::vec3 GetPosition()
	{
		return m_position;
	}

	void SetPosition(glm::vec3 newPos)
	{
		m_position = newPos;
		m_recomputeWorldTransform = true;
	}

	float GetInitialYaw() const { return m_initialYaw; }

	void SetYaw(float newYaw)
	{
		m_yaw = newYaw;
		m_recomputeWorldTransform = true;
	};

	void ComputeAudioWorldTransform() override;

	void UpdatePlayerVectors();
	void UpdatePlayerAimVectors();

	void PlayerProcessKeyboard(CameraMovement direction, float deltaTime);
	void PlayerProcessMouseMovement(float xOffset);

	float GetVelocity() const { return m_velocity; }
	void SetVelocity(float newVelocity) { m_velocity = newVelocity; }

	PlayerState GetPlayerState() const { return m_playerState; }
	void SetPlayerState(PlayerState newState);

	glm::vec3 GetShootPos()
	{
		return GetPosition() + glm::vec3(0.0f, 4.5f, 0.0f) + (4.5f * m_playerAimFront) + (-0.5f * m_playerAimRight);
	}

	float GetShootDistance() const { return m_shootDistance; }
	glm::vec3 GetPlayerHitPoint() const { return m_playerShootHitPoint; }

	void Shoot();

	void SetCameraMatrices(glm::mat4 viewMat, glm::mat4 proj)
	{
		m_view = viewMat;
		m_projection = proj;
	}

	void SetUpAABB();
	void SetAABBShader(Shader* aabbShdr) { m_aabbShader = aabbShdr; }
	void UpdateAabb()
	{
		glm::mat4 modelMatrix = translate(glm::mat4(1.0f), m_position) *
			rotate(glm::mat4(1.0f), glm::radians(-m_yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), m_scale);
		m_aabb->Update(modelMatrix);
	}

	AABB* GetAABB() const { return m_aabb; }
	void SetAabbColor(glm::vec3 color) { m_aabbColor = color; }

	void OnHit() override;

	void OnMiss() override
	{
		std::cout << "Player was missed!" << std::endl;
		SetAabbColor(glm::vec3(1.0f, 1.0f, 1.0f));
	};

	void OnDeath();

	float GetHealth() const { return m_combat.health; }
	void SetHealth(float newHealth) { m_combat.health = newHealth; }

	void TakeDamage(float damage)
	{
		m_combat.ApplyDamage(damage);
		if (m_combat.isDead)
			OnDeath();
	}

	int GetAnimNum() const { return m_animNum; }
	int GetSourceAnimNum() const { return m_sourceAnim; }
	int GetDestAnimNum() const { return m_destAnim; }
	void SetAnimNum(int newAnimNum) { m_animNum = newAnimNum; }
	void SetSourceAnimNum(int newSrcAnim) { m_sourceAnim = newSrcAnim; }

	void SetDestAnimNum(int newDestAnim)
	{
		m_destAnim = newDestAnim;
		m_destAnimSet = true;
	}

	void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards);
	void SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playAnimBackwards);

	void ResetGame();

	void HasDealtDamage() override {}
	void HasKilledPlayer() override {}

	float GetPlayerYaw() const { return m_playerYaw; }
	void SetPlayerYaw(float val) { m_playerYaw = val; }

	float GetAimPitch() const { return m_aimPitch; }
	void SetAimPitch(float val) { m_aimPitch = val; }

	CameraMovement GetPrevDirection() const { return m_prevDirection; }
	void SetPrevDirection(CameraMovement val) { m_prevDirection = val; }

	glm::vec3 GetPlayerFront() const { return m_playerFront; }
	void SetPlayerFront(glm::vec3 val) { m_playerFront = val; }

	glm::vec3 GetPlayerRight() const { return m_playerRight; }
	void SetPlayerRight(glm::vec3 val) { m_playerRight = val; }

	glm::vec3 GetPlayerAimFront() const { return m_playerAimFront; }
	void SetPlayerAimFront(glm::vec3 val) { m_playerAimFront = val; }

	glm::vec3 GetPlayerAimUp() const { return m_playerAimUp; }
	void SetPlayerAimUp(glm::vec3 val) { m_playerAimUp = val; }

	glm::vec3 GetInitialPos() const { return m_initialPos; }
	void SetInitialPos(glm::vec3 val) { m_initialPos = val; }

	Shader* m_aabbShader;
	CameraMovement m_prevDirection = STATIONARY;

	float m_playerYaw;
	glm::vec3 m_playerFront;
	glm::vec3 m_playerRight;
	glm::vec3 m_playerUp;
	glm::vec3 m_playerAimFront;
	glm::vec3 m_playerAimRight;
	glm::vec3 m_playerAimUp;
	float m_aimPitch = 0.0f;
	float m_initialYaw = -90.0f;

	int m_animNum = 2;

private:
	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);

	Texture m_normal{};
	Texture m_metallic{};
	Texture m_roughness{};
	Texture m_ao{};

	AudioComponent* m_takeDamageAc;
	AudioComponent* m_shootAc;
	AudioComponent* m_deathAc;

	glm::vec3 m_shootStartPos = GetPosition() + (glm::vec3(0.0f, 2.5f, 0.0f));
	float m_shootDistance = 100000.0f;
	glm::vec3 m_playerShootHitPoint = glm::vec3(0.0f);

	float m_movementSpeed = 30.5f;
	float m_velocity = 0.0f;

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_playerDualQuatSsBuffer{};

	PlayerState m_playerState = MOVING;

	CombatComponent m_combat;

	int m_sourceAnim = 2;
	int m_destAnim = 2;
	bool m_destAnimSet = true;
	float m_blendSpeed = 10.0f;
	float m_blendFactor = 0.0f;
	bool m_blendAnim = false;
	bool m_resetBlend = false;

	glm::vec3 m_initialPos = glm::vec3(0.0f, 0.0f, 0.0f);

	bool m_playGameStartAudio = true;
	float m_playGameStartAudioTimer = 3.0f;

	AABB* m_aabb;
	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	std::map<std::string, GLint> m_playerModelAttributes =
	{
		{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"JOINTS_0", 4}, {"WEIGHTS_0", 5},
		{"TANGENT", 6}
	};

	SkinnedMeshComponent m_skinnedMesh;
};
