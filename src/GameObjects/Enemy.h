#pragma once

#include "GameObject.h"
#include "Player.h"
#include "src/Pathfinding/Grid.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"

enum EnemyState {
    PATROL,
    ATTACK,
	TAKE_DAMAGE,
    DYING,
    DEAD
};

static const char* EnemyStateNames[] = {
    "Patrol",
    "Attack"
};

class Enemy : public GameObject {
public:

    Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, bool applySkinning, GameManager* gameMgr, Grid* grd, float yaw = 0.0f)
		: GameObject(pos, scale, yaw, sdr, applySkinning, gameMgr), grid(grd)
    {
        model = std::make_shared<GltfModel>();

        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman.gltf";
        std::string modelTextureFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman2.png";

        if (!model->loadModel(renderData, modelFilename, modelTextureFilename)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }

        if (uploadVertexBuffer)
        {
            model->uploadVertexBuffers();
            //aabb.calculateAABB(model->getVertices());
            //aabb.owner = this;
            //updateAABB();
            ////class GameManager* gameMgr = GetGameManager();
            //mGameManager->GetPhysicsWorld()->addCollider(GetAABB());
            //mGameManager->GetPhysicsWorld()->addEnemyCollider(GetAABB());
            SetUpAABB();
            uploadVertexBuffer = false;
        }

        model->uploadIndexBuffer();
        Logger::log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());


        size_t enemyModelJointDualQuatBufferSize = model->getJointDualQuatsSize() *
            sizeof(glm::mat2x4);
        mEnemyDualQuatSSBuffer.init(enemyModelJointDualQuatBufferSize);
        Logger::log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, enemyModelJointDualQuatBufferSize);

        ComputeAudioWorldTransform();

        UpdateEnemyCameraVectors();
        UpdateEnemyVectors(); 

        currentWaypoint = waypointPositions[std::rand() % waypointPositions.size()];
        takeDamageAC = new AudioComponent(this);
    }

    ~Enemy() 
    {
        model->cleanup();
    }

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void Update(float dt, Player& player, float blendFactor, bool playAnimBackwards);

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

    EnemyState GetEnemyState() const { return state; }

    void SetEnemyState(EnemyState newState) { state = newState; }

    void moveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards);

    void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);

	void SetYaw(float newYaw) { 
        yaw = newYaw; 
    }

	float GetHealth() const { return health; }
	void SetHealth(float newHealth) { health = newHealth; }

	void TakeDamage(float damage) {
		SetHealth(GetHealth() - damage);
		if (GetHealth() <= 0.0f) {
			OnDeath();
		}
	}

    void OnDeath();

    void updateAABB() {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f), glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scale);
        aabb->mModelMatrix = modelMatrix;
        aabb->update(modelMatrix);
    };

    void SetUpAABB();

    AABB* GetAABB() const { return aabb; }
    void renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* shader);

	void setAABBColor(glm::vec3 color) { aabbColor = color; }

    int GetAnimNum() const { return animNum; }
	void SetAnimNum(int newAnimNum) { animNum = newAnimNum; }

    void OnHit() override;
    void OnMiss() override {
        aabbColor = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    
    glm::vec3 selectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints) {

        std::vector<glm::vec3> availableWaypoints;
        for (const auto& wp : allWaypoints) {
            if (wp != currentWaypoint) {
                availableWaypoints.push_back(wp);
            }
        }

        // Select a random waypoint from the available waypoints
        int randomIndex = std::rand() % availableWaypoints.size();
        return availableWaypoints[randomIndex];
    }

	glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	float health = 100.0f;
    EnemyState state = PATROL;
    float EnemyCameraYaw;
    float EnemyCameraPitch;
    glm::vec3 EnemyFront;
    glm::vec3 EnemyRight;
    glm::vec3 EnemyUp;
    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 Up;
    bool reachedDestination = false;
    bool reachedPlayer = false;
    glm::vec3 currentWaypoint;

    bool uploadVertexBuffer = true;
    ShaderStorageBuffer mEnemyDualQuatSSBuffer{};

    AABB* aabb;
    Shader* aabbShader;

    AudioComponent* takeDamageAC;
	int animNum = 0;
    bool takingDamage = false;
	float damageTimer = 0.0f;
    bool isDying = false;
	float dyingTimer = 0.0f;

	Grid* grid;

    std::vector<glm::vec3> waypointPositions = {
            grid->snapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
            grid->snapToGrid(glm::vec3(0.0f, 0.0f, 90.0f)),
            grid->snapToGrid(glm::vec3(30.0f, 0.0f, 0.0f)),
            grid->snapToGrid(glm::vec3(30.0f, 0.0f, 90.0f))
    };
};