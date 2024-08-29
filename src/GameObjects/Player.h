#pragma once

#include <string>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Physics/AABB.h"

enum PlayerState {
    MOVING,
    AIMING,
    SHOOTING,
    PLAYER_STATE_COUNT
};

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, Shader* shdr, bool applySkinning, GameManager* gameMgr, float yaw = -90.0f)
        : GameObject(pos, scale, yaw, shdr, applySkinning, gameMgr)
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

        ComputeAudioWorldTransform();

        UpdatePlayerVectors();
		PlayerAimFront = PlayerFront;
		PlayerAimRight = PlayerRight;
		PlayerAimUp = PlayerUp;
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
		mRecomputeWorldTransform = true;
    }

    void ComputeAudioWorldTransform() override;

    void UpdatePlayerVectors();
    void UpdatePlayerAimVectors();

    void PlayerProcessKeyboard(CameraMovement direction, float deltaTime);
    void PlayerProcessMouseMovement(float xOffset);

    void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards);

    float GetVelocity() const { return mVelocity; }
	void SetVelocity(float newVelocity) { mVelocity = newVelocity; }

    PlayerState GetPlayerState() const { return mPlayerState; }
    void SetPlayerState(PlayerState newState);

	glm::vec3 GetShootPos() { return getPosition() + glm::vec3(0.0f, 2.5f, 0.0f); }
	float GetShootDistance() const { return shootDistance; }

    void Shoot();

    void updateAABB() {
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f), glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), scale);
        aabb.update(modelMatrix);
    }

    AABB GetAABB() const { return aabb; }
    void renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* aabbSdr);
    void setAABBColor(glm::vec3 color) { aabbColor = color; }

    void OnHit() override;

    AABB aabb;
    glm::vec3 aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

public:
    float PlayerYaw;
    glm::vec3 PlayerFront;
    glm::vec3 PlayerRight;
    glm::vec3 PlayerUp;
    glm::vec3 PlayerAimFront;
    glm::vec3 PlayerAimRight;
    glm::vec3 PlayerAimUp;

	float aimPitch = 0.0f;

    glm::vec3 mShootStartPos = getPosition() + (glm::vec3(0.0f, 2.5f, 0.0f));
    float shootDistance = 100000.0f;

    float MovementSpeed = 7.5f;
    float mVelocity = 0.0f;

    bool uploadVertexBuffer = true;
    ShaderStorageBuffer mPlayerDualQuatSSBuffer{};

    PlayerState mPlayerState = MOVING;

    Shader* aabbShader;
};