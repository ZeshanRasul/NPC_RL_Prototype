#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "GameObjects/GameObject.h"
#include "Shader.h"

class AABB
{
public:
	AABB();
	AABB(const glm::vec3& min, const glm::vec3& max);

	void calculateAABB(const std::vector<glm::vec3>& vertices);

	void setShader(Shader* sdr) { shader = sdr; }
	void setUpMesh();
	void render(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 model, glm::vec3 aabbColor);

	glm::vec3 getMin() const { return mMin; }
	glm::vec3 getMax() const { return mMax; }
	glm::vec3 getCenter() const { return (mMin + mMax) * 0.5f; }
	glm::vec3 getSize() const { return mMax - mMin; }

	void update(const glm::mat4& modelMatrix);

	glm::vec3 getTransformedMin() const { return transformedMin; }
	glm::vec3 getTransformedMax() const { return transformedMax; }

	glm::vec3 transformedMin = glm::vec3(1.0f);
	glm::vec3 transformedMax = glm::vec3(1.0f);
	GameObject* owner = nullptr;

	bool isPlayer = false;
	bool isEnemy = false;

private:
	glm::vec3 mMin;
	glm::vec3 mMax;
	GLuint VAO;
	GLuint VBO;

	std::vector<glm::vec3> lineVertices;
	Shader* shader;
};
