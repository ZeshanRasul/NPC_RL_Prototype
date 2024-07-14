#include "Enemy.h"

void Enemy::Draw(Shader& shader)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	shader.setMat4("model", modelMat);
	shader.setVec3("objectColor", color);

	model.Draw(shader);
}

void Enemy::UpdateEnemyCameraVectors()
{
	glm::vec3 front = glm::vec3(1.0f);
	front.x = cos(glm::radians(EnemyCameraYaw));
	front.y = 0.0f;
	front.z = sin(glm::radians(EnemyCameraYaw));
	EnemyFront = glm::normalize(front);
	EnemyRight = glm::normalize(glm::cross(EnemyFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	EnemyUp = glm::normalize(glm::cross(EnemyRight, EnemyFront));
}

void Enemy::EnemyProcessMouseMovement(float xOffset)
{
	//    xOffset *= SENSITIVITY;  

	EnemyCameraYaw += xOffset;

	UpdateEnemyCameraVectors();
}
