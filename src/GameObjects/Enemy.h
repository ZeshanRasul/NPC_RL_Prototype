#pragma once

#include "GameObject.h"

enum EnemyState {
    PATROL,
    ATTACK
};

static const char* EnemyStateNames[] = {
    "Patrol",
    "Attack"
};

class Enemy : public GameObject {
public:

    Enemy(glm::vec3 pos, glm::vec3 scale, glm::vec3 color, float yaw = 0.0f)
        : GameObject(pos, scale, color), Yaw(yaw)
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/MaleMilitary/MaleMilitary.obj");
        UpdateEnemyCameraVectors();
        UpdateEnemyVectors();
    }

    void Draw(Shader& shader) override;

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

};