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
	if (!tex->loadTexture(textureFilename, false))
	{
		Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
		return false;
	}
	Logger::Log(1, "%s: Cube texture successfully loaded\n", __FUNCTION__, textureFilename);
	return true;
}

void Cube::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                      GLuint shadowMapTexture, glm::vec3 camPos)
{
	auto modelMat = glm::mat4(1.0f);
	modelMat = translate(modelMat, m_position);
	//	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, m_scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	m_uniformBuffer.uploadUboData(matrixData, 0);

	if (shadowMap)
	{
		m_shadowShader->use();
		glBindVertexArray(mVAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}
	else
	{
		m_shader->setVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);

		m_tex.bind();
		m_shader->setInt("albedoMap", 0);
		mNormal.bind(1);
		m_shader->setInt("normalMap", 1);
		mMetallic.bind(2);
		m_shader->setInt("metallicMap", 2);
		mRoughness.bind(3);
		m_shader->setInt("roughnessMap", 3);
		mAO.bind(4);
		m_shader->setInt("aoMap", 4);
		mEmissive.bind(5);
		m_shader->setInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->setInt("shadowMap", 6);
		glBindVertexArray(mVAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		m_tex.unbind();

#ifdef _DEBUG
		m_aabb->Render(viewMat, proj, modelMat, m_aabbColor);
#endif
	}
}

void Cube::CreateAndUploadVertexBuffer()
{
	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), static_cast<void*>(nullptr));
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
		auto vertex = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
		verticesVec.push_back(vertex);
	}
	aabb->CalculateAABB(verticesVec);
	aabb->SetShader(aabbShader);
	aabb->SetUpMesh();
	aabb->SetOwner(this);
	aabb->SetIsEnemy(false);
	aabb->SetIsPlayer(false);
	updateAABB();
	m_gameManager->GetPhysicsWorld()->AddCollider(GetAABB());
}
