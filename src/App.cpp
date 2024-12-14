#include "App.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

App::App(unsigned int screenWidth, unsigned int screenHeight)
	: m_width(screenWidth), m_height(screenHeight)
{
	m_window = new Window();

	if (!m_window->Init(screenWidth, screenHeight, "NPC AI System")) {
		Logger::Log(1, "%s error: Window init error\n", __FUNCTION__);
	}

	m_gameManager = new GameManager(m_window, m_width, m_height);
}



App::~App()
{
	m_window->Cleanup();
	delete m_gameManager;
}

void App::Run()
{
	while (m_window->IsOpen()) {
		m_gameManager->CheckGameOver();

		m_currentFrame = (float)glfwGetTime();
		m_deltaTime = m_currentFrame - m_lastFrame;
		m_lastFrame = m_currentFrame;

		m_gameManager->SetUpDebugUi();
		m_gameManager->ShowDebugUi();
		m_window->Clear();

		m_gameManager->SetSceneData();
		m_gameManager->Update(m_deltaTime);
		m_gameManager->CreateLightSpaceMatrices();
		m_gameManager->SetupCamera(m_width, m_height);
		m_gameManager->Render(false, true, false);
		m_gameManager->Render(true, false, false);
		m_gameManager->Render(false, false, true);

		m_gameManager->RenderDebugUi();
		m_window->MainLoop();

#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}
}
