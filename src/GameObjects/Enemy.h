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

enum NashAction {
	ATTACK,
	ADVANCE,
	RETREAT,
	PATROL
};

struct NashState {
	bool playerDetected;
	bool playerVisible;
	float distanceToPlayer;
	float health;
	bool isSuppressionFire;

	bool operator==(const NashState& other) const {
		return playerDetected == other.playerDetected &&
			playerVisible == other.playerVisible &&
			distanceToPlayer == other.distanceToPlayer &&
			isSuppressionFire == other.isSuppressionFire &&
			health == other.health;
	}
};


// Custom hash function for State and Action pair
struct PairHash {
	std::size_t operator()(const std::pair<NashState, NashAction>& pair) const {
		const NashState& state = pair.first;
		NashAction action = pair.second;
		return ((std::hash<bool>()(state.playerDetected) ^ (std::hash<bool>()(state.playerVisible) << 1)) >> 1) ^
			(std::hash<bool>()(state.isSuppressionFire) << 1) ^ (std::hash<int>()((int)state.health) << 2) ^
			std::hash<int>()(action);
	}
};

class Enemy : public GameObject {
public:
	// NASH LEARNING


private:

	const float learningRate = 0.1f;
	const float discountFactor = 0.9f;
	float explorationRate;
	float initialExplorationRate = 0.5f;
	float minExplorationRate = 0.1f;
	int targetQTableSize = 100000;  

	float DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize) {
		if (currentSize >= targetSize) {
			return minRate;  
		}
		float decayedRate = minRate + (initialRate - minRate) * (1.0f - static_cast<float>(currentSize) / targetSize);
		return decayedRate;
	}

	Player& player;

	// NASH LEARNING

