#pragma once

#include "GameObject.h"
#include "Player.h"
#include "src/Pathfinding/Grid.h"

enum EnemyState {
    PATROL,
    ATTACK
};

static const char* EnemyStateNames[] = {
    "Patrol",
    "Attack"
};


static glm::vec3 snapToGrid(const glm::vec3& position);

static std::vector<glm::vec3> waypointPositions = {
            snapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
            snapToGrid(glm::vec3(0.0f, 0.0f, 90.0f)),
            snapToGrid(glm::vec3(30.0f, 0.0f, 0.0f)),
            snapToGrid(glm::vec3(30.0f, 0.0f, 90.0f))
};
static glm::vec3 selectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints) {

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

class Enemy : public GameObject {
public:

    Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, float yaw = 0.0f)
        : GameObject(pos, scale, sdr), Yaw(yaw)
    {
        model = std::make_shared<GltfModel>();

        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman.gltf";
        std::string modelTextureFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman2.png";

        if (!model->loadModel(renderData, modelFilename, modelTextureFilename)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }

        model->uploadIndexBuffer();
        Logger::log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());

        UpdateEnemyCameraVectors();
        UpdateEnemyVectors(); 

        currentWaypoint = waypointPositions[std::rand() % waypointPositions.size()];
    }

    ~Enemy() 
    {
        model->cleanup();
    }

    void drawObject() const override;

    void Update(float dt, Player& player);

    glm::vec3 getPosition() {
        return position;
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }

    void UpdateEnemyCameraVectors();

    void UpdateEnemyVectors();

    void EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    EnemyState GetEnemyState() const { return state; }

    void SetEnemyState(EnemyState newState) { state = newState; }

    void moveEnemy(const std::vector<glm::ivec2>& path, float deltaTime);


    EnemyState state = PATROL;
    float Yaw;
    float EnemyCameraYaw;
    float EnemyCameraPitch;
    glm::vec3 EnemyFront;
    glm::vec3 EnemyRight;
    glm::vec3 EnemyUp;
    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 Up;
    bool reachedDestination = false;
    glm::vec3 currentWaypoint;
};