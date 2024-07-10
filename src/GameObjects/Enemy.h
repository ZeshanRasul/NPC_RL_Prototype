#pragma once

#include "GameObject.h"

class Enemy : public GameObject {
public:
    Enemy(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color) 
    {
        setupVAO();
    }

    void Draw(Shader& shader) override;
};