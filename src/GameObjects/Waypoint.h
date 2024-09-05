#pragma once

#include "GameObject.h"

class Waypoint : public GameObject {
public:
    Waypoint(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = 0.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
    {
		model = std::make_shared<GltfModel>();
        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Wall/Box.gltf";
        std::string modelTextureFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman.png";
    
        if (!model->loadModel(renderData, modelFilename, modelTextureFilename, false)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }
        model->uploadVertexBuffers(2);

        model->uploadIndexBuffer();
        Logger::log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());


        ComputeAudioWorldTransform();
    }

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void ComputeAudioWorldTransform() override;

    void OnHit() override {};
    void OnMiss() override {};
};