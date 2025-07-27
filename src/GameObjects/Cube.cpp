#include "Cube.h"
#include "GameManager.h"

void Cube::LoadMesh()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);
	glBindVertexArray(m_vao);
	CreateAndUploadVertexBuffer();
	glBindVertexArray(0);
	SetUpAABB();
}

bool Cube::LoadTexture(std::string textureFilename, Texture* tex)
{
	if (!tex->LoadTexture(textureFilename, false))
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
	m_uniformBuffer.UploadUboData(matrixData, 0);

	if (shadowMap)
	{
		m_shadowShader->Use();
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}
	else
	{
		m_shader->SetVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);

		m_tex.Bind();
		m_shader->SetInt("albedoMap", 0);
		m_normal.Bind(1);
		m_shader->SetInt("normalMap", 1);
		m_metallic.Bind(2);
		m_shader->SetInt("metallicMap", 2);
		m_roughness.Bind(3);
		m_shader->SetInt("roughnessMap", 3);
		m_ao.Bind(4);
		m_shader->SetInt("aoMap", 4);
		m_emissive.Bind(5);
		m_shader->SetInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->SetInt("shadowMap", 6);
		glBindVertexArray(m_vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		m_tex.Unbind();

#ifdef _DEBUG
		aabb->Render(viewMat, proj, modelMat, aabbColor);
#endif
	}
}

void Cube::CreateAndUploadVertexBuffer()
{
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices), m_indices, GL_STATIC_DRAW);

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
		auto vertex = glm::vec3(m_vertices[i], m_vertices[i + 1], m_vertices[i + 2]);
		verticesVec.push_back(vertex);
	}
	aabb->CalculateAABB(verticesVec);
	aabb->SetShader(aabbShader);
	aabb->SetUpMesh();
	aabb->SetOwner(this);
	aabb->SetIsEnemy(false);
	aabb->SetIsPlayer(false);
	UpdateAabb();
	m_gameManager->GetPhysicsWorld()->AddCollider(GetAABB());
}
