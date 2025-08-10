#include "AABB.h"
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

AABB::AABB()
	: m_min(std::numeric_limits<float>::max()), m_max(std::numeric_limits<float>::lowest())
{
}

AABB::AABB(const glm::vec3& min, const glm::vec3& max)
	: m_min(min), m_max(max)
{
}

void AABB::CalculateAABB(const std::vector<glm::vec3>& vertices)
{
	for (const auto& vertex : vertices)
	{
		m_min = min(m_min, vertex);
		m_max = max(m_max, vertex);
	}
}

void AABB::SetUpMesh()
{
	glm::vec3 min = m_transformedMin;
	glm::vec3 max = m_transformedMax;

	m_lineVertices = {
		{min.x, min.y, min.z}, {max.x, min.y, min.z},
		{max.x, min.y, min.z}, {max.x, max.y, min.z},
		{max.x, max.y, min.z}, {min.x, max.y, min.z},
		{min.x, max.y, min.z}, {min.x, min.y, min.z},

		{min.x, min.y, max.z}, {max.x, min.y, max.z},
		{max.x, min.y, max.z}, {max.x, max.y, max.z},
		{max.x, max.y, max.z}, {min.x, max.y, max.z},
		{min.x, max.y, max.z}, {min.x, min.y, max.z},

		{min.x, min.y, min.z}, {min.x, min.y, max.z},
		{max.x, min.y, min.z}, {max.x, min.y, max.z},
		{max.x, max.y, min.z}, {max.x, max.y, max.z},
		{min.x, max.y, min.z}, {min.x, max.y, max.z}
	};

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_lineVertices.size() * sizeof(glm::vec3), m_lineVertices.data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void*>(nullptr));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void AABB::Render(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 model, glm::vec3 aabbColor)
{
	m_shader->Use();

	m_shader->SetMat4("projection", proj);
	m_shader->SetMat4("view", viewMat);
	m_shader->SetMat4("model", glm::mat4(1.0f));
	m_shader->SetVec3("color", aabbColor);

	glBindVertexArray(m_vao);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineVertices.size()));
	glBindVertexArray(0);
}

void AABB::Update(const glm::mat4& modelMatrix)
{
	std::vector<glm::vec3> corners = {
		m_min,
		{m_min.x, m_min.y, m_max.z},
		{m_min.x, m_max.y, m_min.z},
		{m_min.x, m_max.y, m_max.z},
		{m_max.x, m_min.y, m_min.z},
		{m_max.x, m_min.y, m_max.z},
		{m_max.x, m_max.y, m_min.z},
		m_max
	};

	m_transformedMin = glm::vec3(std::numeric_limits<float>::max());
	m_transformedMax = glm::vec3(std::numeric_limits<float>::lowest());

	for (const auto& corner : corners)
	{
		auto transformed = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));

		m_transformedMin = min(m_transformedMin, transformed);
		m_transformedMax = max(m_transformedMax, transformed);
	}

	glm::vec3 min = m_transformedMin;
	glm::vec3 max = m_transformedMax;

	m_lineVertices = {
		{min.x, min.y, min.z}, {max.x, min.y, min.z},
		{max.x, min.y, min.z}, {max.x, max.y, min.z},
		{max.x, max.y, min.z}, {min.x, max.y, min.z},
		{min.x, max.y, min.z}, {min.x, min.y, min.z},

		{min.x, min.y, max.z}, {max.x, min.y, max.z},
		{max.x, min.y, max.z}, {max.x, max.y, max.z},
		{max.x, max.y, max.z}, {min.x, max.y, max.z},
		{min.x, max.y, max.z}, {min.x, min.y, max.z},

		{min.x, min.y, min.z}, {min.x, min.y, max.z},
		{max.x, min.y, min.z}, {max.x, min.y, max.z},
		{max.x, max.y, min.z}, {max.x, max.y, max.z},
		{min.x, max.y, min.z}, {min.x, max.y, max.z}
	};

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, m_lineVertices.size() * sizeof(glm::vec3), m_lineVertices.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
