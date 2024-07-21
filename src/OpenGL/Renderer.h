#pragma once
#include <GLFW/glfw3.h>

#include "RenderData.h"
#include "src/Camera.h"
#include "src/GameObjects/GameObject.h"

class Renderer {
public:
	Renderer(GLFWwindow* window);

	bool init(unsigned int width, unsigned int height);
	void draw(GameObject* gameObj);
	void clear();

	void cleanup();

private:
	RenderData mRenderData{};
};