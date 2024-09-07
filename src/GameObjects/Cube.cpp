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

	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	renderAABB(proj, viewMat, modelMat, aabbShader);
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
	aabb->owner = this;
	updateAABB();
	mGameManager->GetPhysicsWorld()->addCollider(GetAABB());

}

void Cube::renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* shader)
{
	glm::vec3 min = aabb->transformedMin;
	glm::vec3 max = aabb->transformedMax;

	std::vector<glm::vec3> lineVertices = {
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

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	aabbShader->use();

	aabbShader->setMat4("projection", proj);
	aabbShader->setMat4("view", viewMat);
	aabbShader->setMat4("model", model);
	aabbShader->setVec3("color", aabbColor);

	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, lineVertices.size());
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}
