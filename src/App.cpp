#include "App.h"
#include "src/Tools/Logger.h"

App::App(unsigned int screenWidth, unsigned int screenHeight)
    : width(screenWidth), height(screenHeight)
{
    mWindow = new Window();

    if (!mWindow->init(screenWidth, screenHeight, "NPC AI System")) {
        Logger::log(1, "%s error: Window init error\n", __FUNCTION__);
    }
    
    mGameManager = new GameManager(mWindow, screenWidth, screenHeight);
}

App::~App()
{
    mWindow->cleanup();
    delete mGameManager;
}

void App::run()
{
    while (mWindow->isOpen()) {
        currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        mGameManager->setUpDebugUI();
        mGameManager->showDebugUI();
        mWindow->clear();

        mGameManager->setupCamera(width, height);
        mGameManager->update(deltaTime);
        mGameManager->render();

        mGameManager->renderDebugUI();
        mWindow->mainLoop();
    }
}
