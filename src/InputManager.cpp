#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#include "InputManager.h"

void InputManager::handleMouseMovement(double xPosIn, double yPosIn)
{   
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    float xPos = static_cast<float>(xPosIn);
    float yPos = static_cast<float>(yPosIn);

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    if (camera->Mode == PLAYER_FOLLOW)
        player->PlayerProcessMouseMovement(xOffset);
    else if (camera->Mode == ENEMY_FOLLOW)
        enemy->EnemyProcessMouseMovement(xOffset, yOffset, true);

    camera->ProcessMouseMovement(xOffset, yOffset);

    if (camera->Mode == PLAYER_FOLLOW)
    {
        player->PlayerYaw = camera->Yaw;
        player->UpdatePlayerVectors();
    }
    else if (camera->Mode == ENEMY_FOLLOW)
    {
        enemy->UpdateEnemyCameraVectors();
    }
}

void InputManager::handleMouseScroll(double xOffset, double yOffset)
{
    camera->ProcessMouseScroll(static_cast<float>(yOffset));
}

void InputManager::processInput(GLFWwindow* window, float deltaTime)
{
    bool spaceKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (spaceKeyCurrentlyPressed && !spaceKeyPressed)
    {
        controlCamera = !controlCamera;

        if (!controlCamera)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    spaceKeyPressed = spaceKeyCurrentlyPressed;


    bool tabKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;


    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!tabBeenPressed && isTabPressed)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera->Mode = static_cast<CameraMode>((camera->Mode + 1) % MODE_COUNT);
    }

    tabBeenPressed = tabKeyCurrentlyPressed;

    if (controlCamera && camera->Mode == FLY)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera->ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera->ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera->ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera->ProcessKeyboard(RIGHT, deltaTime);
    }

    handlePlayerMovement(window, *player, *camera, deltaTime);
}

void InputManager::setContext(Camera* cam, Player* plyr, Enemy* enmy, unsigned int width, unsigned int height)
{
    camera = cam;
    player = plyr;
    enemy = enmy;

    lastX = width / 2.0f;
    lastY = height / 2.0f;
}

void InputManager::handlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime)
{
    if (camera.Mode == PLAYER_FOLLOW)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            player.PlayerYaw = camera.Yaw;
            player.UpdatePlayerVectors();
            player.PlayerProcessKeyboard(FORWARD, deltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            player.PlayerProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            player.PlayerProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            player.PlayerProcessKeyboard(RIGHT, deltaTime);

    }
}
