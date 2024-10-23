#include <random>

#include "Enemy.h"
#include "src/Pathfinding/Grid.h"
#include "GameManager.h"
#include "src/Tools/Logger.h"

void Enemy::SetUpModel()
{
    if (uploadVertexBuffer)
    {
        model->uploadVertexBuffers();
        uploadVertexBuffer = false;
    }

    model->uploadIndexBuffer();
    Logger::log(1, "%s: glTF model '%s' successfully loaded\n", __FUNCTION__, model->filename.c_str());

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

void Enemy::Update()
{
    if (!isDead_ || !isDestroyed)
    {
		//float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());
		//if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
		//{
		//	DetectPlayer();
		//}

		//behaviorTree_->Tick();


		//if (enemyHasShot)
		//{
		//	enemyRayDebugRenderTimer -= dt_;
  //          enemyShootCooldown -= dt_;
		//}
		//if (enemyShootCooldown <= 0.0f)
		//{
		//	enemyHasShot = false;
		//}

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
                provideSuppressionFire_ = true;
            }
        }
	}
	else if (const NPCDiedEvent* e = dynamic_cast<const NPCDiedEvent*>(&event))
	{
        allyHasDied = true;
        numDeadAllies++;
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
//    static size_t pathIndex = 0;
    if (path.empty())
    {
        return;
    }

    const float tolerance = 0.1f; // Smaller tolerance for better alignment
    const float agentRadius = 0.1f; // Adjust this value to match the agent's radius
    
    if (!reachedPlayer && !inCover)
    {
        SetAnimNum(2);
        SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
    }

    if (pathIndex_ >= path.size()) {
        Logger::log(1, "%s success: Agent has reached its destination.\n", __FUNCTION__);
		grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

        if (IsPatrolling() || EDBTState == "Patrol")
            reachedDestination = true;

		if (isTakingCover_)
		{
			if (glm::distance(getPosition(), selectedCover_->worldPosition) < grid_->GetCellSize() / 4.0f)
            {
                reachedCover = true;
                isTakingCover_ = false;
                isInCover_ = true;
				grid_->OccupyCell(selectedCover_->gridX, selectedCover_->gridZ, id_);
			    SetAnimNum(0);
			    SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
            }
            else
            {
				setPosition(getPosition() + (glm::normalize(selectedCover_->worldPosition - getPosition()) * 2.0f) * speed * deltaTime);
            }
		}
		
        return; // Stop moving if the agent has reached its destination
    }

    // Calculate the target position from the current path node
    glm::vec3 targetPos = glm::vec3(path[pathIndex_].x * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f, getPosition().y, path[pathIndex_].y * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f);
 
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
            glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid_->GetCellSize(), (newPos.z + zOffset) / grid_->GetCellSize());
            if (checkPos.x < 0 || checkPos.x >= grid_->GetCellSize() || checkPos.y < 0 || checkPos.y >= grid_->GetGridSize() 
                || grid_->GetGrid()[checkPos.x][checkPos.y].IsObstacle() || (grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupied() 
                    && !grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupiedBy(id_))) {
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
        newPos = getPosition() + direction * (speed * deltaTime * 0.01f);
        isObstacleFree = true;
        for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
            for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
                glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid_->GetCellSize(), (newPos.z + zOffset) / grid_->GetCellSize());
                if (checkPos.x < 0 || checkPos.x >= grid_->GetCellSize() || checkPos.y < 0 || checkPos.y >= grid_->GetCellSize() 
                    || grid_->GetGrid()[checkPos.x][checkPos.y].IsObstacle()) {
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
        
    if (pathIndex_ == 0) {
        // Snap the enemy to the center of the starting grid cell when the path starts
        glm::vec3 startCellCenter = glm::vec3(path[pathIndex_].x * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f, getPosition().y, path[pathIndex_].y * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f);
        setPosition(startCellCenter);
    }

	if (glm::distance(getPosition(), targetPos) < grid_->GetCellSize() / 2.0f) 
    {
		grid_->OccupyCell(path[pathIndex_].x, path[pathIndex_].y, id_);
		
	}
    if (pathIndex_ >= 1)
       grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

    // Check if the enemy has reached the current target position within a tolerance
    if (glm::distance(getPosition(), targetPos) < tolerance) {
		grid_->VacateCell(path[pathIndex_].x, path[pathIndex_].y, id_);

        pathIndex_++;
        if (pathIndex_ >= path.size()) {
			grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);
            pathIndex_ = 0; // Reset path index if the end is reached
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

    shootAC->PlayEvent("event:/EnemyShoot");
    enemyRayDebugRenderTimer = 0.3f;
    enemyHasShot = true;
    enemyShootCooldown = 0.3f;
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

void Enemy::OnHit()
{
	Logger::log(1, "Enemy was hit!\n", __FUNCTION__);
	setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
    SetAnimNum(4);
    TakeDamage(20.0f);
    isTakingDamage_ = true;
    takeDamageAC->PlayEvent("event:/EnemyTakeDamage");
	damageTimer = model->getAnimationEndTime(4);
    eventManager_.Publish(NPCDamagedEvent{ id_ });
}

void Enemy::OnDeath()
{
	Logger::log(1, "%s Enemy Died!\n", __FUNCTION__);
    isDying_ = true;
	dyingTimer = model->getAnimationEndTime(1);
	SetAnimNum(1);
	deathAC->PlayEvent("event:/EnemyDeath");
    hasDied_ = true;
	eventManager_.Publish(NPCDiedEvent{ id_ });
}

void Enemy::ScoreCoverLocations(Player& player)
{
    float bestScore = -100000.0f;

	Grid::Cover* bestCover = selectedCover_;

	for (Grid::Cover* cover : grid_->GetCoverLocations())
	{
        float score = 0.0f; 
		
        float distanceToPlayer = glm::distance(cover->worldPosition, player.getPosition());
        score += distanceToPlayer * 0.5f;

		float distanceToEnemy = glm::distance(cover->worldPosition, getPosition());
		score -= (1.0f / distanceToEnemy + 1.0f) * 1.0f;

		glm::vec3 rayOrigin = cover->worldPosition + glm::vec3(0.0f, 2.5f, 0.0f);
		glm::vec3 rayDirection = glm::normalize(player.getPosition() - rayOrigin);
        glm::vec3 hitPoint = glm::vec3(0.0f);

        bool visibleToPlayer = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(rayOrigin, rayDirection, hitPoint, aabb);
        if (!visibleToPlayer) {
            score += 20.0f;
        }

        if (grid_->GetGrid()[cover->gridX][cover->gridZ].IsOccupied())
            continue;

		if (score > bestScore)
	    {
			bestScore = score;
			selectedCover_ = cover;
		}
    }
}

void Enemy::VacatePreviousCell()
{
    if (prevPath_.empty())
    {
        prevPathIndex_ = pathIndex_;
        prevPath_ = currentPath_;
    }
	else if (prevPath_ != currentPath_)
    {
        for (size_t i = 0; i < prevPath_.size(); i++)
        {
            grid_->VacateCell(prevPath_[i].x, prevPath_[i].y, id_);
        }
        prevPathIndex_ = pathIndex_;
		prevPath_ = currentPath_;
    }
}

void Enemy::BuildBehaviorTree()
{
    // Top-level Selector
    auto root = std::make_shared<SelectorNode>();

    // Dead check
    auto isDeadCondition = std::make_shared<ConditionNode>([this]() { return IsDead(); });
    auto deadAction = std::make_shared<ActionNode>([this]() { return Die(); });

    auto isDeadSequence = std::make_shared<SequenceNode>();


    auto deadSequence = std::make_shared<SequenceNode>();
    deadSequence->AddChild(isDeadCondition);
    deadSequence->AddChild(deadAction);

    // Dying sequence
    auto dyingSequence = std::make_shared<SequenceNode>();
    dyingSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsHealthZeroOrBelow(); }));
    dyingSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterDyingState(); }));

	isDeadSequence->AddChild(dyingSequence);
	isDeadSequence->AddChild(deadSequence);

    // Taking Damage sequence
    auto takingDamageSequence = std::make_shared<SequenceNode>();
    takingDamageSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
    takingDamageSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsTakingDamage(); }));
    takingDamageSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterTakingDamageState(); }));

    // Attack Selector
    auto attackSelector = std::make_shared<SelectorNode>();

    // Player detected sequence
    auto playerDetectedSequence = std::make_shared<SequenceNode>();
    playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
    playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsPlayerDetected(); }));
 
    // Suppression Fire sequence
    auto suppressionFireSequence = std::make_shared<SequenceNode>();
    suppressionFireSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
    suppressionFireSequence->AddChild(std::make_shared<ConditionNode>([this]() { return ShouldProvideSuppressionFire(); }));

    // Player Detected Selector: Player Visible or Not Visible
    auto playerDetectedSelector = std::make_shared<SelectorNode>();

    // Player Detected Selector: Player Visible or Not Visible
    auto suppressionFireSelector = std::make_shared<SelectorNode>();

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
    seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
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
    inCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
    inCoverSequence->AddChild(inCoverCondition);
    inCoverSequence->AddChild(inCoverAction);
    
    inCoverSequence->AddChild(playerVisibleSequence);
    inCoverSequence->AddChild(playerNotVisibleSequence);

    // Patrol action
    auto patrolSequence = std::make_shared<SequenceNode>();
    patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() {return !IsHealthZeroOrBelow(); }));
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
    suppressionFireSelector->AddChild(playerVisibleSequence);
    suppressionFireSelector->AddChild(playerNotVisibleSequence);

    // Add the Suppression Fire Selector to the Suppression Fire Sequence
    suppressionFireSequence->AddChild(suppressionFireSelector);

    // Add sequences to attack selector
    attackSelector->AddChild(playerDetectedSequence);
    attackSelector->AddChild(patrolSequence);

    takingDamageSequence->AddChild(attackSelector);

    // Add sequences to root
	root->AddChild(isDeadSequence);
    root->AddChild(takingDamageSequence);
    root->AddChild(seekCoverSequence);
    root->AddChild(inCoverSequence);
    root->AddChild(suppressionFireSequence);
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
    return provideSuppressionFire_;
}

