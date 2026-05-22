#pragma once
#include <memory>
#include <random>

#include "GameObject.h"
#include "Player.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "Components/CombatComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "AI/BehaviourTree.h"
#include "AI/EnemyConfig.h"
#include "AI/Event.h"
#include "AI/Events.h"
#include "AI/ConditionNode.h"
#include "AI/ActionNode.h"
#include "Logger.h"

#ifdef NPC_RL_QLEARNING
enum Action
{
	PATROL,
	RETREAT,
	ADVANCE,
	ATTACK
};

struct State
{
	bool playerDetected;
	bool playerVisible;
	float distanceToPlayer;
	float health;
	bool isSuppressionFire;

	bool operator==(const State& other) const
	{
		return playerDetected == other.playerDetected &&
			playerVisible == other.playerVisible &&
			distanceToPlayer == other.distanceToPlayer &&
			isSuppressionFire == other.isSuppressionFire &&
			health == other.health;
	}
};


// Custom hash function for State and Action pair
struct PairHash
{
	std::size_t operator()(const std::pair<State, Action>& pair) const
	{
		const State& state = pair.first;
		Action action = pair.second;
		return ((std::hash<bool>()(state.playerDetected) ^ (std::hash<bool>()(state.playerVisible) << 1)) >> 1) ^
			(std::hash<bool>()(state.isSuppressionFire) << 1) ^ (std::hash<int>()(static_cast<int>(state.health)) << 2)
			^
			std::hash<int>()(action);
	}
};
#endif // NPC_RL_QLEARNING

class Enemy : public GameObject
{
public:
	Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, std::string texFilename, int id, EventManager& eventManager, Player& player,
		const EnemyConfig& config, float yaw = 0.0f);

	~Enemy()
	{
		m_model->cleanup();
	}

	void SetUpModel();

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

	// --- Rendering mesh data (GL VAOs/VBOs, stays on Enemy) ---

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

	void Update(bool shouldUseEDBT, bool isPaused, bool isTimeScaled);

	void OnEvent(const Event& event);

	int GetID() const { return m_id; }

	AudioComponent* GetAudioComponent() const { return m_takeDamageAc; }

	glm::vec3 GetPosition()
	{
		return m_position;
	}

	glm::vec3 GetInitialPosition() const { return m_initialPosition; }

	void SetPosition(glm::vec3 newPos);

	void ComputeAudioWorldTransform() override;

	void UpdateEnemyCameraVectors();

	void UpdateEnemyVectors();

	void EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

	void MoveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards);

	void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
	void SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards);

	void SetYaw(float newYaw) { m_yaw = newYaw; }

	void Shoot();

	float GetHealth() const { return m_combat.health; }
	void SetHealth(float newHealth) { m_combat.health = newHealth; }

	void TakeDamage(float damage);
	void OnDeath();
	void SetIsDead(bool newValue) { m_combat.isDead = newValue; }

	void SetAABBShader(Shader* aabbShdr) { m_aabbShader = aabbShdr; }
	void SetUpAABB();
	AABB* GetAABB() const { return m_aabb; }
	void SetAABBColor(glm::vec3 color) { m_aabbColor = color; }
	void UpdateAABB();

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

	glm::vec3 GetEnemyShootPos(float forwardOffset)
	{
		UpdateEnemyVectors();
		return GetPosition() + glm::vec3(0.0f, 4.5f, 0.0f) + (forwardOffset * m_front);
	}

	glm::vec3 GetEnemyShootDir() const { return m_enemyShootDir; }
	glm::vec3 GetEnemyHitPoint() const { return m_enemyHitPoint; }
	bool GetEnemyHasShot() const { return m_combat.hasShot; }
	bool GetEnemyHasHit() const { return m_combat.hasHit; }
	float GetEnemyShootDistance() const { return m_enemyShootDistance; }
	float GetEnemyDebugRayRenderTimer() const { return m_enemyRayDebugRenderTimer; }

	void SetDeltaTime(float newDt) { m_dt = newDt; }

	std::string GetEDBTState() const { return m_state; }

	glm::vec3 GetEnemyFront() const { return m_enemyFront; }

	void Speak(const std::string& clipName, float priority, float cooldown);

	void OnHit() override;

	void OnMiss() override
	{
		m_aabbColor = glm::vec3(1.0f, 1.0f, 1.0f);
	}

	bool IsDead();
	const EnemyConfig& GetConfig() const { return m_config; }

	void ScoreCoverLocations(Player& player);

	glm::vec3 SelectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints);

