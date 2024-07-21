#pragma once

#include "GameObject.h"
#include "../Camera.h"

class Player : public GameObject {
public:
    Player(glm::vec3 pos, glm::vec3 scale, Shader* shdr, float yaw = -90.0f)
        : GameObject(pos, scale, shdr), PlayerYaw(yaw)
    {
        model.LoadModel("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Models/MaleMercenary/MaleMercenary.obj");
        UpdatePlayerVectors();
    }

    void drawObject() const override;

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

public:
    float PlayerYaw;
    glm::vec3 PlayerFront;
    glm::vec3 PlayerRight;
    glm::vec3 PlayerUp;
    float MovementSpeed = 7.5f;
};