#pragma once
#include <string>
#include <memory>

#include "src/OpenGL/Renderer.h"
#include "src/InputManager.h"
#include <GLFW/glfw3.h>

class Window {
public:
	bool Init(unsigned int width, unsigned int height, std::string title);
	void MainLoop();
	bool IsOpen() const { return !glfwWindowShouldClose(m_window); };
	void Clear() { m_renderer->Clear(); }

	GLFWwindow* GetWindow() { return m_window; }
	Renderer* GetRenderer() { return m_renderer; }

	void SetInputManager(InputManager* inputManager);

	unsigned int GetWidth() const { return m_width; }
	unsigned int GetHeight() const { return m_height; }

	void Cleanup();

private:
	GLFWwindow* m_window;
	unsigned int m_width;
	unsigned int m_height;

	Renderer* m_renderer;
};