NodeStatus Enemy::EnterDyingState()
{
	EDBTState = "Dying";
    SetAnimNum(1);
    
    if (!isDying_)
	{
		deathAC->PlayEvent("event:/EnemyDeath");
        dyingTimer = model->getAnimationEndTime(1);
		isDying_ = true;
	}

    if (dyingTimer > 0.0f)
    {
        dyingTimer -= dt_;
		return NodeStatus::Running;
    }

    isDead_ = true;
    eventManager_.Publish(NPCDiedEvent{ id_ });
    return NodeStatus::Success;
}

NodeStatus Enemy::EnterTakingDamageState()
{
    EDBTState = "Taking Damage";
    setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
    SetAnimNum(4);

    if (damageTimer > 0.0f)
    {
		damageTimer -= dt_;
		return NodeStatus::Running;
    }

    isTakingDamage_ = false;
    return NodeStatus::Success;
}

NodeStatus Enemy::AttackShoot()
{
	if (ShouldProvideSuppressionFire())
	{
		EDBTState = "Providing Suppression Fire";
    } 
    else
    {
		EDBTState = "Attacking";
    }

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
    EDBTState = "Chasing Player";

    currentPath_ = grid_->findPath(
        glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
        glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
        grid_->GetGrid(),
        id_
    );

    VacatePreviousCell();

    isAttacking_ = true;

	moveEnemy(currentPath_, dt_, 1.0f, false);

    if (!IsPlayerVisible())
    {
        return NodeStatus::Running;
    }

    return NodeStatus::Success;
}

