#pragma once

#include <string>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, float yaw = -90.0f)
        : GameObject(pos, scale, shdr, applySkinning), PlayerYaw(yaw)
    {
        model = std::make_shared<GltfModel>();

        std::string modelFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman.gltf";
        std::string modelTextureFilename = "C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Woman/Woman.png";

        if (!model->loadModel(renderData, modelFilename, modelTextureFilename)) {
            Logger::log(1, "%s: loading glTF model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
        }

        model->uploadIndexBuffer();
        Logger::log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, modelFilename.c_str());

        size_t playerModelJointDualQuatBufferSize = model->getJointDualQuatsSize() *
            sizeof(glm::mat2x4);
        mPlayerDualQuatSSBuffer.init(playerModelJointDualQuatBufferSize);
        Logger::log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, playerModelJointDualQuatBufferSize);


        UpdatePlayerVectors();
    }

    ~Player()
    {
        model->cleanup();
    }

    void drawObject(glm::mat4 viewMat, glm::mat4 proj) override;

    void Update(float dt);

    glm::vec3 getPosition() {
        return position;
    }

    void setPosition(glm::vec3 newPos) {
        position = newPos;
    }

    void UpdatePlayerVectors();

    void PlayerProcessKeyboard(CameraMovement direction, float deltaTime);
    void PlayerProcessMouseMovement(float xOffset);

    void SetAnimation(int animNum, float speedDivider);

public:
    float PlayerYaw;
    glm::vec3 PlayerFront;
    glm::vec3 PlayerRight;
    glm::vec3 PlayerUp;
    float MovementSpeed = 7.5f;
    float mVelocity = 0.0f;

    bool uploadVertexBuffer = true;
    ShaderStorageBuffer mPlayerDualQuatSSBuffer{};
};