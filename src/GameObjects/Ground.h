#pragma once

#include "GameObject.h"

class Ground : public GameObject {
public:
    Ground(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning)
        : GameObject(pos, scale, shdr, applySkinning)
    {
//        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/GrassBase/GrassBase.obj");
    }

    void drawObject() const override;
};