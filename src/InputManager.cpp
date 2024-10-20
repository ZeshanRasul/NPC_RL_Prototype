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

    if (camera->Mode == PLAYER_FOLLOW || camera->Mode == PLAYER_AIM)
        player->PlayerProcessMouseMovement(xOffset);
    else if (camera->Mode == ENEMY_FOLLOW)
        enemy->EnemyProcessMouseMovement(xOffset, yOffset, true);

    if (camera->Mode == PLAYER_FOLLOW)
    {
        player->PlayerYaw = camera->Yaw;
        player->aimPitch = camera->Pitch;
        if (player->aimPitch > 19.0f)
            player->aimPitch = 19.0f;
        if (player->aimPitch < -19.0f)
            player->aimPitch = -19.0f;
        player->UpdatePlayerVectors();
        player->UpdatePlayerAimVectors();
    }
    else if (camera->Mode == PLAYER_AIM)
    {
		player->PlayerYaw = camera->Yaw;
        player->aimPitch = camera->Pitch;
        if (player->aimPitch > 19.0f)
            player->aimPitch = 19.0f;
        if (player->aimPitch < -19.0f)
            player->aimPitch = -19.0f;
        player->UpdatePlayerVectors();
		player->UpdatePlayerAimVectors();
    }
    else if (camera->Mode == ENEMY_FOLLOW)
    {
        enemy->UpdateEnemyCameraVectors();
    }

    camera->ProcessMouseMovement(xOffset, yOffset);
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

    if (!tabBeenPressed && tabKeyCurrentlyPressed)
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
    bool shiftKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    if (shiftKeyCurrentlyPressed && !shiftKeyPressed && (camera->Mode == PLAYER_FOLLOW || camera->Mode == PLAYER_AIM))
    {
        if (player->GetPlayerState() == MOVING)
        {
            player->SetPlayerState(AIMING);
            player->UpdatePlayerAimVectors();
			camera->Mode = PLAYER_AIM;
        }
        else if (player->GetPlayerState() == AIMING)
        {
			player->SetPlayerState(MOVING);
			player->UpdatePlayerVectors();
			camera->Mode = PLAYER_FOLLOW;
        }
    }

    shiftKeyPressed = shiftKeyCurrentlyPressed;

    bool leftClickCurrentlyPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;

    if (leftClickCurrentlyPressed && !leftClickPressed && camera->Mode == PLAYER_AIM)
    {
        if (player->GetPlayerState() == AIMING)
        {
            player->SetPlayerState(SHOOTING);
			player->Shoot();
        }
    }

    leftClickPressed = leftClickCurrentlyPressed;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && player->GetPlayerState() == SHOOTING)
	{
		player->SetPlayerState(AIMING);
	}

	bool rKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

	if (rKeyCurrentlyPressed && !rKeyPressed)
	{
        // TODO: Reset Enemies
        player->ResetEnemies();
    }

	rKeyPressed = rKeyCurrentlyPressed;

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
    if (camera.Mode == PLAYER_FOLLOW || camera.Mode == PLAYER_AIM)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            player.PlayerYaw = camera.Yaw;
            player.UpdatePlayerVectors();
            player.UpdatePlayerAimVectors();
            player.PlayerProcessKeyboard(FORWARD, deltaTime);
        }
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            player.PlayerProcessKeyboard(BACKWARD, deltaTime);
        else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            player.PlayerProcessKeyboard(LEFT, deltaTime);
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            player.PlayerProcessKeyboard(RIGHT, deltaTime);
		else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE || 
            glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE || 
            glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE)
			player.SetVelocity(0.0f);

    }

    
}
