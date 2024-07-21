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

DirLight dirLight = {
        glm::vec3(-0.2f, -1.0f, -0.3f),

        glm::vec3(0.15f, 0.15f, 0.15f),
        glm::vec3(0.4f),
        glm::vec3(0.1f, 0.1f, 0.1f)
};

glm::vec3 snapToGrid(const glm::vec3& position) {
    int gridX = static_cast<int>(position.x / CELL_SIZE);
    int gridZ = static_cast<int>(position.z / CELL_SIZE);
    return glm::vec3(gridX * CELL_SIZE + CELL_SIZE / 2.0f, position.y, gridZ * CELL_SIZE + CELL_SIZE / 2.0f);
}

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

private:
    void showDebugUI();
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
