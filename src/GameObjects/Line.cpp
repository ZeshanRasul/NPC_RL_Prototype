#include "Line.h"

void Line::LoadMesh()
{
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
	CreateAndUploadVertexBuffer();
	glBindVertexArray(0);
}

void Line::DrawLine(glm::mat4 viewMat, glm::mat4 proj, glm::vec3 lineColor)
{
	shader->use();
	shader->setMat4("view", viewMat);
	shader->setMat4("projection", proj);
	shader->setVec3("lineColor", lineColor);

	glBindVertexArray(mVAO);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

void Line::ComputeAudioWorldTransform()
{
}

void Line::CreateAndUploadVertexBuffer()
{
	glBindVertexArray(mVAO);
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void Line::UpdateVertexBuffer(glm::vec3 rayOrigin, glm::vec3 rayEnd)
{
	vertices[0] = rayOrigin.x;
	vertices[1] = rayOrigin.y;
	vertices[2] = rayOrigin.z;
	vertices[3] = rayEnd.x;
	vertices[4] = rayEnd.y;
	vertices[5] = rayEnd.z;

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
