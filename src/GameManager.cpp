#include "GameManager.h"

GameManager::GameManager(Window* window, unsigned int width, unsigned int height)
    : window(window)
{
    // TODO: Initialise Game Objects
    
    Camera camera(glm::vec3(50.0f, 3.0f, 80.0f));
    
    //camera = new Camera();
    //player = new Player();
    //enemy = new Enemy();
    
    inputManager = new InputManager();
    
    inputManager->setContext(&camera, player, enemy, width, height);
    
    window->setInputManager(inputManager);
    
    renderer = window->getRenderer();
    
    //gameObjects.push_back(player);
    //gameObjects.push_back(enemy);   
}

void GameManager::setupCamera(unsigned int width, unsigned int height)
{
    if (camera->Mode == PLAYER_FOLLOW)
    {
        camera->FollowTarget(player->getPosition(), player->PlayerFront, camera->playerCamRearOffset, camera->playerCamHeightOffset);
        view = camera->GetViewMatrixPlayerFollow(player->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (camera->Mode == ENEMY_FOLLOW)
    {
        camera->FollowTarget(enemy->getPosition(), enemy->EnemyFront, camera->enemyCamRearOffset, camera->enemyCamHeightOffset);
        view = camera->GetViewMatrixEnemyFollow(enemy->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (camera->Mode == FLY)
        view = camera->GetViewMatrix();

    projection = glm::perspective(glm::radians(camera->Zoom), (float)width / (float)height, 0.1f, 100.0f);

}

void GameManager::update(float deltaTime)
{
    inputManager->processInput(window->getWindow(), deltaTime);

    // TODO: Update Game Objects
    /*for (auto obj : gameObjects) {
        obj->update();
    }*/
}

void GameManager::render()
{
    // TODO:: Render Game Objects

    // for (auto obj : gameObjects) {
    // obj->setCameraMatrices(view, projection);
    // renderer->draw(obj);
    // }

}