#ifdef NPC_RL_QLEARNING
	void EnemyDecision(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void EnemyDecisionPrecomputedQ(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);
#endif // NPC_RL_QLEARNING

	void ResetState();

private:
	Player& m_player;
	float m_decisionDelayTimer = 0.0f;

#ifdef NPC_RL_QLEARNING
	const float m_learningRate = 0.05f;
	const float m_discountFactor = 0.95f;
	float m_explorationRate;
	float m_initialExplorationRate = 0.7f;
	float m_minExplorationRate = 0.1f;
	int m_targetQTableSize = 1000000;
	Action m_chosenAction;
	const float BUCKET_SIZE = 10.0f;
	const float TOLERANCE = 10.0f;

	float DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize);

	int GetDistanceBucket(float distance)
	{
		return static_cast<int>(distance / BUCKET_SIZE);
	}

	float CalculateReward(const State& state, Action action, int enemyId, const std::vector<Action>& squadActions);

	float GetMaxQValue(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	Action ChooseAction(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void UpdateQValue(const State& currentState, Action action, const State& nextState, float reward,
		int enemyId, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	Action ChooseActionFromTrainedQTable(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);
#endif // NPC_RL_QLEARNING

	void HasDealtDamage() override;
	void HasKilledPlayer() override;

	Texture m_normal{};
	Texture m_metallic{};
	Texture m_roughness{};
	Texture m_ao{};
	Texture m_emissive{};

	glm::vec3 m_initialPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	int m_id;
	EventManager& m_eventManager;
	BTNodePtr m_behaviorTree;

	std::vector<glm::vec3> m_waypointPositions = {};

	CombatComponent m_combat;
	bool m_isPlayerDetected;
	bool m_isPlayerVisible;
	bool m_isPlayerInRange;
	bool m_isTakingDamage;
	bool m_hasTakenDamage = false;
	bool m_isDying = false;
	bool m_hasDied = false;
	bool m_isInCover;
	bool m_isSeekingCover;
	bool m_isTakingCover;
	bool m_isAttacking = false;
	bool m_hasDealtDamage = false;
	bool m_hasKilledPlayer = false;
	bool m_isPatrolling = false;
	bool m_provideSuppressionFire = false;
	bool m_allyHasDied = false;

	int m_numDeadAllies = 0;

	std::vector<glm::ivec2> m_currentPath;
	float m_dt = 0.0f;
	size_t m_pathIndex = 0;
	size_t m_prevPathIndex = 0;
	std::vector<glm::ivec2> m_prevPath = {};

	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_aabbScale = glm::vec3(3.8f, 3.3f, 3.5f);

	float m_speed = 7.5f;
	float m_enemyCameraYaw;
	float m_enemyCameraPitch = 10.0f;
	glm::vec3 m_enemyFront;
	glm::vec3 m_enemyRight;
	glm::vec3 m_enemyUp;
	glm::vec3 m_front;
	glm::vec3 m_right;
	glm::vec3 m_up;
	bool m_reachedDestination = false;
	bool m_reachedPlayer = false;
	glm::vec3 m_currentWaypoint;
	std::string m_state = "Patrolling";

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_enemyDualQuatSsBuffer{};

	AABB* m_aabb;
	Shader* m_aabbShader;

	std::vector<glm::vec3> verts;

	AudioComponent* m_takeDamageAc;
	AudioComponent* m_shootAc;
	AudioComponent* m_deathAc;

	int m_animNum = 1;
	int m_sourceAnim = 1;
	int m_destAnim = 1;
	bool m_destAnimSet = false;
	float m_blendSpeed = 5.0f;
	float m_blendFactor = 0.0f;
	bool m_blendAnim = false;
	bool m_resetBlend = false;

	bool m_takingDamage = false;
	float m_damageTimer = 0.0f;
	float m_dyingTimer = 0.0f;
	float m_coverTimer = 0.0f;
	bool m_reachedCover = false;
	float m_shootAudioCooldown = 0.0f;

	glm::vec3 m_enemyShootPos = glm::vec3(0.0f);
	glm::vec3 m_enemyShootDir = glm::vec3(0.0f);
	glm::vec3 m_enemyHitPoint = glm::vec3(0.0f);
	float m_enemyShootDistance = 100000.0f;
	float m_enemyRayDebugRenderTimer = 0.1f;
	bool m_playerIsVisible = false;

	bool m_startingSuppressionFire = true;
	bool m_playNotVisibleAudio = true;

	EnemyConfig m_config;

	void VacatePreviousCell();

	void BuildBehaviorTree();

	void DetectPlayer();

	bool IsHealthZeroOrBelow();
	bool IsTakingDamage();
	bool IsPlayerDetected();
	bool IsPlayerVisible();
	bool IsCooldownComplete();
	bool IsHealthBelowThreshold();
	bool IsPlayerInRange();
	bool IsTakingCover();
	bool IsInCover();
	bool IsAttacking();
	bool IsPatrolling();
	bool ShouldProvideSuppressionFire();

	NodeStatus EnterDyingState();
	NodeStatus EnterTakingDamageState();
	NodeStatus AttackShoot();
	NodeStatus AttackChasePlayer();
	NodeStatus SeekCover();
	NodeStatus TakeCover();
	NodeStatus EnterInCoverState();
	NodeStatus Patrol();
	NodeStatus InCoverAction();
	NodeStatus Die();

	std::map<std::string, GLint> m_enemyModelAttributes =
	{
		{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"JOINTS_0", 4}, {"WEIGHTS_0", 5},
		{"TANGENT", 6}
	};

	SkinnedMeshComponent m_skinnedMesh;
};
