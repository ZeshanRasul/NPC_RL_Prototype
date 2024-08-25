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
	if (!mTexture.loadTexture(textureFilename, false)) {
		Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
		return false;
	}
	Logger::log(1, "%s: Crosshair texture successfully loaded\n", __FUNCTION__,);
	return true;
}

void Crosshair::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
	shader->use();
	shader->setMat4("view", viewMat);
	shader->setMat4("projection", proj);
	
	mTexture.bind();
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	mTexture.unbind();

}

void Crosshair::ComputeAudioWorldTransform() {}

void Crosshair::CreateAndUploadVertexBuffer()
{
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
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
