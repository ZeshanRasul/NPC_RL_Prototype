#include "Enemy.h"

void Enemy::Draw(Shader& shader)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::scale(modelMat, scale);
	shader.setMat4("model", modelMat);
	shader.setVec3("objectColor", color);

	model.Draw(shader);
}
