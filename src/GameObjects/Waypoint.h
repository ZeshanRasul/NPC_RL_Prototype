#pragma once

#include "GameObject.h"

class Waypoint : public GameObject {
public:
    Waypoint(glm::vec3 pos, glm::vec3 scale, Shader* shdr)
        : GameObject(pos, scale, shdr)
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/Barrel/Barrel.obj");
    }

    void drawObject() const override;
};