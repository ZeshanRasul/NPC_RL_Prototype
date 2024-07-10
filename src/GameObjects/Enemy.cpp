#include "Enemy.h"

void Enemy::Draw(Shader& shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::scale(model, scale);
	shader.setMat4("model", model);
	shader.setVec3("objectColor", color);
}
