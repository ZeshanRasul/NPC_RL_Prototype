#include "Ground.h"

void Ground::Draw(Shader& shader)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
//	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	shader.setMat4("model", modelMat);
	shader.setVec3("objectColor", color);

	model.Draw(shader);
}