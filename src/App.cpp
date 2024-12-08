#include "App.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
    #include "tracy/Tracy.hpp"
#endif

App::App(unsigned int screenWidth, unsigned int screenHeight)
    : width(screenWidth), height(screenHeight)
{
    mWindow = new Window();

    if (!mWindow->init(screenWidth, screenHeight, "NPC AI System")) {
        Logger::log(1, "%s error: Window init error\n", __FUNCTION__);
    }

	mGameManager = new GameManager(mWindow, width, height);
}



App::~App()
{
    mWindow->cleanup();
    delete mGameManager;
}

void App::run()
{
    while (mWindow->isOpen()) {
        mGameManager->RemoveDestroyedGameObjects();

        currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        mGameManager->setUpDebugUI();
        mGameManager->showDebugUI();
        mWindow->clear();

        mGameManager->setupCamera(width, height);
        mGameManager->setSceneData();
        mGameManager->update(deltaTime);
        mGameManager->CreateLightSpaceMatrices();
		mGameManager->render(false, true, false);
        mGameManager->render(true, false, false);
        mGameManager->render(false, false, true);

        mGameManager->renderDebugUI();
        mWindow->mainLoop();

#ifdef TRACY_ENABLE
        FrameMark;
#endif
    }
}
