#pragma once
#include <string>
#include <memory>
#include <GLFW/glfw3.h>

#include "src/OpenGL/Renderer.h"
#include "src/GameObjects/Player.h"
#include "src/GameObjects/Enemy.h"
#include "src/Camera.h"

class Window {
public:
	bool init(unsigned int width, unsigned int height, std::string title);
	void mainLoop();
	void cleanup();

private:
	GLFWwindow* mWindow = nullptr;

	std::unique_ptr<Renderer> mRenderer;
};

// TODO: Move these elsewhere
Player* g_player = nullptr;
Enemy* g_enemy = nullptr;
Camera* g_camera = nullptr;

bool controlCamera = true;
bool spaceKeyPressed = false;

bool tabKeyPressed = false;

float lastX = 0;
float lastY = 0;
bool firstMouse = true;
