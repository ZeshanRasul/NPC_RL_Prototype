#include "Crosshair.h"

void Crosshair::LoadMesh()
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	CreateAndUploadVertexBuffer();
	CreateAndUploadIndexBuffer();
	glBindVertexArray(0);
}

bool Crosshair::LoadTexture(std::string textureFilename)
{
	if (!m_texture.LoadTexture(textureFilename, false))
	{
		Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
		return false;
	}
	Logger::Log(1, "%s: Crosshair texture successfully loaded\n", __FUNCTION__, textureFilename);
	return true;
}

void Crosshair::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                           GLuint shadowMapTexture, glm::vec3 camPos)
{
	m_shader->Use();
	m_shader->SetMat4("view", viewMat);
	m_shader->SetMat4("projection", proj);

	m_texture.Bind();
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	m_texture.Unbind();
}

glm::vec2 Crosshair::CalculateCrosshairPosition(glm::vec3 rayEnd, int screenWidth, int screenHeight,
                                                glm::mat4 proj, glm::mat4 view)
{
	glm::vec4 clipSpacePos = proj * view * glm::vec4(rayEnd, 1.0f);
	glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
	auto screenSpacePos = glm::vec2(0.0f);
	screenSpacePos.x = screenWidth * (1.0f - (ndcSpacePos.x + 1.0f) / 2.0f);
	screenSpacePos.y = screenHeight * (1.0f - (ndcSpacePos.y + 1.0f) / 2.0f);

	return screenSpacePos;
}

void Crosshair::DrawCrosshair(glm::vec2 ndcPos, glm::vec3 color)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_shader->Use();
	m_shader->SetVec2("ndcPos", ndcPos.x, ndcPos.y);
	m_shader->SetVec3("color", color);
	m_texture.Bind();
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	m_texture.Unbind();
	glDisable(GL_BLEND);
}

void Crosshair::ComputeAudioWorldTransform()
{
}

void Crosshair::CreateAndUploadVertexBuffer()
{
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_STATIC_DRAW);

	// m_position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);

	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void Crosshair::CreateAndUploadIndexBuffer()
{
	glGenBuffers(1, &m_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);
}
