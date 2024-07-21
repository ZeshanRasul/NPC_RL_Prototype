#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Mesh {
	std::vector<Vertex> vertices;
};

struct RenderData {
	GLFWwindow* rdWindow = nullptr;

	int rdWidth = 0;
	int rdHeight = 0;
};