public:
	Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, bool applySkinning, GameManager* gameMgr, Grid* grd, std::string texFilename, int id, EventManager& eventManager, Player& player, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, sdr, applySkinning, gameMgr), grid_(grd), id_(id), eventManager_(eventManager), health_(100.0f), isPlayerDetected_(false),
		isPlayerVisible_(false), isPlayerInRange_(false), isTakingDamage_(false), isDead_(false), isInCover_(false), isSeekingCover_(false), isTakingCover_(false), player(player), initialPosition(pos)
    {
		isEnemy = true;

		id_ = id;

        model = std::make_shared<GltfModel>();

        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly.gltf";

        if (!model->loadModel(modelFilename, true)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }

        if (!mTex.loadTexture(texFilename, false)) {
            Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
        }
        Logger::log(1, "%s: glTF model texture '%s' successfully loaded\n", __FUNCTION__, texFilename.c_str());

        SetUpModel();

        ComputeAudioWorldTransform();

        UpdateEnemyCameraVectors();
        UpdateEnemyVectors(); 

        currentWaypoint = waypointPositions[std::rand() % waypointPositions.size()];
        takeDamageAC = new AudioComponent(this);
		deathAC = new AudioComponent(this);
		shootAC = new AudioComponent(this);

        BuildBehaviorTree();

        eventManager_.Subscribe<PlayerDetectedEvent>([this](const Event& e) { OnEvent(e); });
        eventManager_.Subscribe<NPCDamagedEvent>([this](const Event& e) { OnEvent(e); });
		eventManager_.Subscribe<NPCDiedEvent>([this](const Event& e) { OnEvent(e); });
    }

    ~Enemy() 
    {
        model->cleanup();
    }

    void SetUpModel();

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void Update(bool shouldUseEDBT);

    void OnEvent(const Event& event);

	int GetID() const { return id_; }

    glm::vec3 getPosition() {
        return position;
    }

	glm::vec3 getInitialPosition() const { return initialPosition; }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
        updateAABB();
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
    }

	void ComputeAudioWorldTransform() override;

    void UpdateEnemyCameraVectors();

    void UpdateEnemyVectors();

    void EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    void moveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards);

    void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
    void SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards);

	void SetYaw(float newYaw) { 
        yaw = newYaw; 
    }

    void Shoot();

	float GetHealth() const { return health_; }
	void SetHealth(float newHealth) { health_ = newHealth; }

	void TakeDamage(float damage) {
		SetHealth(GetHealth() - damage);
		if (health_ <= 0)
		{
			OnDeath();
			return;
		}
        isTakingDamage_ = true;
		hasTakenDamage_ = true;
    }

    void OnDeath();

    void updateAABB() {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f), glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), aabbScale);
        aabb->update(modelMatrix);
    };

	void SetAABBShader(Shader* aabbShdr) { aabbShader = aabbShdr; }
    void SetUpAABB();
    AABB* GetAABB() const { return aabb; }
	void setAABBColor(glm::vec3 color) { aabbColor = color; }

    int GetAnimNum() const { return animNum; }
	int GetSourceAnimNum() const { return sourceAnim; }
	int GetDestAnimNum() const { return destAnim; }
	void SetAnimNum(int newAnimNum) { animNum = newAnimNum; }
	void SetSourceAnimNum(int newSrcAnim) { sourceAnim = newSrcAnim; }
	void SetDestAnimNum(int newDestAnim) {
		destAnim = newDestAnim;
		destAnimSet = true;
	}

    glm::vec3 GetEnemyShootPos() const { return enemyShootPos; }
    glm::vec3 GetEnemyShootDir() const { return enemyShootDir; }
	glm::vec3 GetEnemyHitPoint() const { return enemyHitPoint; }
	bool GetEnemyHasShot() const { return enemyHasShot; }
	bool GetEnemyHasHit() const { return enemyHasHit; }
	float GetEnemyShootDistance() const { return enemyShootDistance; }
	float GetEnemyDebugRayRenderTimer() const { return enemyRayDebugRenderTimer; }

    void SetDeltaTime(float newDt) { dt_ = newDt; }

	std::string GetEDBTState() const { return EDBTState; }

    glm::vec3 GetEnemyFront() const { return EnemyFront; }

    void OnHit() override;

    void OnMiss() override {
        aabbColor = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    
    void ScoreCoverLocations(Player& player);

    glm::vec3 selectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints) {

        if (isDestroyed) return glm::vec3(0.0f);

        std::vector<glm::vec3> availableWaypoints;
        for (const auto& wp : allWaypoints) {
            if (wp != currentWaypoint) {
                availableWaypoints.push_back(wp);
            }
        }

        // Select a random way point from the available way points
        int randomIndex = std::rand() % availableWaypoints.size();
        return availableWaypoints[randomIndex];
    }
	Grid* grid_;

    // NASH LEARNING

	const float BUCKET_SIZE = 10.0f;  
	const float TOLERANCE = 10.0f;     

	int getDistanceBucket(float distance) {
		return static_cast<int>(distance / BUCKET_SIZE);
	}

    float CalculateReward(const NashState& state, NashAction action, int enemyId, const std::vector<NashAction>& squadActions) {
		float reward = 0.0f;

		if (action == ATTACK) {
			reward += (state.playerVisible && state.playerDetected) ? 20.0f : -10.0f;
			if (hasDealtDamage_)
			{
				reward += 8.0f;
				hasDealtDamage_ = false;
				
				if (hasKilledPlayer_)
				{
					reward += 25.0f;
					hasKilledPlayer_ = false;
				}
			}

			if (state.health <= 20)
			{
				reward -= 5.0f;
			}
		}
		else if (action == ADVANCE) {
			reward += (state.distanceToPlayer > 15.0f && state.playerDetected || (state.playerDetected && !state.playerVisible)) ? 12.0f : -3.0f;

			if (state.distanceToPlayer < 10.0f)
			{
				reward -= 5.0f;
			}
		}
		else if (action == RETREAT) {
			reward += (state.health <= 40) ? 10.0f : -5.0f;

			if (state.health <= 20 && state.distanceToPlayer > 20.0f)
			{
				reward += 5.0f;
			}
		}
		else if (action == PATROL) {
			reward += (!state.playerDetected) ? 5.0f : -10.0f;
		}

		// Additional reward for coordinated behavior
		int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
		if (action == ATTACK && numAttacking > 1 && (state.playerVisible && state.playerDetected)) {
			reward += 10.0f; 
		}

		if (hasTakenDamage_)
		{
			reward -= 5.0f;
			hasTakenDamage_ = false;
		}

		if (hasDied_)
		{
			reward -= 20.0f;
			hasDied_ = false;

			if (numDeadAllies = 3)
			{
				reward -= 30.0f;
				numDeadAllies = 0;
			}
		}

		if (allyHasDied)
		{
			reward -= 10.0f;
			allyHasDied = false;

		}

		hasDealtDamage_ = false;
		hasKilledPlayer_ = false;

		return reward;
	}

	float GetMaxQValue(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		float maxQ = -std::numeric_limits<float>::infinity();
		int targetBucket = getDistanceBucket(state.distanceToPlayer);

		for (auto action : { ATTACK, ADVANCE, RETREAT, PATROL }) {
		    for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset) {
		        int bucket = targetBucket + bucketOffset;
		        NashState modifiedState = state;
		        modifiedState.distanceToPlayer = bucket * BUCKET_SIZE; // Discretized distance
		        
		        auto it = qTable[enemyId].find({ modifiedState, action });
		        if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <= TOLERANCE) {
		            maxQ = std::max(maxQ, it->second);
		        }
		    }
		}
		return (maxQ == -std::numeric_limits<float>::infinity()) ? 0.0f : maxQ;
	}

	// Choose an action based on epsilon-greedy strategy for a specific enemy
	NashAction ChooseAction(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_real_distribution<> dis(0.0, 1.0);

		int currentQTableSize = qTable[enemyId].size();
		explorationRate = DecayExplorationRate(initialExplorationRate, minExplorationRate, currentQTableSize, targetQTableSize);

		if (dis(gen) < explorationRate) {
			// Exploration: choose a random action
			std::uniform_int_distribution<> actionDist(0, 3);
			return static_cast<NashAction>(actionDist(gen));
		}
		else {
			// Exploitation: choose the action with the highest Q-value
			float maxQ = -std::numeric_limits<float>::infinity();
			NashAction bestAction = PATROL;
			for (auto action : { ATTACK, ADVANCE, RETREAT, PATROL }) {
				float qValue = qTable[enemyId][{state, action}];
				if (qValue > maxQ) {
					maxQ = qValue;
					bestAction = action;
				}
			}
			return bestAction;
		}
	}

	// Update Q-value for a given state-action pair for a specific enemy
	void UpdateQValue(const NashState& currentState, NashAction action, const NashState& nextState, float reward,
		int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		float currentQ = qTable[enemyId][{currentState, action}];
		float maxFutureQ = GetMaxQValue(nextState, enemyId, qTable);
		float updatedQ = (1 - learningRate) * currentQ + learningRate * (reward + discountFactor * maxFutureQ);
		qTable[enemyId][{currentState, action}] = updatedQ;
	}

	// Function to perform enemy decision-making during an attack using Nash Q-learning
	void EnemyDecision(NashState& currentState, int enemyId, std::vector<NashAction>& squadActions,
		float deltaTime, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		
		if (enemyHasShot)
		{
			enemyRayDebugRenderTimer -= dt_;
			enemyShootCooldown -= dt_;
		}
		if (enemyShootCooldown <= 0.0f)
		{
			enemyHasShot = false;
		}

		if (damageTimer > 0.0f)
		{
			damageTimer -= deltaTime;
			return;
		}
		
		if (dyingTimer > 0.0f && isDying_)
		{
			dyingTimer -= deltaTime;
			return;
		}
		else if (dyingTimer <= 0.0f && isDying_)
		{
			isDying_ = false;
			isDead_ = true;
			isDestroyed = true;
			dyingTimer = 100000.0f;
			return;
		}

		NashAction chosenAction = ChooseAction(currentState, enemyId, qTable);

		// Simulate taking action and getting a reward
		NashState nextState = currentState;
		int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
		bool isSuppressionFire = numAttacking > 0;
		float playerDistance = glm::distance(getPosition(), player.getPosition());

		if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
		{
			DetectPlayer();
		}

		if (chosenAction == ADVANCE) {
			EDBTState = "ADVANCE";
			currentPath_ = grid_->findPath(
				glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
				glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
				grid_->GetGrid(),
				enemyId
			);

			VacatePreviousCell();

			moveEnemy(currentPath_, deltaTime, 1.0f, false);

			nextState.playerDetected = IsPlayerDetected();
			nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			nextState.playerVisible = IsPlayerVisible();
			nextState.health = GetHealth();
			nextState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == RETREAT) {
			EDBTState = "RETREAT";

			if (!selectedCover_ || grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
			{
				ScoreCoverLocations(player);
			}

			glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
			glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);


			currentPath_ = grid_->findPath(
				glm::ivec2(snappedCurrentPos.x / grid_->GetCellSize(), snappedCurrentPos.z / grid_->GetCellSize()),
				glm::ivec2(snappedCoverPos.x / grid_->GetCellSize(), snappedCoverPos.z / grid_->GetCellSize()),
				grid_->GetGrid(),
				id_
			);

			VacatePreviousCell();

			moveEnemy(currentPath_, dt_, 1.0f, false);

			nextState.playerDetected = IsPlayerDetected();
			nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			nextState.playerVisible = IsPlayerVisible();
			nextState.health = GetHealth();
			nextState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == ATTACK) {
			EDBTState = "ATTACK";

			if (enemyShootCooldown > 0.0f)
			{
				return;
			}

			Shoot();

			nextState.playerDetected = IsPlayerDetected();
			nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			nextState.playerVisible = IsPlayerVisible();
			nextState.health = GetHealth();
			nextState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == PATROL) {
			EDBTState = "PATROL";

			if (reachedDestination == false)
			{
				currentPath_ = grid_->findPath(
					glm::ivec2((int)(getPosition().x / grid_->GetCellSize()), (int)(getPosition().z / grid_->GetCellSize())),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);

				VacatePreviousCell();

				moveEnemy(currentPath_, dt_, 1.0f, false);
			}
			else
			{
				currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);

				VacatePreviousCell();

				reachedDestination = false;

				moveEnemy(currentPath_, dt_, 1.0f, false);
			}

			nextState.playerDetected = IsPlayerDetected();
			nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			nextState.playerVisible = IsPlayerVisible();
			nextState.health = GetHealth();
			nextState.isSuppressionFire = isSuppressionFire;
		}

		float reward = CalculateReward(currentState, chosenAction, enemyId, squadActions);

		// Update Q-value
		UpdateQValue(currentState, chosenAction, nextState, reward, enemyId, qTable);

		// Update current state
		currentState = nextState;
		squadActions[enemyId] = chosenAction;

		// Print chosen action
		std::cout << "Enemy " << enemyId << " Chosen Action: " << chosenAction << " with reward: " << reward << std::endl;
	}

	// USING PRECOMPUTED Q-VALUES

	// Choose an action based on the highest Q-value for a specific enemy
	NashAction ChooseActionFromTrainedQTable(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		float maxQ = -std::numeric_limits<float>::infinity();
		NashAction bestAction = PATROL;
		int targetBucket = getDistanceBucket(state.distanceToPlayer);

		for (auto action : { ATTACK, ADVANCE, RETREAT, PATROL }) {
			for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset) {
				int bucket = targetBucket + bucketOffset;
				NashState modifiedState = state;
				modifiedState.distanceToPlayer = bucket * BUCKET_SIZE;  // Use discretized distance

				auto it = qTable[enemyId].find({ modifiedState, action });
				// Check if entry exists and is within tolerance range
				if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <= TOLERANCE) {
					if (it->second > maxQ) {
						maxQ = it->second;
						bestAction = action;
					}
				}
			}
		}

		return bestAction;
	}

	void EnemyDecisionPrecomputedQ(NashState& currentState, int enemyId, std::vector<NashAction>& squadActions,
		float deltaTime, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable) {
		
		if (enemyHasShot)
		{
			enemyRayDebugRenderTimer -= dt_;
			enemyShootCooldown -= dt_;
		}
		if (enemyShootCooldown <= 0.0f)
		{
			enemyHasShot = false;
		}

		if (damageTimer > 0.0f)
		{
			damageTimer -= deltaTime;
			return;
		}
		
		if (dyingTimer > 0.0f && isDying_)
		{
			dyingTimer -= deltaTime;
			return;
		}
		else if (dyingTimer <= 0.0f && isDying_)
		{
			isDying_ = false;
			isDead_ = true;
			isDestroyed = true;
			dyingTimer = 100000.0f;
			return;
		}

		NashAction chosenAction = ChooseActionFromTrainedQTable(currentState, enemyId, qTable);
		
		int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
		bool isSuppressionFire = numAttacking > 0;
		float playerDistance = glm::distance(getPosition(), player.getPosition());
		if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
		{
			DetectPlayer();
		}

		if (chosenAction == ADVANCE) {
			EDBTState = "ADVANCE";
			currentPath_ = grid_->findPath(
				glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
				glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
				grid_->GetGrid(),
				enemyId
			);

			VacatePreviousCell();

			moveEnemy(currentPath_, deltaTime, 1.0f, false);

			currentState.playerDetected = IsPlayerDetected();
			currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			currentState.playerVisible = IsPlayerVisible();
			currentState.health = GetHealth();
			currentState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == RETREAT) 
		{
			EDBTState = "RETREAT";

			if (!selectedCover_ || grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
			{
				ScoreCoverLocations(player);
			}

			glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
			glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);


			currentPath_ = grid_->findPath(
				glm::ivec2(snappedCurrentPos.x / grid_->GetCellSize(), snappedCurrentPos.z / grid_->GetCellSize()),
				glm::ivec2(snappedCoverPos.x / grid_->GetCellSize(), snappedCoverPos.z / grid_->GetCellSize()),
				grid_->GetGrid(),
				id_
			);

			VacatePreviousCell();

			moveEnemy(currentPath_, dt_, 1.0f, false);
			currentState.playerDetected = IsPlayerDetected();
			currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			currentState.playerVisible = IsPlayerVisible();
			currentState.health = GetHealth();
			currentState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == ATTACK) 
		{
			EDBTState = "ATTACK";

			if (enemyShootCooldown > 0.0f)
			{
				return;
			}

			Shoot();

			currentState.playerDetected = IsPlayerDetected();
			currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			currentState.playerVisible = IsPlayerVisible();
			currentState.health = GetHealth();
			currentState.isSuppressionFire = isSuppressionFire;
		}
		else if (chosenAction == PATROL) {
			EDBTState = "PATROL";

			if (reachedDestination == false)
			{
				currentPath_ = grid_->findPath(
					glm::ivec2((int)(getPosition().x / grid_->GetCellSize()), (int)(getPosition().z / grid_->GetCellSize())),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);

				VacatePreviousCell();

				moveEnemy(currentPath_, dt_, 1.0f, false);
			}
			else
			{
				currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);

				VacatePreviousCell();

				reachedDestination = false;

				moveEnemy(currentPath_, dt_, 1.0f, false);
			}


			currentState.playerDetected = IsPlayerDetected();
			currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
			currentState.playerVisible = IsPlayerVisible();
			currentState.health = GetHealth();
			currentState.isSuppressionFire = isSuppressionFire;
		}

		squadActions[enemyId] = chosenAction;

		// Print chosen action
		std::cout << "Enemy " << enemyId << " Chosen Action: " << chosenAction << std::endl;
	}


	// USING PRECOMPUTED Q-VALUES

	// NASH LEARNING

	bool isDead_ = false;

	void HasDealtDamage() override { hasDealtDamage_ = true; }
	void HasKilledPlayer() override { hasKilledPlayer_ = true; }

