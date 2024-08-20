#pragma once

#include "src/OpenGL/Renderer.h"
#include "src/OpenGL/RenderData.h"
#include "src/OpenGL/UniformBuffer.h"
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
    Enemy* enemy2;
    Enemy* enemy3;
    Enemy* enemy4;
    InputManager* inputManager;
    std::vector<GameObject*> gameObjects;

    Shader playerShader{};
    Shader enemyShader{};
    Shader gridShader{};
    UniformBuffer mEnemyUniformBuffer{};
    UniformBuffer mEnemy2UniformBuffer{};
    UniformBuffer mEnemy3UniformBuffer{};
    UniformBuffer mEnemy4UniformBuffer{};
    int enemyJointMatrixSize;
    int enemy2JointMatrixSize;
    int enemy3JointMatrixSize;
    int enemy4JointMatrixSize;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    Cell cell;

    Waypoint* waypoint1;
    Waypoint* waypoint2;
    Waypoint* waypoint3;
    Waypoint* waypoint4;

    int currentStateIndex = 0;
};
