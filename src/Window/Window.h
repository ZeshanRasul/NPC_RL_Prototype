#pragma once
#include <string>
#include <memory>
#include <GLFW/glfw3.h>

#include "src/OpenGL/Renderer.h"
#include "src/InputManager.h"

class Window {
public:
	bool init(unsigned int width, unsigned int height, std::string title);
	void mainLoop();
	bool isOpen() const { return !glfwWindowShouldClose(mWindow); };
	void clear() { mRenderer->clear(); }
	
	GLFWwindow* getWindow() { return mWindow; }
	Renderer* getRenderer() { return mRenderer; }

	void setInputManager(InputManager* inputManager);

	void cleanup();

private:
	GLFWwindow* mWindow = nullptr;

	Renderer* mRenderer;
};
