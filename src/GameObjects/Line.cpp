#include "Line.h"

void Line::LoadMesh()
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	CreateAndUploadVertexBuffer();
	glBindVertexArray(0);
}

void Line::DrawLine(glm::mat4 viewMat, glm::mat4 proj, glm::vec3 lineColor, glm::mat4 lightSpaceMat,
                    GLuint shadowMapTexture, bool shadowMap, float lifetime)
{
	if (shadowMap)
	{
		m_shadowShader->Use();
		m_shadowShader->SetMat4("view", viewMat);
		m_shadowShader->SetMat4("projection", proj);
		m_shadowShader->SetVec3("lineColor", lineColor);
		m_shadowShader->SetFloat("lifetime", lifetime);
	}
	else
	{
		m_shader->Use();
		m_shader->SetMat4("view", viewMat);
		m_shader->SetMat4("projection", proj);
		m_shader->SetVec3("lineColor", lineColor);
		m_shader->SetMat4("lightSpaceMatrix", lightSpaceMat);
		m_shader->SetFloat("lifetime", lifetime);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->SetInt("shadowMap", 0);
	}

	glBindVertexArray(m_vao);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

void Line::ComputeAudioWorldTransform()
{
}

void Line::CreateAndUploadVertexBuffer()
{
	glBindVertexArray(m_vao);
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_DYNAMIC_DRAW);

	// m_position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void Line::UpdateVertexBuffer(glm::vec3 rayOrigin, glm::vec3 rayEnd)
{
	m_vertices[0] = rayOrigin.x;
	m_vertices[1] = rayOrigin.y;
	m_vertices[2] = rayOrigin.z;
	m_vertices[3] = rayEnd.x;
	m_vertices[4] = rayEnd.y;
	m_vertices[5] = rayEnd.z;

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_vertices), m_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
