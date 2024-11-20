#include "Cube.h"
#include "GameManager.h"

void Cube::LoadMesh()
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mEBO);
	glBindVertexArray(mVAO);
	CreateAndUploadVertexBuffer();
	glBindVertexArray(0);
	SetUpAABB();
}

bool Cube::LoadTexture(std::string textureFilename, Texture* tex)
{
	if (!tex->loadTexture(textureFilename, false)) {
		Logger::log(1, "%s: texture loading failed\n", __FUNCTION__);
		return false;
	}
	Logger::log(1, "%s: Crosshair texture successfully loaded\n", __FUNCTION__, textureFilename);
	return true;
}

void Cube::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	//	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	mUniformBuffer.uploadUboData(matrixData, 0);

	mTex.bind();
	shader->setInt("albedoMap", 0);
	mNormal.bind(1);
	shader->setInt("normalMap", 1);
	mMetallic.bind(2);
	shader->setInt("metallicMap", 2);
	mRoughness.bind(3);
	shader->setInt("roughnessMap", 3);
	mAO.bind(4);
	shader->setInt("aoMap", 4);
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	mTex.unbind();
	aabb->render(viewMat, proj, modelMat, aabbColor);
}

void Cube::CreateAndUploadVertexBuffer()
{
	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
}

void Cube::SetUpAABB()
{
	aabb = new AABB();

	std::vector<glm::vec3> verticesVec;
	for (int i = 0; i < 192; i = i + 8)
	{
		glm::vec3 vertex = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
		verticesVec.push_back(vertex);
	}
	aabb->calculateAABB(verticesVec);
	aabb->setShader(aabbShader);
	aabb->setUpMesh();
	aabb->owner = this;
	updateAABB();
	mGameManager->GetPhysicsWorld()->addCollider(GetAABB());

}