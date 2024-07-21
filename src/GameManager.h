#pragma once

#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/RenderData.h"
#include "src/Window/Window.h"

#include "src/InputManager.h"

#include "src/Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"
#include "GameObjects/Waypoint.h"
#include "src/Pathfinding/Grid.h"

class GameManager {
public:
    GameManager(Window* window, unsigned int width, unsigned int height);

    ~GameManager() {
        delete camera;
        delete player;
        delete enemy;
        delete inputManager;
    }

    void setupCamera(unsigned int width, unsigned int height);
    void setSceneData();


    void update(float deltaTime);

    void render();

    void setUpDebugUI();
    void showDebugUI();
    void renderDebugUI();
private:
    void ShowCameraControlWindow(Camera& cam);
    void ShowLightControlWindow(DirLight& light);

    Renderer* renderer;
    Window* window;
    Camera* camera;
    Player* player;
    Enemy* enemy;
    InputManager* inputManager;
    std::vector<GameObject*> gameObjects;

    Shader playerShader{};
    Shader enemyShader{};
    Shader gridShader{};

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    Cell cell;

    Waypoint* waypoint1;
    Waypoint* waypoint2;
    Waypoint* waypoint3;
    Waypoint* waypoint4;

    int currentStateIndex = 0;
};
