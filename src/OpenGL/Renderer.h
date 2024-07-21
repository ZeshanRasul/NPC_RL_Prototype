#pragma once
#include <GLFW/glfw3.h>

#include "RenderData.h"
#include "src/Camera.h"
#include "src/GameObjects/GameObject.h"
#include "Shader.h"

class Renderer {
public:
	Renderer(GLFWwindow* window);

	bool init(unsigned int width, unsigned int height);
	
	void setScene(glm::mat4 viewMat, glm::mat4 proj, DirLight light);
	void draw(GameObject* gameObj);
	void clear();

	void cleanup();

private:
	RenderData mRenderData{};

	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	
	DirLight sun;
};