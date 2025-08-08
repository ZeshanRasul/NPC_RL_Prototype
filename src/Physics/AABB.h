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

	void CalculateAABB(const std::vector<glm::vec3>& vertices);

	void SetShader(Shader* sdr) { m_shader = sdr; }
	void SetUpMesh();
	void Render(glm::mat4 viewMat, glm::mat4 proj, glm::mat4 model, glm::vec3 aabbColor);

	glm::vec3 GetMin() const { return m_min; }
	glm::vec3 GetMax() const { return m_max; }
	glm::vec3 GetCenter() const { return (m_min + m_max) * 0.5f; }
	glm::vec3 getSize() const { return m_max - m_min; }

	void Update(const glm::mat4& modelMatrix);

	glm::vec3 GetTransformedMin() const { return m_transformedMin; }
	glm::vec3 GetTransformedMax() const { return m_transformedMax; }

	GameObject* GetOwner() const { return m_owner; }
	void SetOwner(GameObject* val) { m_owner = val; }
	bool GetIsPlayer() const { return m_isPlayer; }
	void SetIsPlayer(bool val) { m_isPlayer = val; }
	bool GetIsEnemy() const { return m_isEnemy; }
	void SetIsEnemy(bool val) { m_isEnemy = val; }

private:
	glm::vec3 m_transformedMin = glm::vec3(1.0f);
	glm::vec3 m_transformedMax = glm::vec3(1.0f);

	GameObject* m_owner = nullptr;
	bool m_isPlayer = false;
	bool m_isEnemy = false;

	glm::vec3 m_min;
	glm::vec3 m_max;
	GLuint m_vao;
	GLuint m_vbo;

	std::vector<glm::vec3> m_lineVertices;
	Shader* m_shader;
};
