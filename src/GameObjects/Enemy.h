#pragma once
#include <memory>
#include <random>

#include "GameObject.h"
#include "Player.h"
#include "src/Pathfinding/Grid.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "AI/BehaviourTree.h"
#include "AI/Event.h"
#include "AI/Events.h"
#include "AI/ConditionNode.h"
#include "AI/ActionNode.h"

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

class Enemy : public GameObject
{
public:
	Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, Grid* grd, std::string texFilename, int id, EventManager& eventManager, Player& player,
		float yaw = 0.0f);

	~Enemy()
	{
		m_model->cleanup();
	}

	void SetUpModel();

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
		glm::vec3 camPos) override;

	void Update(bool shouldUseEDBT);

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

	float GetHealth() const { return m_health; }
	void SetHealth(float newHealth) { m_health = newHealth; }

	void TakeDamage(float damage);
	void OnDeath();
	void SetIsDead(bool newValue) { m_isDead = newValue; }

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
		return GetPosition() + glm::vec3(0.0f, 4.5f, 0.0f) + (forwardOffset * m_front);
	}

	glm::vec3 GetEnemyShootDir() const { return m_enemyShootDir; }
	glm::vec3 GetEnemyHitPoint() const { return m_enemyHitPoint; }
	bool GetEnemyHasShot() const { return m_enemyHasShot; }
	bool GetEnemyHasHit() const { return m_enemyHasHit; }
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

	void ScoreCoverLocations(Player& player);

	glm::vec3 SelectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints);

	// Function to perform enemy decision-making during an attack using Nash Q-learning
	void EnemyDecision(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void EnemyDecisionPrecomputedQ(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void ResetState();

private:
	// NASH LEARNING START
	const float m_learningRate = 0.05f;
	const float m_discountFactor = 0.95f;
	float m_explorationRate;
	float m_initialExplorationRate = 0.7f;
	float m_minExplorationRate = 0.1f;
	int m_targetQTableSize = 1000000;
	Action m_chosenAction;
	float m_decisionDelayTimer = 0.0f;


	Player& m_player;
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

	// Choose an action based on epsilon-greedy strategy for a specific enemy
	Action ChooseAction(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	// Update Q-value for a given state-action pair for a specific enemy
	void UpdateQValue(const State& currentState, Action action, const State& nextState, float reward,
		int enemyId, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);


	// USING PRECOMPUTED Q-VALUES START

	// Choose an action based on the highest Q-value for a specific enemy
	Action ChooseActionFromTrainedQTable(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	// USING PRECOMPUTED Q-VALUES END

	// NASH LEARNING EMD

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
	Grid* m_grid;

	std::vector<glm::vec3> m_waypointPositions = {
	m_grid->snapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
	m_grid->snapToGrid(glm::vec3(0.0f, 0.0f, 70.0f)),
	m_grid->snapToGrid(glm::vec3(40.0f, 0.0f, 0.0f)),
	m_grid->snapToGrid(glm::vec3(40.0f, 0.0f, 70.0f))
	};

	float m_health = 100.0f;
	bool m_isPlayerDetected;
	bool m_isPlayerVisible;
	bool m_isPlayerInRange;
	bool m_isTakingDamage;
	bool m_hasTakenDamage = false;
	bool m_isDying = false;
	bool m_isDead = false;
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
	Grid::Cover* m_selectedCover = nullptr;
	size_t m_pathIndex = 0;
	size_t m_prevPathIndex = 0;
	std::vector<glm::ivec2> m_prevPath = {};

	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_aabbScale = glm::vec3(3.2f, 3.0f, 3.2f);

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

	float m_accuracy = 60.0f;
	glm::vec3 m_enemyShootPos = glm::vec3(0.0f);
	glm::vec3 m_enemyShootDir = glm::vec3(0.0f);
	glm::vec3 m_enemyHitPoint = glm::vec3(0.0f);
	float m_enemyShootDistance = 100000.0f;
	float m_enemyShootCooldown = 0.0f;
	float m_enemyRayDebugRenderTimer = 0.3f;
	bool m_enemyHasShot = false;
	bool m_enemyHasHit = false;
	bool m_playerIsVisible = false;

	bool m_startingSuppressionFire = true;
	bool m_playNotVisibleAudio = true;

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
};
