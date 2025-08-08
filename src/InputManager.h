#pragma once

#include "src/Camera.h"
#include "src/GameObjects/Player.h"
#include "src/GameObjects/Enemy.h"
#include "GLFW/glfw3.h"

class InputManager {
public:
	static void MouseCallback(GLFWwindow* window, double xPosIn, double yPosIn) {
		// Retrieve the InputManager instance
		InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

		if (inputManager) {
			inputManager->HandleMouseMovement(xPosIn, yPosIn);
		}
	}

	static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		// Retrieve the InputManager instance
		InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

		if (inputManager) {
			inputManager->HandleMouseScroll(xOffset, yOffset);
		}
	}

	void HandleMouseMovement(double xpos, double ypos);
	void HandleMouseScroll(double xOffset, double yOffset);
	void ProcessInput(GLFWwindow* window, float deltaTime);
	void HandlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime);
	float GetPauseFactor() const { return pauseFactor; }
	float GetTimeScaleFacotr() const { return timeScaleFactor; }
	bool GetIsPaused() const { return isPaused; }
	bool GetIsTimeScaled() const { return isTimeScaled; }

	void SetContext(Camera* cam, Player* plyr, Enemy* enmy, unsigned int width, unsigned int height);

private:
	Camera* m_camera;
	Player* m_player;
	Enemy* m_enemy;

	float m_lastX = 0;
	float m_lastY = 0;
	bool m_firstMouse = true;

	float pauseFactor = 1.0f;
	float timeScaleFactor = 1.0f;
	bool isPaused = false;
	bool isTimeScaled = false;


	bool m_controlCamera = true;
	bool m_isTabPressed = false;
	bool m_ctrlBeenPressed = false;
	bool m_spaceKeyPressed = false;
	bool m_shiftKeyPressed = false;
	bool m_leftClickPressed = false;
	bool m_rKeyBeenPressed = false;
	bool m_rKeyPressed = false;
	bool m_pausePressed = false;
	bool m_pauseKeyBeenPressed = false;
	bool m_zKeyPressed = false;
	bool m_zKeyBeenPressed = false;
};
