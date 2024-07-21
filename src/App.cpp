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
        mWindow->clear();

        // TODO: Update deltaTime arg
        mGameManager->update(1.0f/60.0f);
        mGameManager->render();

        mWindow->mainLoop();
    }
}
