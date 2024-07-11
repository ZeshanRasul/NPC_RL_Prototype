#pragma once

#include "GameObject.h"

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color) 
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/MaleMercenary/MaleMercenary.obj");
    }

    void Draw(Shader& shader) override;

    glm::vec3 getPosition() {
        return position;
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }
};