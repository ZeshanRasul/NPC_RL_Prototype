#pragma once

#include "GameObject.h"

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color) 
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/backpack/backpack.obj");
    }

    void Draw(Shader& shader) override;
};