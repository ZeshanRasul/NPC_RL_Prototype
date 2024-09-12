#include "Enemy.h"
#include "src/Pathfinding/Grid.h"
#include "GameManager.h"
#include <random>

void Enemy::drawObject(glm::mat4 viewMat, glm::mat4 proj)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::rotate(modelMat, glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
    std::vector<glm::mat4> matrixData;
    matrixData.push_back(viewMat);
    matrixData.push_back(proj);
    matrixData.push_back(modelMat);
    mUniformBuffer.uploadUboData(matrixData, 0);

    GetShader()->use();
    mEnemyDualQuatSSBuffer.uploadSsboData(model->getJointDualQuats(), 2);


  //  if (uploadVertexBuffer)
  //  {
  //      model->uploadVertexBuffers();
		//aabb.calculateAABB(model->getVertices());
  //      aabb.owner = this;
		//aabb.mModelMatrix = modelMat;
  //      updateAABB();
  //      GameManager* gameMgr = GetGameManager();
  //      gameMgr->GetPhysicsWorld()->addCollider(GetAABB());
  //      gameMgr->GetPhysicsWorld()->addEnemyCollider(GetAABB());

		//uploadVertexBuffer = false;
  //  }

    model->draw();
	renderAABB(proj, viewMat, modelMat, aabbShader);
}

void Enemy::Update(float dt, Player& player, float blendFactor, bool playAnimBackwards)
{    
    if (GetEnemyState() == TAKE_DAMAGE)
	    damageTimer -= dt;

    if (GetEnemyState() == DYING)
        dyingTimer -= dt;
	
    if (GetEnemyState() == TAKING_COVER)
        coverTimer -= dt;

    float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());

    if (playerEnemyDistance < 35.0f && enemyShootCooldown > 0.0f && GetEnemyState() != TAKE_DAMAGE && GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && GetEnemyState() != SEEKING_COVER)
    {
        SetEnemyState(ATTACK);
    }
    else if (playerEnemyDistance < 35.0f && enemyShootCooldown <= 0.0f && GetEnemyState() != TAKE_DAMAGE && GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && GetEnemyState() != SEEKING_COVER)
    {
        SetEnemyState(ENEMY_SHOOTING);
    }
	else if (GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && health < 51.0f && !reachedCover)
	{
		SetEnemyState(SEEKING_COVER);
	}
	else if (GetEnemyState() != TAKE_DAMAGE && GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && GetEnemyState() != SEEKING_COVER)
    {
        SetEnemyState(PATROL);
        enemyHasShot = false;
    } 

    switch (GetEnemyState())
    {
    case PATROL:
    {
        if (reachedDestination == false)
        {
            std::vector<glm::ivec2> path = grid->findPath(
                glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
                glm::ivec2(currentWaypoint.x / grid->GetCellSize(), currentWaypoint.z / grid->GetCellSize()),
				grid->GetGrid()
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

            moveEnemy(path, dt, blendFactor, playAnimBackwards);
        }
        else
        {
            currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

            std::vector<glm::ivec2> path = grid->findPath(
                glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
                glm::ivec2(currentWaypoint.x / grid->GetCellSize(), currentWaypoint.z / grid->GetCellSize()),
                grid->GetGrid()
            );

            //std::cout << "Finding new waypoint destination" << std::endl;

            reachedDestination = false;

            moveEnemy(path, dt, blendFactor, playAnimBackwards);
        }

        break;
    }
    case ATTACK:
    {
        if (enemyShootCooldown <= 0.0f)
		{
			SetEnemyState(ENEMY_SHOOTING);
		}
		else
		{
			enemyShootCooldown -= dt;
            enemyRayDebugRenderTimer -= dt;
		}
        break;
    }
    case ENEMY_SHOOTING:
    {
        Shoot(player);
        SetAnimNum(5);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
        enemyShootCooldown = 0.3f;
        enemyRayDebugRenderTimer = 0.3f;
        enemyHasShot = true;
		SetEnemyState(ATTACK);
		break;
    }
    case TAKE_DAMAGE:
    {
        SetAnimNum(8);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
        if (damageTimer <= 0.0f)
        {
            state = PATROL;
        }
        break;
    }
    case SEEKING_COVER:
    {
		cover = ScoreCoverLocations(player);
        SetEnemyState(TAKING_COVER);
        break;
    }
	case TAKING_COVER:
	{
		std::vector<glm::ivec2> path = grid->findPath(
			glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
			glm::ivec2(cover.worldPosition.x / grid->GetCellSize(), cover.worldPosition.z / grid->GetCellSize()),
			grid->GetGrid()
		);

		moveEnemy(path, dt, blendFactor, playAnimBackwards);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);

        if (coverTimer <= 0.0f && reachedCover)
        {
            inCover = false;
            SetEnemyState(PATROL);
        }

		break;
	}
    case DYING:
	{
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
		if (dyingTimer <= 0.0f)
		{
			SetEnemyState(DEAD);
		}
		break;
	}
    case DEAD:
    {
        isDestroyed = true;
        return;
    }
    default:
        break;
    }

    updateAABB();
    ComputeAudioWorldTransform();
    UpdateComponents(dt);
}

