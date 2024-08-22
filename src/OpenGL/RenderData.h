#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 uv;
};

struct Mesh {
	std::vector<Vertex> vertices;
};

struct Material {
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;
};

struct DirLight {
	glm::vec3 direction;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct PointLight {
	glm::vec3 position;

	float constant;
	float linear;
	float quadratic;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct SpotLight {
	glm::vec3 position;
	glm::vec3 direction;
	float cutoff;
	float outerCutoff;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct RenderData {
	GLFWwindow* rdWindow = nullptr;

	int rdWidth = 0;
	int rdHeight = 0;

	unsigned int rdGltfTriangleCount = 0;
	size_t animClipsSize = 0;
};