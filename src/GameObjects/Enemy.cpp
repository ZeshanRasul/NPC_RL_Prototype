#include "Enemy.h"
#include "src/Pathfinding/Grid.h"
#include "GameManager.h"
#include <random>

void Enemy::SetUpModel()
{
    if (uploadVertexBuffer)
    {
        model->uploadVertexBuffers();
        //SetUpAABB();
        uploadVertexBuffer = false;
    }

    model->uploadIndexBuffer();
    Logger::log(1, "%s: glTF model '%s' succesfully loaded\n", __FUNCTION__, model->filename.c_str());


    size_t enemyModelJointDualQuatBufferSize = model->getJointDualQuatsSize() *
        sizeof(glm::mat2x4);
    mEnemyDualQuatSSBuffer.init(enemyModelJointDualQuatBufferSize);
    Logger::log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, enemyModelJointDualQuatBufferSize);
}

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

    model->draw(mTex);
	aabb->render(viewMat, proj, modelMat, aabbColor);
}

void Enemy::OldUpdate(float dt, Player& player, float blendFactor, bool playAnimBackwards)
{    
    if (GetEnemyState() == TAKE_DAMAGE)
	    damageTimer -= dt;

    if (GetEnemyState() == DYING)
        dyingTimer -= dt;
	
    if (dyingTimer <= 0.0f && (GetEnemyState() == DYING || GetEnemyState() == DEAD))
    {
        isDestroyed = true;
        GetGameManager()->GetPhysicsWorld()->removeCollider(GetAABB());
        GetGameManager()->GetPhysicsWorld()->removeEnemyCollider(GetAABB());
        return;
    }

    if (GetEnemyState() == TAKING_COVER)
        coverTimer -= dt;

    float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());

    glm::vec3 tempEnemyShootPos = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
    glm::vec3 tempEnemyShootDir = glm::normalize(player.getPosition() - getPosition());
    glm::vec3 hitPoint = glm::vec3(0.0f);

    if (playerEnemyDistance < 35.0f && enemyShootCooldown > 0.0f && GetEnemyState() != TAKE_DAMAGE && GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && GetEnemyState() != SEEKING_COVER)
    {
        playerIsVisible = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(tempEnemyShootPos, tempEnemyShootDir, hitPoint, aabb);
        if (playerIsVisible)
        {
            SetEnemyState(ATTACK);
        }
        else
        {
            SetEnemyState(PATROL);
        }
    }
    else if (playerEnemyDistance < 35.0f && enemyShootCooldown <= 0.0f && GetEnemyState() != TAKE_DAMAGE && GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && GetEnemyState() != SEEKING_COVER)
    {
        playerIsVisible = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(tempEnemyShootPos, tempEnemyShootDir, hitPoint, aabb);
        if (playerIsVisible)
        {
            SetEnemyState(ENEMY_SHOOTING);
        } 
        else
        {
            SetEnemyState(PATROL);
        }
    }
	else if (GetEnemyState() != DYING && GetEnemyState() != DEAD && GetEnemyState() != TAKING_COVER && health < 51.0f && !reachedCover && damageTimer <= 0.0f)
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
        Shoot();
        SetAnimNum(3);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
        enemyShootCooldown = 0.3f;
        enemyRayDebugRenderTimer = 0.3f;
        enemyHasShot = true;
		SetEnemyState(ATTACK);
		break;
    }
    case TAKE_DAMAGE:
    {
        SetAnimNum(4);
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
        return;
    }
    default:
        break;
    }

    updateAABB();
    ComputeAudioWorldTransform();
    UpdateComponents(dt);
}

void Enemy::Update()
{
    if (!isDead_)
    {
        float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());
        if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
        {
            DetectPlayer();
        }

        behaviorTree_->Tick();


		if (enemyHasShot)
		{
			enemyRayDebugRenderTimer -= dt;
            enemyShootCooldown -= dt;
		}
		if (enemyShootCooldown <= 0.0f)
		{
			enemyHasShot = false;
		}

        if (isDestroyed)
        {
            GetGameManager()->GetPhysicsWorld()->removeCollider(GetAABB());
            GetGameManager()->GetPhysicsWorld()->removeEnemyCollider(GetAABB());
        }


		SetAnimation(GetAnimNum(), 1.0f, 1.0f, false);
    }
}

