#include "Enemy.h"
#include "src/Pathfinding/Grid.h"

void Enemy::drawObject() const
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::rotate(modelMat, glm::radians(-Yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	shader->setMat4("model", modelMat);

    model->uploadVertexBuffers();
    model->uploadPositionBuffer();
    model->draw();
}

void Enemy::Update(float dt, Player& player)
{
    float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());

    if (playerEnemyDistance < 35.0f)
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

            //if (path.empty()) {
            //    std::cerr << "No path found" << std::endl;
            //}
            //else {
            //    std::cout << "Path found: ";
            //    for (const auto& step : path) {
            //        std::cout << "(" << step.x << ", " << step.y << ") ";
            //    }
            //    std::cout << std::endl;
            //}

            moveEnemy(path, dt);
        }
        else
        {
            currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

            std::vector<glm::ivec2> path = findPath(
                glm::ivec2(getPosition().x / CELL_SIZE, getPosition().z / CELL_SIZE),
                glm::ivec2(currentWaypoint.x / CELL_SIZE, currentWaypoint.z / CELL_SIZE),
                grid
            );

            //std::cout << "Finding new waypoint destination" << std::endl;

            reachedDestination = false;

            moveEnemy(path, dt);
        }

        break;
    }
    case ATTACK:
    {
        std::vector<glm::ivec2> path = findPath(
            glm::ivec2(getPosition().x / CELL_SIZE, getPosition().z / CELL_SIZE),
            glm::ivec2(player.getPosition().x / CELL_SIZE, player.getPosition().z / CELL_SIZE),
            grid
        );

        std::cout << "Moving to Player destination" << std::endl;

        moveEnemy(path, dt);

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

void Enemy::moveEnemy(const std::vector<glm::ivec2>& path, float deltaTime) {
    static size_t pathIndex = 0;
    if (path.empty()) return;

    const float tolerance = 0.1f; // Smaller tolerance for better alignment
    const float agentRadius = 0.5f; // Adjust this value to match the agent's radius
    float speed = 10.0f; // Ensure this speed is appropriate for the grid size and cell size

    if (pathIndex >= path.size()) {
        std::cout << "Agent has reached its destination." << std::endl;
        reachedDestination = true;
        return; // Stop moving if the agent has reached its destination
    }

    float cellOffset = 0.0f;

    if (pathIndex < path.size())
    {
        cellOffset = CELL_SIZE / 2.0f;
    }

    // Calculate the target position from the current path node
    glm::vec3 targetPos = glm::vec3(path[pathIndex].x * CELL_SIZE + CELL_SIZE / 2.0f, getPosition().y, path[pathIndex].y * CELL_SIZE + CELL_SIZE / 2.0f);

    // Calculate the direction to the target position
    glm::vec3 direction = glm::normalize(targetPos - getPosition());

    //enemy.Yaw = glm::degrees(glm::acos(glm::dot(glm::normalize(enemy.Front), direction)));
    Yaw = glm::degrees(glm::atan(direction.z, direction.x));

    UpdateEnemyVectors();

    // Calculate the new position
    glm::vec3 newPos = getPosition() + direction * speed * deltaTime;

    // Ensure the new position is not within an obstacle by checking the bounding box
    bool isObstacleFree = true;
    for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
        for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
            glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / CELL_SIZE, (newPos.z + zOffset) / CELL_SIZE);
            if (checkPos.x < 0 || checkPos.x >= GRID_SIZE || checkPos.y < 0 || checkPos.y >= GRID_SIZE || grid[checkPos.x][checkPos.y].isObstacle) {
                isObstacleFree = false;
                break;
            }
        }
        if (!isObstacleFree) break;
    }

    if (isObstacleFree) {
        setPosition(newPos);
    }
    else {
        // If the new position is within an obstacle, try to adjust the position slightly
        newPos = getPosition() + direction * (speed * deltaTime * 0.5f);
        isObstacleFree = true;
        for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
            for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
                glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / CELL_SIZE, (newPos.z + zOffset) / CELL_SIZE);
                if (checkPos.x < 0 || checkPos.x >= GRID_SIZE || checkPos.y < 0 || checkPos.y >= GRID_SIZE || grid[checkPos.x][checkPos.y].isObstacle) {
                    isObstacleFree = false;
                    break;
                }
            }
            if (!isObstacleFree) break;
        }

        if (isObstacleFree) {
            setPosition(newPos);
        }
    }

    // Check if the enemy has reached the current target position within a tolerance
    if (glm::distance(getPosition(), targetPos) < tolerance) {
        pathIndex++;
        if (pathIndex >= path.size()) {
            pathIndex = 0; // Reset path index if the end is reached
        }
    }
}