#pragma once

#include "GameObject.h"

class Enemy : public GameObject {
public:

    enum EnemyState {
        PATROL,
        ATTACK
    };

    Enemy(glm::vec3 pos, glm::vec3 scale, glm::vec3 color, float yaw = 90.0f)
        : GameObject(pos, scale, color), Yaw(yaw)
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/MaleMilitary/MaleMilitary.obj");
        UpdateEnemyCameraVectors();
    }

    void Draw(Shader& shader) override;

    glm::vec3 getPosition() {
        return position;
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }

    void UpdateEnemyCameraVectors();

    void EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);



    float Yaw;
    float EnemyCameraYaw;
    float EnemyCameraPitch;
    glm::vec3 EnemyFront;
    glm::vec3 EnemyRight;
    glm::vec3 EnemyUp;

};