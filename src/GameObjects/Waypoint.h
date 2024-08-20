#pragma once

#include "GameObject.h"

class Waypoint : public GameObject {
public:
    Waypoint(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning)
        : GameObject(pos, scale, shdr, applySkinning)
    {
//        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/Barrel/Barrel.obj");
    }

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;
};