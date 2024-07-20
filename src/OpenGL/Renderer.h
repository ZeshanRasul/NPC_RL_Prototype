#pragma once
#include <GLFW/glfw3.h>

#include "RenderData.h"

class Renderer {
public:
	Renderer(GLFWwindow* window);

	bool init(unsigned int width, unsigned int height);
	void draw();

	void cleanup();

private:
	RenderData mRenderData{};
};