private:
	std::vector<glm::vec3> waypointPositions = {
		grid_->snapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
		grid_->snapToGrid(glm::vec3(0.0f, 0.0f, 70.0f)),
		grid_->snapToGrid(glm::vec3(40.0f, 0.0f, 0.0f)),
		grid_->snapToGrid(glm::vec3(40.0f, 0.0f, 70.0f))
	};

	glm::vec3 initialPosition = glm::vec3(0.0f, 0.0f, 0.0f);

    int id_;
    EventManager& eventManager_;
    BTNodePtr behaviorTree_;

    float health_ = 100.0f;
    bool isPlayerDetected_;
    bool isPlayerVisible_;
    bool isPlayerInRange_;
    bool isTakingDamage_;
	bool hasTakenDamage_ = false;
    bool isDying_ = false;
	bool hasDied_ = false;
    bool isInCover_;
    bool isSeekingCover_;
    bool isTakingCover_;
    bool isAttacking_ = false;
	bool hasDealtDamage_ = false;
	bool hasKilledPlayer_ = false;
    bool isPatrolling_ = false;
    bool provideSuppressionFire_ = false;
	bool allyHasDied = false;

	int numDeadAllies = 0;

    std::vector<glm::ivec2> currentPath_;
	float dt_ = 0.0f;
	Grid::Cover* selectedCover_ = nullptr;
	size_t pathIndex_ = 0;
	size_t prevPathIndex_ = 0;
	std::vector<glm::ivec2> prevPath_ = {};

	glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 aabbScale = glm::vec3(4.5f, 3.0f, 4.5f);

	float health = 100.0f;
	float speed = 7.5f;
	float EnemyCameraYaw;
	float EnemyCameraPitch = 10.0f;
	glm::vec3 EnemyFront;
	glm::vec3 EnemyRight;
	glm::vec3 EnemyUp;
	glm::vec3 Front;
	glm::vec3 Right;
	glm::vec3 Up;
	bool reachedDestination = false;
	bool reachedPlayer = false;
	glm::vec3 currentWaypoint;
	std::string EDBTState = "Patrolling";

	bool uploadVertexBuffer = true;
	ShaderStorageBuffer mEnemyDualQuatSSBuffer{};

	AABB* aabb;
	Shader* aabbShader;

	AudioComponent* takeDamageAC;
	AudioComponent* shootAC;
	AudioComponent* deathAC;
	
	int animNum = 1;
	int sourceAnim = 1;
	int destAnim = 1;
	bool destAnimSet = false;
	float blendSpeed = 5.0f;
	float blendFactor = 0.0f;
	bool blendAnim = false;
	bool resetBlend = false;

	bool takingDamage = false;
	float damageTimer = 0.0f;
	float dyingTimer = 0.0f;
	bool inCover = false;
	float coverTimer = 0.0f;
	bool reachedCover = false;

	float accuracy = 60.0f;
	glm::vec3 enemyShootPos = glm::vec3(0.0f);
	glm::vec3 enemyShootDir = glm::vec3(0.0f);
	glm::vec3 enemyHitPoint = glm::vec3(0.0f);
	float enemyShootDistance = 100000.0f;
	float enemyShootCooldown = 0.0f;
	float enemyRayDebugRenderTimer = 0.3f;
	bool enemyHasShot = false;
	bool enemyHasHit = false;
	bool playerIsVisible = false;

	void VacatePreviousCell();

    void BuildBehaviorTree();

    void DetectPlayer();

    bool IsDead();
    bool IsHealthZeroOrBelow();
    bool IsTakingDamage();
    bool IsPlayerDetected();
    bool IsPlayerVisible();
    bool IsCooldownComplete();
    bool IsHealthBelowThreshold();
    bool IsPlayerInRange();
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