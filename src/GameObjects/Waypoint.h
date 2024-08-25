#pragma once

#include "GameObject.h"

class Waypoint : public GameObject {
public:
    Waypoint(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
    {
//        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/Barrel/Barrel.obj");
        ComputeAudioWorldTransform();
    }

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void ComputeAudioWorldTransform() override;
};