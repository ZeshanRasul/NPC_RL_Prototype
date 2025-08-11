#include "imgui/imgui.h"
#include "imgui/backend/imgui_impl_glfw.h"
#include "imgui/backend/imgui_impl_opengl3.h"

#include "InputManager.h"

void InputManager::HandleMouseMovement(double xPosIn, double yPosIn)
{
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	if (!m_controlCamera)
		return;

	float xPos = static_cast<float>(xPosIn);
	float yPos = static_cast<float>(yPosIn);

	if (m_firstMouse)
	{
		m_lastX = xPos;
		m_lastY = yPos;
		m_firstMouse = false;
	}

	float xOffset = xPos - m_lastX;
	float yOffset = m_lastY - yPos;

	m_lastX = xPos;
	m_lastY = yPos;

	if (m_camera->GetMode() == PLAYER_FOLLOW || m_camera->GetMode() == PLAYER_AIM)
		m_player->PlayerProcessMouseMovement(xOffset);
	else if (m_camera->GetMode() == ENEMY_FOLLOW)
		m_enemy->EnemyProcessMouseMovement(xOffset, yOffset, true);

	if (m_camera->GetMode() == PLAYER_FOLLOW)
	{
		m_player->SetPlayerYaw(m_camera->GetYaw() + 90.0f);
		m_player->SetAimPitch(m_camera->GetPitch());
		if (m_player->GetAimPitch() > 50.0f)
			m_player->SetAimPitch(50.0f);
		if (m_player->GetAimPitch() < -30.0f)
			m_player->SetAimPitch(-30.0f);
		m_player->UpdatePlayerVectors();
		m_player->UpdatePlayerAimVectors();
	}
	else if (m_camera->GetMode() == PLAYER_AIM)
	{
		m_player->SetPlayerYaw(m_camera->GetYaw());
		m_player->SetAimPitch(m_camera->GetPitch());
		//if (m_player->m_aimPitch > 19.0f)
		//    m_player->m_aimPitch = 19.0f;
		//if (m_player->m_aimPitch < -19.0f)
		//    m_player->m_aimPitch = -19.0f;
		m_player->UpdatePlayerVectors();
		m_player->UpdatePlayerAimVectors();
	}
	else if (m_camera->GetMode() == ENEMY_FOLLOW)
	{
		m_enemy->UpdateEnemyCameraVectors();
	}

	m_camera->ProcessMouseMovement(xOffset, yOffset);
}

void InputManager::HandleMouseScroll(double xOffset, double yOffset)
{
	m_camera->ProcessMouseScroll(static_cast<float>(yOffset));
}

void InputManager::ProcessInput(GLFWwindow* window, float deltaTime)
{
	bool spaceKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

	if (spaceKeyCurrentlyPressed && !m_spaceKeyPressed)
	{
		m_controlCamera = !m_controlCamera;

		if (!m_controlCamera)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}

	m_spaceKeyPressed = spaceKeyCurrentlyPressed;


	bool ctrlKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;


	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (!m_ctrlBeenPressed && ctrlKeyCurrentlyPressed)
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		{
			m_camera->SetMode(static_cast<CameraMode>((m_camera->GetMode() + 1) % MODE_COUNT));
		}
	}

	m_ctrlBeenPressed = ctrlKeyCurrentlyPressed;

	if (m_controlCamera && m_camera->GetMode() == FLY)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			m_camera->ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			m_camera->ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			m_camera->ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			m_camera->ProcessKeyboard(RIGHT, deltaTime);
	}
	bool shiftKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

	if (shiftKeyCurrentlyPressed && !m_shiftKeyPressed && (m_camera->GetMode() == PLAYER_FOLLOW || m_camera->GetMode() == PLAYER_AIM))
	{
		if (m_player->GetPlayerState() == MOVING)
		{
			m_player->SetPlayerState(AIMING);
			m_player->UpdatePlayerAimVectors();
			m_camera->SetMode(PLAYER_AIM);
		}
		else if (m_player->GetPlayerState() == AIMING)
		{
			m_player->SetPlayerState(MOVING);
			m_player->UpdatePlayerVectors();
			m_camera->SetMode(PLAYER_FOLLOW);
		}
		m_camera->hasSwitched = true;
		//-m_camera->LerpCamera();
		m_player->UpdatePlayerAimVectors();
		m_player->UpdatePlayerVectors();
	}

	m_shiftKeyPressed = shiftKeyCurrentlyPressed;

	bool leftClickCurrentlyPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;

	if (leftClickCurrentlyPressed && !m_leftClickPressed && m_camera->GetMode() == PLAYER_AIM)
	{
		if (m_player->GetPlayerState() == AIMING)
		{
			m_player->SetPlayerState(SHOOTING);
			m_player->Shoot();
		}
	}

	m_leftClickPressed = leftClickCurrentlyPressed;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && m_player->GetPlayerState() == SHOOTING)
	{
		m_player->SetPlayerState(AIMING);
	}

	bool rKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

	if (rKeyCurrentlyPressed && !m_rKeyPressed)
	{
		// TODO: Reset Enemies
		leftClickCurrentlyPressed = false;
		shiftKeyCurrentlyPressed = false;
		spaceKeyCurrentlyPressed = false;
		m_leftClickPressed = false;
		m_shiftKeyPressed = false;
		m_spaceKeyPressed = false;
		m_player->ResetGame();
	}

	m_rKeyPressed = rKeyCurrentlyPressed;

	bool pauseKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

	if (pauseKeyCurrentlyPressed && !m_pausePressed)
	{
		if (!isPaused)
		{
			pauseFactor = 0.0f;
			isPaused = true;
		}
		else
		{
			pauseFactor = 1.0f;
			isPaused = false;
		}
	}

	m_pausePressed = pauseKeyCurrentlyPressed;

	bool zKeyCurrentlyPressed = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;

	if (m_zKeyPressed && !m_zKeyBeenPressed)
	{
		if (!isTimeScaled)
		{
			timeScaleFactor = 0.25f;
			isTimeScaled = true;
		}
		else
		{
			timeScaleFactor = 1.0f;
			isTimeScaled = false;
		}
	}

	m_zKeyPressed = zKeyCurrentlyPressed;

	HandlePlayerMovement(window, *m_player, *m_camera, deltaTime * pauseFactor * timeScaleFactor);
}

void InputManager::SetContext(Camera* cam, Player* plyr, Enemy* enmy, unsigned int width, unsigned int height)
{
	m_camera = cam;
	m_player = plyr;
	m_enemy = enmy;

	m_lastX = width / 2.0f;
	m_lastY = height / 2.0f;
}

void InputManager::HandlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime)
{
	if (camera.GetMode() == PLAYER_FOLLOW || camera.GetMode() == PLAYER_AIM)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			player.SetPlayerYaw(camera.GetYaw());
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
		else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE && player.GetPrevDirection() == FORWARD)
		{
			player.SetVelocity(0.0f);
		}
		else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE && player.GetPrevDirection() == BACKWARD)
		{
			player.SetVelocity(0.0f);
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE && player.GetPrevDirection() == LEFT)
		{
			player.SetVelocity(0.0f);
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE && player.GetPrevDirection() == RIGHT)
		{
			player.SetVelocity(0.0f);
		}
	}


}
