#pragma once
#include "GLFW/glfw3.h"

#include "src/Camera.h"
#include "src/GameObjects/Player.h"
#include "src/GameObjects/Enemy.h"

class InputManager {
public:
    static void mouse_callback(GLFWwindow* window, double xPosIn, double yPosIn) {
        // Retrieve the InputManager instance
        InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

        if (inputManager) {
            inputManager->handleMouseMovement(xPosIn, yPosIn);
        }
    }

    static void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
    {
        // Retrieve the InputManager instance
        InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

        if (inputManager) {
            inputManager->handleMouseScroll(xOffset, yOffset);
        }
    }

    void handleMouseMovement(double xpos, double ypos);
    void handleMouseScroll(double xOffset, double yOffset);
    void processInput(GLFWwindow* window, float deltaTime);
    void handlePlayerMovement(GLFWwindow* window, Player& player, Camera& camera, float deltaTime);


    void setContext(Camera* cam, Player* plyr, Enemy* enmy, unsigned int width, unsigned int height);

private:
    Camera* camera;
    Player* player;
    Enemy* enemy;

    float lastX = 0;
    float lastY = 0;
    bool firstMouse = true;

    bool controlCamera = true;
    bool isTabPressed = false;
    bool tabBeenPressed = false;
    bool spaceKeyPressed = false;
};
