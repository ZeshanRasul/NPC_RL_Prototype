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
		mGameManager->CheckGameOver();

		currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		mGameManager->SetUpDebugUi();
		mGameManager->ShowDebugUi();
		mWindow->clear();

		mGameManager->SetSceneData();
		mGameManager->Update(deltaTime);
		mGameManager->CreateLightSpaceMatrices();
		mGameManager->SetupCamera(width, height);
		mGameManager->Render(false, true, false);
		mGameManager->Render(true, false, false);
		mGameManager->Render(false, false, true);

		mGameManager->RenderDebugUi();
		mWindow->mainLoop();

#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}
}
