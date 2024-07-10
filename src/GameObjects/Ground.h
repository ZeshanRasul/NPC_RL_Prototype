#pragma once

#include "GameObject.h"

class Ground : public GameObject {
public:
    Ground(glm::vec3 pos, glm::vec3 scale, glm::vec3 color)
        : GameObject(pos, scale, color)
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/GrassBase/GrassBase.obj");
    }

    void Draw(Shader& shader) override;
};