void Enemy::OnEvent(const Event& event)
{
    if (const PlayerDetectedEvent* e = dynamic_cast<const PlayerDetectedEvent*>(&event))
    {
        if (e->npcID != id_)
        {
            isPlayerDetected_ = true;
        }
    }
    else if (const NPCDamagedEvent* e = dynamic_cast<const NPCDamagedEvent*>(&event))
    {
        if (e->npcID != id_)
        {
            if (isInCover_)
            {
                // Come out of cover and provide suppression fire
                isInCover_ = false;
                isSeekingCover_ = false;
                isTakingCover_ = false;
                provideSuppressionFire = true;
            }
        }
    }

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
    float speed = 5.0f; // Ensure this speed is appropriate for the grid size and cell size
    
    if (!reachedPlayer && !inCover)
    {
        SetAnimNum(2);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
    }

    if (pathIndex >= path.size()) {
        std::cout << "Agent has reached its destination." << std::endl;
        
        if (IsPatrolling())
            reachedDestination = true;

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
            SetAnimNum(3);
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

void Enemy::Shoot()
{
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
    hit = GetGameManager()->GetPhysicsWorld()->rayPlayerIntersect(enemyShootPos, enemyShootDir, hitPoint, aabb);

    if (hit) {
        std::cout << "\nRay hit at: " << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << std::endl;
    }
    else {
        std::cout << "\nNo hit detected." << std::endl;
    }

    shootAC->PlayEvent("event:/EnemyShoot");
    enemyRayDebugRenderTimer = 0.3f;
    enemyHasShot = true;
    enemyShootCooldown = 0.3f;
}

void Enemy::OnDeath()
{
	std::cout << "Enemy Died!" << std::endl;
    SetEnemyState(DYING);
    SetAnimNum(1);
    deathAC->PlayEvent("event:/EnemyDeath");
	dyingTimer = 1.5f;
}

void Enemy::SetUpAABB()
{
    aabb = new AABB();
    aabb->calculateAABB(model->getVertices());
    aabb->setShader(aabbShader);
    updateAABB();
    aabb->setUpMesh();
    aabb->owner = this;
	aabb->isEnemy = true;
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
    SetAnimNum(4);
    TakeDamage(82.0f);
    isTakingDamage_ = true;
    if (GetEnemyState() != DYING)
        takeDamageAC->PlayEvent("event:/EnemyTakeDamage");
	damageTimer = model->getAnimationEndTime(4);
	std::cout << "Damage Timer: " << damageTimer << std::endl;
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

        bool visibleToPlayer = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(rayOrigin, rayDirection, hitPoint, aabb);
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

void Enemy::BuildBehaviorTree()
{
    // Top-level Selector
    auto root = std::make_shared<SelectorNode>();

    // Dead check
    auto isDeadCondition = std::make_shared<ConditionNode>([this]() { return IsDead(); });
    auto deadAction = std::make_shared<ActionNode>([]() { return NodeStatus::Success; });

    auto deadSequence = std::make_shared<SequenceNode>();
    deadSequence->AddChild(isDeadCondition);
    deadSequence->AddChild(deadAction);

    // Dying sequence
    auto dyingSequence = std::make_shared<SequenceNode>();
    dyingSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsHealthZeroOrBelow(); }));
    dyingSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterDyingState(); }));

    // Taking Damage sequence
    auto takingDamageSequence = std::make_shared<SequenceNode>();
    takingDamageSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsTakingDamage(); }));
    takingDamageSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterTakingDamageState(); }));

    // Attack Selector
    auto attackSelector = std::make_shared<SelectorNode>();

    // Player detected sequence
    auto playerDetectedSequence = std::make_shared<SequenceNode>();
    playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsPlayerDetected(); }));
 
    // Suppression Fire sequence
    auto supressionFireSequence = std::make_shared<SequenceNode>();
    supressionFireSequence->AddChild(std::make_shared<ConditionNode>([this]() { return ShouldProvideSuppressionFire(); }));

    // Player Detected Selector: Player Visible or Not Visible
    auto playerDetectedSelector = std::make_shared<SelectorNode>();

    // Player Detected Selector: Player Visible or Not Visible
    auto supressionFireSelector = std::make_shared<SelectorNode>();

    // Player visible sequence
    auto playerVisibleSequence = std::make_shared<SequenceNode>();
    playerVisibleSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsPlayerVisible(); }));
    playerVisibleSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsCooldownComplete(); }));
    playerVisibleSequence->AddChild(std::make_shared<ActionNode>([this]() { return AttackShoot(); }));

    // Player not visible sequence
    auto playerNotVisibleSequence = std::make_shared<SequenceNode>();
    playerNotVisibleSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsPlayerVisible(); }));
    playerNotVisibleSequence->AddChild(std::make_shared<ActionNode>([this]() { return AttackChasePlayer(); }));
        
    // Health below threshold sequence (Seek Cover)
    auto seekCoverSequence = std::make_shared<SequenceNode>();
    seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsInCover(); }));
    seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !ShouldProvideSuppressionFire(); }));
    seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsHealthBelowThreshold(); }));
    seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return SeekCover(); }));
    seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return TakeCover(); }));
    seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterInCoverState(); }));

    // In Cover condition
    auto inCoverCondition = std::make_shared<ConditionNode>([this]() { return IsInCover(); });
    auto inCoverAction = std::make_shared<ActionNode>([this]() { return InCoverAction(); });

    auto inCoverSequence = std::make_shared<SequenceNode>();
    inCoverSequence->AddChild(inCoverCondition);
    inCoverSequence->AddChild(inCoverAction);
    
    inCoverSequence->AddChild(playerVisibleSequence);
    inCoverSequence->AddChild(playerNotVisibleSequence);

    // Patrol action
    auto patrolSequence = std::make_shared<SequenceNode>();
    patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsPlayerInRange(); }));
    patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthBelowThreshold(); }));
    patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsAttacking(); }));
    patrolSequence->AddChild(std::make_shared<ActionNode>([this]() { return Patrol(); }));

    // Add the Player Visible and Not Visible sequences to the Player Detected Selector
    playerDetectedSelector->AddChild(playerVisibleSequence);
    playerDetectedSelector->AddChild(playerNotVisibleSequence);

    // Add the Player Detected Selector to the Player Detected Sequence
    playerDetectedSequence->AddChild(playerDetectedSelector);

    // Add the Player Visible and Not Visible sequences to the Suppression Fire Selector
    supressionFireSelector->AddChild(playerVisibleSequence);
    supressionFireSelector->AddChild(playerNotVisibleSequence);

    // Add the Supression Fire Selector to the Suppression Fire Sequence
    supressionFireSequence->AddChild(supressionFireSelector);

    // Add sequences to attack selector
    attackSelector->AddChild(playerDetectedSequence);
	/*attackSelector->AddChild(playerVisibleSequence);
	attackSelector->AddChild(playerNotVisibleSequence);*/
   /* attackSelector->AddChild(seekCoverSequence);
    attackSelector->AddChild(inCoverSequence);*/
    attackSelector->AddChild(patrolSequence);

    takingDamageSequence->AddChild(attackSelector);

    // Add sequences to root
    root->AddChild(deadSequence);
    root->AddChild(dyingSequence);
    root->AddChild(takingDamageSequence);
    root->AddChild(seekCoverSequence);
    root->AddChild(inCoverSequence);
    root->AddChild(supressionFireSequence);
    root->AddChild(attackSelector);
    root->AddChild(patrolSequence);

    behaviorTree_ = root;
}

