#pragma once
#include <string>
#include <memory>

#include "src/OpenGL/Renderer.h"
#include "src/InputManager.h"
#include <GLFW/glfw3.h>

class Window {
public:
	bool init(unsigned int width, unsigned int height, std::string title);
	void mainLoop();
	bool isOpen() const { return !glfwWindowShouldClose(mWindow); };
	void clear() { mRenderer->clear(); }

	GLFWwindow* getWindow() { return mWindow; }
	Renderer* getRenderer() { return mRenderer; }

	void setInputManager(InputManager* inputManager);

	unsigned int GetWidth() const { return mWidth; }
	unsigned int GetHeight() const { return mHeight; }

	void cleanup();

private:
	GLFWwindow* mWindow;
	unsigned int mWidth;
	unsigned int mHeight;

	Renderer* mRenderer;
};
