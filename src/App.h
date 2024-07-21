#pragma once

#include "src/Window/Window.h"
#include "src/GameManager.h"

class App {
public:
	App(unsigned int screenWidth, unsigned int screenHeight);
	~App();

	void run();

	Window& getWindow() { return *mWindow; }

private:
	Window* mWindow = nullptr;
	GameManager* mGameManager = nullptr;

	unsigned int width;
	unsigned int height;
};