void Enemy::DetectPlayer()
{
    isPlayerDetected_ = true;
    eventManager_.Publish(PlayerDetectedEvent{ id_ });
}

bool Enemy::IsDead()
{
    return isDead_;
}

bool Enemy::IsHealthZeroOrBelow()
{
    return health_ <= 0;
}

bool Enemy::IsTakingDamage()
{
    return isTakingDamage_;
}

bool Enemy::IsPlayerDetected()
{
    return isPlayerDetected_;
}

bool Enemy::IsPlayerVisible()
{
    glm::vec3 tempEnemyShootPos = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
    glm::vec3 tempEnemyShootDir = glm::normalize(player.getPosition() - getPosition());
    glm::vec3 hitPoint = glm::vec3(0.0f);

    
    isPlayerVisible_ = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(tempEnemyShootPos, tempEnemyShootDir, hitPoint, aabb);

    return isPlayerVisible_;
}

bool Enemy::IsCooldownComplete()
{
    return enemyShootCooldown <= 0.0f;
}

bool Enemy::IsHealthBelowThreshold()
{
    return health_ < 40;
}

bool Enemy::IsPlayerInRange()
{
    float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());

    glm::vec3 tempEnemyShootPos = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
    glm::vec3 tempEnemyShootDir = glm::normalize(player.getPosition() - getPosition());
    glm::vec3 hitPoint = glm::vec3(0.0f);

    if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
    {
        DetectPlayer();
        isPlayerInRange_ = true;
    }

    return isPlayerInRange_;
}

