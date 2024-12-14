#pragma once

#include "src/Window/Window.h"
#include "src/GameManager.h"

class App {
public:
	App(unsigned int screenWidth, unsigned int screenHeight);
	~App();

	void Run();

	Window& GetWindow() { return *m_window; }

private:
	Window* m_window = nullptr;
	GameManager* m_gameManager = nullptr;

	unsigned int m_width;
	unsigned int m_height;

	float m_deltaTime = 0.0f;
	float m_lastFrame = 0.0f;
	float m_currentFrame = 0.0f;
};