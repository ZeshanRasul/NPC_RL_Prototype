#include "AABB.h"
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

AABB::AABB()
	: mMin(std::numeric_limits<float>::max()), mMax(std::numeric_limits<float>::lowest())
{
}

AABB::AABB(const glm::vec3& min, const glm::vec3& max)
	: mMin(min), mMax(max)
{
}

void AABB::calculateAABB(const std::vector<glm::vec3>& vertices)
{
	for (const auto& vertex : vertices)
	{
		mMin = min(mMin, vertex);
		mMax = max(mMax, vertex);
	}
}

void AABB::setUpMesh()
{
	glm::vec3 min = transformedMin;
	glm::vec3 max = transformedMax;

	lineVertices = {
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

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void*>(nullptr));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void AABB::render(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 model, glm::vec3 aabbColor)
{
	shader->use();

	shader->setMat4("projection", proj);
	shader->setMat4("view", viewMat);
	shader->setMat4("m_model", model);
	shader->setVec3("color", aabbColor);

	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size()));
	glBindVertexArray(0);
}

void AABB::update(const glm::mat4& modelMatrix)
{
	std::vector<glm::vec3> corners = {
		mMin,
		{mMin.x, mMin.y, mMax.z},
		{mMin.x, mMax.y, mMin.z},
		{mMin.x, mMax.y, mMax.z},
		{mMax.x, mMin.y, mMin.z},
		{mMax.x, mMin.y, mMax.z},
		{mMax.x, mMax.y, mMin.z},
		mMax
	};

	transformedMin = glm::vec3(std::numeric_limits<float>::max());
	transformedMax = glm::vec3(std::numeric_limits<float>::lowest());

	for (const auto& corner : corners)
	{
		auto transformed = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));

		transformedMin = min(transformedMin, transformed);
		transformedMax = max(transformedMax, transformed);
	}

	glm::vec3 min = transformedMin;
	glm::vec3 max = transformedMax;

	lineVertices = {
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

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