NodeStatus Enemy::SeekCover()
{
    isSeekingCover_ = true;
	ScoreCoverLocations(player);
    return NodeStatus::Success;
}

NodeStatus Enemy::TakeCover()
{
	EDBTState = "Taking Cover";
    isSeekingCover_ = false;
    isTakingCover_ = true;

	if (grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
	{
		ScoreCoverLocations(player);
	}

	glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
    glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);
   

    currentPath_ = grid_->findPath(
        glm::ivec2(snappedCurrentPos.x / grid_->GetCellSize(), snappedCurrentPos.z / grid_->GetCellSize()),
		glm::ivec2(snappedCoverPos.x / grid_->GetCellSize(), snappedCoverPos.z / grid_->GetCellSize()),
        grid_->GetGrid(),
        id_
    );

    VacatePreviousCell();

    moveEnemy(currentPath_, dt_, 1.0f, false);

    if (reachedCover)
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
	EDBTState = "Patrolling";
    isAttacking_ = false;
	isPatrolling_ = true;

    if (reachedDestination == false)
    {
        currentPath_ = grid_->findPath(
            glm::ivec2((int)(getPosition().x / grid_->GetCellSize()), (int)(getPosition().z / grid_->GetCellSize())),
            glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
            grid_->GetGrid(),
            id_
        );

        VacatePreviousCell();

        moveEnemy(currentPath_, dt_, 1.0f, false);
    }
    else
    {
        currentWaypoint = selectRandomWaypoint(currentWaypoint, waypointPositions);

        currentPath_ = grid_->findPath(
            glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
            glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
            grid_->GetGrid(),
            id_
        );

        VacatePreviousCell();

        reachedDestination = false;

        moveEnemy(currentPath_, dt_, 1.0f, false);
    }
    return NodeStatus::Running;
}

NodeStatus Enemy::InCoverAction()
{
    if (provideSuppressionFire_)
        return NodeStatus::Success;

	EDBTState = "In Cover";
    return NodeStatus::Running;
}

NodeStatus Enemy::Die()
{
    isDead_ = true;
    isDestroyed = true;
	EDBTState = "Dead";
    eventManager_.Publish(NPCDiedEvent{ id_ });
    return NodeStatus::Success;
}
