#include "Crosshair.h"

void Crosshair::LoadMesh()
{
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
	CreateAndUploadVertexBuffer();
	CreateAndUploadIndexBuffer();
	glBindVertexArray(0);
}

bool Crosshair::LoadTexture(std::string textureFilename)
{
	if (!mTexture.loadTexture(textureFilename, false))
	{
		Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
		return false;
	}
	Logger::log(1, "%s: Crosshair texture successfully loaded\n", __FUNCTION__, textureFilename);
	return true;
}

void Crosshair::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                           GLuint shadowMapTexture, glm::vec3 camPos)
{
	m_shader->use();
	m_shader->setMat4("view", viewMat);
	m_shader->setMat4("projection", proj);

	mTexture.bind();
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	mTexture.unbind();
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
	m_shader->use();
	m_shader->setVec2("ndcPos", ndcPos.x, ndcPos.y);
	m_shader->setVec3("color", color);
	mTexture.bind();
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	mTexture.unbind();
	glDisable(GL_BLEND);
}

void Crosshair::ComputeAudioWorldTransform()
{
}

void Crosshair::CreateAndUploadVertexBuffer()
{
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// m_position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), static_cast<void*>(nullptr));
	glEnableVertexAttribArray(0);

	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void Crosshair::CreateAndUploadIndexBuffer()
{
	glGenBuffers(1, &mEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}
