#pragma once

#include "GameObject.h"

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color) 
    {
        setupVAO();
    }

    void Draw(Shader& shader) override;
};