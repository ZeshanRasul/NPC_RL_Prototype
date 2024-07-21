#include "Enemy.h"
#include "src/Pathfinding/Grid.h"

void Enemy::drawObject() const
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::rotate(modelMat, glm::radians(-Yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	shader->setMat4("model", modelMat);

	// Draw TODO: Update for GLTF
	//model.Draw(shader);
}

void Enemy::Update(float dt, Player& player)
{
    float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());

    if (playerEnemyDistance < 15.0f)
    {
        SetEnemyState(ATTACK);
    }
    else
    {
        SetEnemyState(PATROL);
    }

    switch (GetEnemyState())
    {
    case PATROL:
    {
        if (reachedDestination == false)
        {
            std::vector<glm::ivec2> path = findPath(
                glm::ivec2(getPosition().x / CELL_SIZE, getPosition().z / CELL_SIZE),
                glm::ivec2(currentWaypoint.x / CELL_SIZE, currentWaypoint.z / CELL_SIZE),
                grid
            );

            if (path.empty()) {
                std::cerr << "No path found" << std::endl;
            }
            else {
                std::cout << "Path found: ";
                for (const auto& step : path) {
                    std::cout << "(" << step.x << ", " << step.y << ") ";
                }
                std::cout << std::endl;
            }

            moveEnemy(*this, path, dt);
        }
        else
        {
            currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

            std::vector<glm::ivec2> path = findPath(
                glm::ivec2(getPosition().x / CELL_SIZE, getPosition().z / CELL_SIZE),
                glm::ivec2(currentWaypoint.x / CELL_SIZE, currentWaypoint.z / CELL_SIZE),
                grid
            );

            std::cout << "Finding new waypoint destination" << std::endl;

            reachedDestination = false;

            moveEnemy(*this, path, dt);
        }

        break;
    }
    case ATTACK:
    {
        std::vector<glm::ivec2> path = findPath(
            glm::ivec2(getPosition().x / CELL_SIZE, getPosition().z / CELL_SIZE),
            glm::ivec2(getPosition().x / CELL_SIZE, player.getPosition().z / CELL_SIZE),
            grid
        );

        std::cout << "Moving to Player destination" << std::endl;

        moveEnemy(*this, path, dt);

        break;
    }
    default:
        break;
    }
}

void Enemy::UpdateEnemyCameraVectors()
{
	glm::vec3 front = glm::vec3(1.0f);
	front.x = glm::cos(glm::radians(EnemyCameraYaw)) * glm::cos(glm::radians(EnemyCameraPitch));
	front.y = glm::sin(glm::radians(EnemyCameraPitch));
	front.z = glm::sin(glm::radians(EnemyCameraYaw)) * glm::cos(glm::radians(EnemyCameraPitch));
	EnemyFront = glm::normalize(front);
	EnemyRight = glm::normalize(glm::cross(EnemyFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	EnemyUp = glm::normalize(glm::cross(EnemyRight, EnemyFront));
}

void Enemy::UpdateEnemyVectors()
{
	glm::vec3 front = glm::vec3(1.0f);
	front.x = glm::cos(glm::radians(Yaw));
	front.y = 0.0f;
	front.z = glm::sin(glm::radians(Yaw));
	Front = glm::normalize(front);
	Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));
	Up = glm::normalize(glm::cross(Right, Front));

}

void Enemy::EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
	// TODO: Update this
	//    xOffset *= SENSITIVITY;  

	EnemyCameraYaw += xOffset;
	EnemyCameraPitch += yOffset;

	if (constrainPitch)
	{
		if (EnemyCameraPitch > 13.0f)
			EnemyCameraPitch = 13.0f;
		if (EnemyCameraPitch < -89.0f)
			EnemyCameraPitch = -89.0f;
	}

	UpdateEnemyCameraVectors();
}