void Enemy::ComputeAudioWorldTransform()
{
    if (mRecomputeWorldTransform)
    {
        mRecomputeWorldTransform = false;
        glm::mat4 worldTransform = glm::mat4(1.0f);
        // Scale, then rotate, then translate
        audioWorldTransform = glm::translate(worldTransform, position);
        audioWorldTransform = glm::rotate(worldTransform, glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        audioWorldTransform = glm::scale(worldTransform, scale);

        // Inform components world transform updated
        for (auto comp : mComponents)
        {
            comp->OnUpdateWorldTransform();
        }
    }
};

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
	front.x = glm::cos(glm::radians(yaw));
	front.y = 0.0f;
	front.z = glm::sin(glm::radians(yaw));
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

void Enemy::moveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards) {
    static size_t pathIndex = 0;
    if (path.empty()) return;

    const float tolerance = 0.1f; // Smaller tolerance for better alignment
    const float agentRadius = 0.5f; // Adjust this value to match the agent's radius
    float speed = 10.0f; // Ensure this speed is appropriate for the grid size and cell size
    
    if (!reachedPlayer && !inCover)
    {
        SetAnimNum(0);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
    }

    if (pathIndex >= path.size()) {
        std::cout << "Agent has reached its destination." << std::endl;
		if (state == PATROL)
		{
			reachedDestination = true;
		}
		else if (state == ATTACK)
		{
			reachedPlayer = true;
		} 
		else if (state == TAKING_COVER && !reachedCover)
		{
			reachedCover = true;
			inCover = true;
            coverTimer = 6.4f;
            SetAnimNum(4);
            SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
		}
		
        return; // Stop moving if the agent has reached its destination
    }

    float cellOffset = 0.0f;

    if (pathIndex < path.size())
    {
        cellOffset = grid->GetCellSize() / 2.0f;
    }

    // Calculate the target position from the current path node
    glm::vec3 targetPos = glm::vec3(path[pathIndex].x * grid->GetCellSize() + grid->GetCellSize() / 2.0f, getPosition().y, path[pathIndex].y * grid->GetCellSize() + grid->GetCellSize() / 2.0f);

    // Calculate the direction to the target position
    glm::vec3 direction = glm::normalize(targetPos - getPosition());

    //enemy.Yaw = glm::degrees(glm::acos(glm::dot(glm::normalize(enemy.Front), direction)));
    yaw = glm::degrees(glm::atan(direction.z, direction.x));
	mRecomputeWorldTransform = true;

    UpdateEnemyVectors();

    // Calculate the new position
    glm::vec3 newPos = getPosition() + direction * speed * deltaTime;

    // Ensure the new position is not within an obstacle by checking the bounding box
    bool isObstacleFree = true;
    for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
        for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
            glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid->GetCellSize(), (newPos.z + zOffset) / grid->GetCellSize());
            if (checkPos.x < 0 || checkPos.x >= grid->GetCellSize() || checkPos.y < 0 || checkPos.y >= grid->GetGridSize() || grid->GetGrid()[checkPos.x][checkPos.y].IsObstacle()) {
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
                glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid->GetCellSize(), (newPos.z + zOffset) / grid->GetCellSize());
                if (checkPos.x < 0 || checkPos.x >= grid->GetCellSize() || checkPos.y < 0 || checkPos.y >= grid->GetCellSize() || grid->GetGrid()[checkPos.x][checkPos.y].IsObstacle()) {
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

void Enemy::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
    model->playAnimation(animNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::Shoot(Player& player)
{
    if (GetEnemyState() != ENEMY_SHOOTING)
        return;

	glm::vec3 accuracyOffset = glm::vec3(0.0f);
	glm::vec3 accuracyOffsetFactor = glm::vec3(1.0f);

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist100(0, 100); // distribution in range [1, 100]

	if (dist100(rng) < 60)
	{
		accuracyOffset = accuracyOffset + (accuracyOffsetFactor * (float)dist100(rng));
	}

	enemyShootPos = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
    enemyShootDir = (player.getPosition() - getPosition()) + accuracyOffset;
	glm::vec3 hitPoint = glm::vec3(0.0f);

    yaw = glm::degrees(glm::atan(enemyShootDir.z, enemyShootDir.x));
    UpdateEnemyVectors();

    bool hit = false;
    hit = GetGameManager()->GetPhysicsWorld()->rayIntersect(enemyShootPos, enemyShootDir, hitPoint);

    if (hit) {
        std::cout << "\nRay hit at: " << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << std::endl;
    }
    else {
        std::cout << "\nNo hit detected." << std::endl;
    }

    shootAC->PlayEvent("event:/EnemyShoot");
}

void Enemy::OnDeath()
{
	std::cout << "Enemy Died!" << std::endl;
    SetEnemyState(DYING);
    SetAnimNum(3);
    deathAC->PlayEvent("event:/EnemyDeath");
	dyingTimer = 1.5f;
}

void Enemy::SetUpAABB()
{
    aabb = new AABB();
    aabb->calculateAABB(model->getVertices());
    aabb->owner = this;
    updateAABB();
    //class GameManager* gameMgr = GetGameManager();
    mGameManager->GetPhysicsWorld()->addCollider(GetAABB());
    mGameManager->GetPhysicsWorld()->addEnemyCollider(GetAABB());
}

void Enemy::renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* shader)
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

void Enemy::OnHit()
{
	Logger::log(1, "Enemy was hit!\n", __FUNCTION__);
    SetEnemyState(TAKE_DAMAGE);
	setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
    SetAnimNum(8);
    TakeDamage(50.0f);
    if (GetEnemyState() != DYING)
        takeDamageAC->PlayEvent("event:/EnemyTakeDamage");
	damageTimer = model->getAnimationEndTime(8);
}

Grid::Cover& Enemy::ScoreCoverLocations(Player& player)
{
    float bestScore = -100000.0f;

	Grid::Cover bestCover;

	for (Grid::Cover& cover : grid->GetCoverLocations())
	{
        float score = 0.0f; 
		
        float distanceToPlayer = glm::distance(cover.worldPosition, player.getPosition());
        score += distanceToPlayer * 0.5f;

		float distanceToEnemy = glm::distance(cover.worldPosition, getPosition());
		score -= (1.0f / distanceToEnemy + 1.0f) * 1.0f;

		glm::vec3 rayOrigin = cover.worldPosition + glm::vec3(0.0f, 2.5f, 0.0f);
		glm::vec3 rayDirection = glm::normalize(player.getPosition() - rayOrigin);
        glm::vec3 hitPoint = glm::vec3(0.0f);

        bool visibleToPlayer = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(rayOrigin, rayDirection, hitPoint);
        if (!visibleToPlayer) {
            score += 20.0f;
        }

		if (score > bestScore) {
			bestScore = score;
			bestCover = cover;
		}
	}

    return bestCover;
}
