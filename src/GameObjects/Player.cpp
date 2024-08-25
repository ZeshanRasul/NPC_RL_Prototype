#include "Player.h"

void Player::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = glm::rotate(modelMat, glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, scale);
    std::vector<glm::mat4> matrixData;
    matrixData.push_back(viewMat);
    matrixData.push_back(proj);
    matrixData.push_back(modelMat);
    mUniformBuffer.uploadUboData(matrixData, 0);

    mPlayerDualQuatSSBuffer.uploadSsboData(model->getJointDualQuats(), 2);

//    model->playAnimation(0, 0.8f);

    if (uploadVertexBuffer)
    {
        model->uploadVertexBuffers();
        uploadVertexBuffer = false;
    }

    model->draw();
}

void Player::Update(float dt) 
{
    ComputeAudioWorldTransform();
    UpdateComponents(dt);
}


void Player::ComputeAudioWorldTransform()
{
    if (mRecomputeWorldTransform)
    {
        mRecomputeWorldTransform = false;
        glm::mat4 worldTransform = glm::mat4(1.0f);
        // Scale, then rotate, then translate
        audioWorldTransform = glm::translate(worldTransform, position);
        audioWorldTransform = glm::rotate(worldTransform, glm::radians(-yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        audioWorldTransform = glm::scale(worldTransform, scale);

        // Inform components world transform updated
        for (auto comp : mComponents)
        {
            comp->OnUpdateWorldTransform();
        }
    }
}

void Player::UpdatePlayerVectors()
{
    glm::vec3 front = glm::vec3(1.0f);
    front.x = cos(glm::radians(yaw - 90.0f));
    front.y = 0.0f;
    front.z = sin(glm::radians(yaw - 90.0f));
    PlayerFront = glm::normalize(front);
    PlayerRight = glm::normalize(glm::cross(PlayerFront, glm::vec3(0.0f, 1.0f, 0.0f)));  
    PlayerUp = glm::normalize(glm::cross(PlayerRight, PlayerFront));
}

void Player::PlayerProcessKeyboard(CameraMovement direction, float deltaTime)
{
    mVelocity = MovementSpeed * deltaTime;

    if (direction == FORWARD)
    {
        position += PlayerFront * mVelocity;
		mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
    if (direction == BACKWARD)
    {
        position -= PlayerFront * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
    if (direction == LEFT)
    {
        position -= PlayerRight * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
    if (direction == RIGHT)
    {
        position += PlayerRight * mVelocity;
        mRecomputeWorldTransform = true;
        ComputeAudioWorldTransform();
        UpdateComponents(deltaTime);
    }
}

void Player::PlayerProcessMouseMovement(float xOffset)
{
//    xOffset *= SENSITIVITY;  

    yaw += xOffset;
    mRecomputeWorldTransform = true;
    ComputeAudioWorldTransform();
    UpdatePlayerVectors();
}

void Player::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards)
{
    model->playAnimation(animNum, speedDivider, blendFactor, playAnimBackwards);
}

