#include "App.h"
#include "src/Tools/Logger.h"

App::App(unsigned int screenWidth, unsigned int screenHeight)
    : width(screenWidth), height(screenHeight)
{
    if (!mWindow->init(width, height, "NPC AI System")) {
        Logger::log(1, "%s error: Window init error\n", __FUNCTION__);
    }
    
    mGameManager = new GameManager(mWindow, width, height);
}

App::~App()
{
    mWindow->cleanup();
    delete mWindow;
    delete mGameManager;
}

void App::run()
{
    while (mWindow->isOpen()) {
        currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        mWindow->clear();

        mGameManager->setupCamera(width, height);
        mGameManager->update(deltaTime);
        mGameManager->render();

        mWindow->mainLoop();
    }
}