bool Enemy::IsInCover()
{
    return isInCover_;
}

bool Enemy::IsAttacking()
{
    return isAttacking_;
}

bool Enemy::IsPatrolling()
{
    return isPatrolling_;
}

bool Enemy::ShouldProvideSuppressionFire()
{
    return provideSuppressionFire;
}

NodeStatus Enemy::EnterDyingState()
{
    isDead_ = true;
    isDestroyed = true;
    std::cout << "Enemy Died!" << std::endl;
    SetEnemyState(DYING);
    SetAnimNum(1);
    deathAC->PlayEvent("event:/EnemyDeath");
    dyingTimer = 1.5f;

    // After animation completes, send NPCDiedEvent
    eventManager_.Publish(NPCDiedEvent{ id_ });
    return NodeStatus::Success;
}

NodeStatus Enemy::EnterTakingDamageState()
{
    Logger::log(1, "Enemy was hit!\n", __FUNCTION__);
    SetEnemyState(TAKE_DAMAGE);
    setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
    SetAnimNum(4);
    if (GetEnemyState() != DYING)
        takeDamageAC->PlayEvent("event:/EnemyTakeDamage");
    
    if (damageTimer > 0.0f)
    {
		damageTimer -= dt;
		return NodeStatus::Running;
    }

    isTakingDamage_ = false;
    return NodeStatus::Success;
}

NodeStatus Enemy::AttackShoot()
{
    Shoot();
	isAttacking_ = true;

	if (!IsPlayerVisible())
	{
		return NodeStatus::Failure;
	}

    return NodeStatus::Running;
}

NodeStatus Enemy::AttackChasePlayer()
{
    std::vector<glm::ivec2> path = grid->findPath(
        glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
        glm::ivec2(player.getPosition().x / grid->GetCellSize(), player.getPosition().z / grid->GetCellSize()),
        grid->GetGrid()
    );
    isAttacking_ = true;

    moveEnemy(path, dt, 1.0f, false);

    if (!IsPlayerVisible())
    {
        return NodeStatus::Running;
    }

    return NodeStatus::Success;
}

NodeStatus Enemy::SeekCover()
{
    isSeekingCover_ = true;
	cover = ScoreCoverLocations(player);
    return NodeStatus::Success;
}

NodeStatus Enemy::TakeCover()
{
    isSeekingCover_ = false;
    isTakingCover_ = true;
    std::vector<glm::ivec2> path = grid->findPath(
        glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
        glm::ivec2(cover.worldPosition.x / grid->GetCellSize(), cover.worldPosition.z / grid->GetCellSize()),
        grid->GetGrid()
    );
    moveEnemy(path, dt, 1.0f, false);

    if (reachedDestination)
        return NodeStatus::Success;

    return NodeStatus::Running;
}

NodeStatus Enemy::EnterInCoverState()
{
    isInCover_ = true;
    isSeekingCover_ = false;
    isTakingCover_ = false;
    return NodeStatus::Success;
}

NodeStatus Enemy::Patrol()
{
    isAttacking_ = false;
	isPatrolling_ = true;

    if (reachedDestination == false)
    {
        std::vector<glm::ivec2> path = grid->findPath(
            glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
            glm::ivec2(currentWaypoint.x / grid->GetCellSize(), currentWaypoint.z / grid->GetCellSize()),
            grid->GetGrid()
        );

        moveEnemy(path, dt, 1.0f, false);
    }
    else
    {
        currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

        std::vector<glm::ivec2> path = grid->findPath(
            glm::ivec2(getPosition().x / grid->GetCellSize(), getPosition().z / grid->GetCellSize()),
            glm::ivec2(currentWaypoint.x / grid->GetCellSize(), currentWaypoint.z / grid->GetCellSize()),
            grid->GetGrid()
        );

        reachedDestination = false;

        moveEnemy(path, dt, 1.0f, false);
    }
    return NodeStatus::Running;
}

NodeStatus Enemy::InCoverAction()
{
    if (provideSuppressionFire)
        return NodeStatus::Success;

    return NodeStatus::Running;
}
