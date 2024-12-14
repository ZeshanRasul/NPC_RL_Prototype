#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
	glm::vec3 m_position;
	glm::vec3 m_color;
	glm::vec2 m_uv;
};

struct Mesh
{
	std::vector<Vertex> m_vertices;
};

struct Material
{
	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
	float m_shininess;
};

struct DirLight
{
	glm::vec3 m_direction;

	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
};

struct PointLight
{
	glm::vec3 m_position;

	float m_constant;
	float m_linear;
	float m_quadratic;

	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
};

struct SpotLight
{
	glm::vec3 m_position;
	glm::vec3 m_direction;
	float m_cutoff;
	float m_outerCutoff;

	glm::vec3 m_ambient;
	glm::vec3 m_diffuse;
	glm::vec3 m_specular;
};

struct RenderData
{
	GLFWwindow* m_rdWindow = nullptr;

	int m_rdWidth = 0;
	int m_rdHeight = 0;

	unsigned int m_rdGltfTriangleCount = 0;
	size_t m_animClipsSize = 0;
};
