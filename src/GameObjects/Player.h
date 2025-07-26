#pragma once

#include <string>
#include <iostream>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"

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
	       GameManager* gameMgr, float yaw = -90.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		SetInitialPos(pos);
		m_initialYaw = yaw;

		m_model = std::make_shared<GltfModel>();

		std::string modelFilename =
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat.gltf";
		std::string modelTextureFilename =
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_BaseColor.png";

		if (!m_model->LoadModel(modelFilename))
		{
			Logger::Log(1, "%s: loading glTF m_model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
		}

		m_tex = m_model->LoadTexture(modelTextureFilename, false);

		m_normal.LoadTexture(
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Normal.png");
		m_metallic.LoadTexture(
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Metallic.png");
		m_roughness.LoadTexture(
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Roughness.png");
		m_ao.LoadTexture(
			"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_AO.png");

		m_model->UploadIndexBuffer();
		Logger::Log(1, "%s: glTF m_model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());

		size_t playerModelJointDualQuatBufferSize = m_model->GetJointDualQuatsSize() *
			sizeof(glm::mat2x4);
		m_playerDualQuatSsBuffer.Init(playerModelJointDualQuatBufferSize);
		Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n",
		            __FUNCTION__, playerModelJointDualQuatBufferSize);

		m_takeDamageAc = new AudioComponent(this);
		m_deathAc = new AudioComponent(this);
		m_shootAc = new AudioComponent(this);

		ComputeAudioWorldTransform();

		UpdatePlayerVectors();
		SetPlayerAimFront(GetPlayerFront());
		m_playerAimRight = GetPlayerRight();
		SetPlayerAimUp(m_playerUp);
	}

	~Player()
	{
		m_model->Cleanup();
	}

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
	                glm::vec3 camPos) override;

	void Update(float dt);

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

	float GetHealth() const { return m_health; }
	void SetHealth(float newHealth) { m_health = newHealth; }

	void TakeDamage(float damage)
	{
		/*m_takeDamageAc->PlayEvent("event:/PlayerTakeDamage");*/
		SetHealth(GetHealth() - damage);
		if (GetHealth() <= 0.0f)
		{
			OnDeath();
		}
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

	void HasDealtDamage() override
	{
	};

	void HasKilledPlayer() override
	{
	};

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
private:
	CameraMovement m_prevDirection;

	float m_playerYaw;
	glm::vec3 m_playerFront;
	glm::vec3 m_playerRight;
	glm::vec3 m_playerUp;
	glm::vec3 m_playerAimFront;
	glm::vec3 m_playerAimRight;
	glm::vec3 m_playerAimUp;
	float m_aimPitch = 0.0f;
	float m_initialYaw = -90.0f;

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

	float m_movementSpeed = 10.5f;
	float m_velocity = 0.0f;

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_playerDualQuatSsBuffer{};

	PlayerState m_playerState = MOVING;

	Shader* m_aabbShader;
	float m_health = 100.0f;

	int m_animNum = 0;
	int m_sourceAnim = 0;
	int m_destAnim = 0;
	bool m_destAnimSet = false;
	float m_blendSpeed = 5.0f;
	float m_blendFactor = 0.0f;
	bool m_blendAnim = false;
	bool m_resetBlend = false;

	glm::vec3 m_initialPos = glm::vec3(0.0f, 0.0f, 0.0f);

	bool m_playGameStartAudio = true;
	float m_playGameStartAudioTimer = 3.0f;
	float m_playerShootAudioCooldown = 2.0f;

	AABB* m_aabb;
	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
};
