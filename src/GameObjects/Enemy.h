#pragma once

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
#include <memory>

static const char* EnemyStateNames[] = {
    "Patrol",
    "Attack",
    "Enemy Shooting",
	"Take Damage",
	"Seeking Cover",
	"Taking Cover",
	"Dying",
	"Dead"
};

class Enemy : public GameObject {
public:
    Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, bool applySkinning, GameManager* gameMgr, Grid* grd, std::string texFilename, int id, EventManager& eventManager, Player& player, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, sdr, applySkinning, gameMgr), grid_(grd), id_(id), eventManager_(eventManager), health_(100), isPlayerDetected_(false),
        isPlayerVisible_(false), isPlayerInRange_(false), isTakingDamage_(false), isDead_(false), isInCover_(false), isSeekingCover_(false), isTakingCover_(false), player(player)
    {
        model = std::make_shared<GltfModel>();

        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly.gltf";

        if (!model->loadModel(modelFilename)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }

        if (!mTex.loadTexture(texFilename, false)) {
            Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
        }
        Logger::log(1, "%s: glTF model texture '%s' successfully loaded\n", __FUNCTION__,
            texFilename.c_str());

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

    void Update();

    void OnEvent(const Event& event);

	int GetID() const { return id_; }

    glm::vec3 getPosition() {
        return position;
    }

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

	void SetYaw(float newYaw) { 
        yaw = newYaw; 
    }

    void Shoot();

	float GetHealth() const { return health_; }
	void SetHealth(float newHealth) { health_ = newHealth; }

	void TakeDamage(float damage) {
		SetHealth(GetHealth() - damage);
        isTakingDamage_ = true;
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
	void SetAnimNum(int newAnimNum) { animNum = newAnimNum; }

    glm::vec3 GetEnemyShootPos() const { return enemyShootPos; }
    glm::vec3 GetEnemyShootDir() const { return enemyShootDir; }
	bool GetEnemyHasShot() const { return enemyHasShot; }
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

private:
	std::vector<glm::vec3> waypointPositions = {
		grid_->snapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
		grid_->snapToGrid(glm::vec3(0.0f, 0.0f, 70.0f)),
		grid_->snapToGrid(glm::vec3(40.0f, 0.0f, 0.0f)),
		grid_->snapToGrid(glm::vec3(40.0f, 0.0f, 70.0f))
	};

	Player& player;

    int id_;
    EventManager& eventManager_;
    BTNodePtr behaviorTree_;

    float health_;
    bool isPlayerDetected_;
    bool isPlayerVisible_;
    bool isPlayerInRange_;
    bool isTakingDamage_;
    bool isDead_;
    bool isDying_ = false;
    bool isInCover_;
    bool isSeekingCover_;
    bool isTakingCover_;
    bool isAttacking_ = false;
    bool isPatrolling_ = false;
    bool provideSuppressionFire_ = false;
    std::vector<glm::ivec2> currentPath_;
	float dt_ = 0.0f;
	Grid::Cover* selectedCover_ = nullptr;
	size_t pathIndex_ = 0;
	size_t prevPathIndex_ = 0;
	std::vector<glm::ivec2> prevPath_ = {};

	glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 aabbScale = glm::vec3(4.5f, 3.0f, 4.5f);

	float health = 100.0f;
	float speed = 6.0f;
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
	int animNum = 0;
	bool takingDamage = false;
	float damageTimer = 0.0f;
	float dyingTimer = 0.0f;
	bool inCover = false;
	float coverTimer = 0.0f;
	bool reachedCover = false;

	float accuracy = 60.0f;
	glm::vec3 enemyShootPos = glm::vec3(0.0f);
	glm::vec3 enemyShootDir = glm::vec3(0.0f);
	float enemyShootDistance = 100000.0f;
	float enemyShootCooldown = 0.0f;
	float enemyRayDebugRenderTimer = 0.3f;
	bool enemyHasShot = false;
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