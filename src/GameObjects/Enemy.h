#pragma once

#include "GameObject.h"

class Enemy : public GameObject {
public:
    Enemy(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color) 
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/MaleMilitary/MaleMilitary.obj");
    }

    void Draw(Shader& shader) override;

    glm::vec3 getPosition() {
        return position;
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }
};