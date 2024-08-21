#include "Player.h"

void Player::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = glm::rotate(modelMat, glm::radians(-PlayerYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, scale);
    std::vector<glm::mat4> matrixData;
    matrixData.push_back(viewMat);
    matrixData.push_back(proj);
    matrixData.push_back(modelMat);
    mUniformBuffer.uploadUboData(matrixData, 0);

    if (uploadVertexBuffer)
    {
        model->uploadVertexBuffers();
        uploadVertexBuffer = false;
    }
    //model->uploadPositionBuffer();
    model->draw();
}

void Player::Update(float dt) 
{}


void Player::UpdatePlayerVectors()
{
    glm::vec3 front = glm::vec3(1.0f);
    front.x = cos(glm::radians(PlayerYaw - 90.0f));
    front.y = 0.0f;
    front.z = sin(glm::radians(PlayerYaw - 90.0f));
    PlayerFront = glm::normalize(front);
    PlayerRight = glm::normalize(glm::cross(PlayerFront, glm::vec3(0.0f, 1.0f, 0.0f)));  
    PlayerUp = glm::normalize(glm::cross(PlayerRight, PlayerFront));
}

void Player::PlayerProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime; 
    if (direction == FORWARD)
        position += PlayerFront * velocity;
    if (direction == BACKWARD)
        position -= PlayerFront * velocity;
    if (direction == LEFT)
        position -= PlayerRight * velocity;
    if (direction == RIGHT)
        position += PlayerRight * velocity;
}

void Player::PlayerProcessMouseMovement(float xOffset)
{
//    xOffset *= SENSITIVITY;  

    PlayerYaw += xOffset;

    UpdatePlayerVectors();
}
