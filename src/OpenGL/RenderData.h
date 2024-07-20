#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct RenderData {
	GLFWwindow* rdWindow = nullptr;

	int rdWidth = 0;
	int rdHeight = 0;
};