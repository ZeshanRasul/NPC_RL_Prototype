#pragma once

#include "src/OpenGL/Renderer.h"
#include "src/Window/Window.h"

#include "src/InputManager.h"

#include "src/Camera.h"
#include "GameObjects/Player.h"
#include "GameObjects/Enemy.h"

class GameManager {
public:
    GameManager(Window* window, unsigned int width, unsigned int height)
        : window(window) {

        // TODO: Initialise Game Objects

        //camera = new Camera();
        //player = new Player();
        //enemy = new Enemy();

        inputManager = new InputManager();

        inputManager->setContext(camera, player, enemy, width, height);
       
        window->setInputManager(inputManager);

        renderer = window->getRenderer();

        //gameObjects.push_back(player);
        //gameObjects.push_back(enemy);
    }

    ~GameManager() {
        delete camera;
        delete player;
        delete enemy;
        delete inputManager;
    }

    void update(float deltaTime) {
        inputManager->processInput(window->getWindow(), deltaTime);
        
        // TODO: Update Game Objects
        /*for (auto obj : gameObjects) {
            obj->update();
        }*/
    }

    void render() {

        // TODO:: Render Game Objects
        
        //renderer->draw(gameObjects);
    }

private:
    Renderer* renderer;
    Window* window;
    Camera* camera;
    Player* player;
    Enemy* enemy;
    InputManager* inputManager;
    std::vector<GameObject*> gameObjects;
};
