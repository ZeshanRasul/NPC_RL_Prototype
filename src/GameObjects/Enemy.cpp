#include <random>

#include "Enemy.h"
#include "src/Pathfinding/Grid.h"
#include "GameManager.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

float Enemy::DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize)
{
	if (currentSize >= targetSize) {
		return minRate;
	}
	float decayedRate = minRate + (initialRate - minRate) * (1.0f - static_cast<float>(currentSize) / targetSize);
	return decayedRate;
}

void Enemy::SetUpModel()
{
	if (uploadVertexBuffer)
	{
		model->uploadEnemyVertexBuffers();
		uploadVertexBuffer = false;
	}

	model->uploadIndexBuffer();
	Logger::log(1, "%s: glTF model '%s' successfully loaded\n", __FUNCTION__, model->filename.c_str());

	size_t enemyModelJointDualQuatBufferSize = model->getJointDualQuatsSize() *
		sizeof(glm::mat2x4);
	mEnemyDualQuatSSBuffer.init(enemyModelJointDualQuatBufferSize);
	Logger::log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n", __FUNCTION__, enemyModelJointDualQuatBufferSize);
}

void Enemy::drawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	modelMat = glm::translate(modelMat, position);
	modelMat = glm::rotate(modelMat, glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	mUniformBuffer.uploadUboData(matrixData, 0);

	if (shadowMap)
	{
		shadowShader->use();
		mEnemyDualQuatSSBuffer.uploadSsboData(model->getJointDualQuats(), 2);
		model->draw(mTex);
	}
	else
	{
		GetShader()->use();
		shader->setVec3("cameraPos", mGameManager->GetCamera()->Position);
		mEnemyDualQuatSSBuffer.uploadSsboData(model->getJointDualQuats(), 2);

		mTex.bind();
		shader->setInt("albedoMap", 0);
		mNormal.bind(1);
		shader->setInt("normalMap", 1);
		mMetallic.bind(2);
		shader->setInt("metallicMap", 2);
		mRoughness.bind(3);
		shader->setInt("roughnessMap", 3);
		mAO.bind(4);
		shader->setInt("aoMap", 4);
		mEmissive.bind(5);
		shader->setInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		shader->setInt("shadowMap", 6);
		model->draw(mTex);
#ifdef _DEBUG
		aabb->render(viewMat, proj, modelMat, aabbColor);
#endif
	}
}

void Enemy::Update(bool shouldUseEDBT, float speedDivider, float blendFac)
{
	if (!isDead_ || !isDestroyed)
	{
		if (shouldUseEDBT)
		{
#ifdef TRACY_ENABLE
			ZoneScopedN("EDBT Update");
#endif
			float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());
			if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
			{
				DetectPlayer();
			}

			behaviorTree_->Tick();


			if (enemyHasShot)
			{
				enemyRayDebugRenderTimer -= dt_;
				enemyShootCooldown -= dt_;
			}
			if (enemyShootCooldown <= 0.0f)
			{
				enemyHasShot = false;
			}

			if (shootAudioCooldown > 0.0f)
			{
				shootAudioCooldown -= dt_;
			}
		}
		else
		{
			decisionDelayTimer -= dt_;
		}

		if (isDestroyed)
		{
			GetGameManager()->GetPhysicsWorld()->removeCollider(GetAABB());
			GetGameManager()->GetPhysicsWorld()->removeEnemyCollider(GetAABB());
		}

		if (resetBlend)
		{
			blendAnim = true;
			blendFactor = 0.0f;
			resetBlend = false;
		}

		if (blendAnim)
		{
			blendFactor += (1.0f - blendFactor) * blendSpeed * dt_;
			SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), 2.0f, blendFactor, false);
			if (blendFactor >= 1.0f)
			{
				blendAnim = false;
				blendFactor = 0.0f;
				//SetSourceAnimNum(GetDestAnimNum());
			}
		}
		else
		{
			SetAnimation(GetSourceAnimNum(), speedDivider, blendFac, false);
			blendFactor = 0.0f;
		}
	}
}

void Enemy::OnEvent(const Event& event)
{
	if (const PlayerDetectedEvent* e = dynamic_cast<const PlayerDetectedEvent*>(&event))
	{
		if (e->npcID != id_)
		{
			isPlayerDetected_ = true;
			Logger::log(1, "Player detected by enemy %d\n", id_);
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
		std::random_device rd;
		std::mt19937 gen{ rd() };
		std::uniform_int_distribution<> distrib(1, 3);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		int randomFloat = distribReal(gen);

		int enemyAudioIndex;
		if (id_ == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = id_;
		}

		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Enemy Squad Member Death" + std::to_string(randomIndex);
		Speak(clipName, 6.0f, randomFloat);
	}
	else if (const NPCTakingCoverEvent* e = dynamic_cast<const NPCTakingCoverEvent*>(&event))
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
	const float agentRadius = 0.5f; // Adjust this value to match the agent's radius

	if (!reachedPlayer && !inCover)
	{
		if (!resetBlend && destAnim != 1)
		{
			SetSourceAnimNum(destAnim);
			SetDestAnimNum(1);
			blendAnim = true;
			resetBlend = true;
		}
		//SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
	}

	if (pathIndex_ >= path.size()) {
		Logger::log(1, "%s success: Agent has reached its destination.\n", __FUNCTION__);
		grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

		if (IsPatrolling() || EDBTState == "Patrol" || EDBTState == "PATROL")
		{
			reachedDestination = true;
			std::random_device rd;
			std::mt19937 gen{ rd() };
			std::uniform_int_distribution<> distrib(1, 2);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			int randomFloat = distribReal(gen);

			int enemyAudioIndex;
			if (id_ == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = id_;
			}

			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Patrolling" + std::to_string(randomIndex);
			Speak(clipName, 2.0f, randomFloat);
		}

		if (isTakingCover_)
		{
			if (glm::distance(getPosition(), selectedCover_->worldPosition) < grid_->GetCellSize() / 4.0f)
			{
				reachedCover = true;
				isTakingCover_ = false;
				isInCover_ = true;
				//			grid_->OccupyCell(selectedCover_->gridX, selectedCover_->gridZ, id_);

				if (!resetBlend && destAnim != 2)
				{
					SetSourceAnimNum(destAnim);
					SetDestAnimNum(2);
					blendAnim = true;
					resetBlend = true;
					//SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
				}
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
			if (checkPos.x < 0 || checkPos.x >= grid_->GetGridSize() || checkPos.y < 0 || checkPos.y >= grid_->GetGridSize()
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
				if (checkPos.x < 0 || checkPos.x >= grid_->GetGridSize() || checkPos.y < 0 || checkPos.y >= grid_->GetGridSize()
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
	}

	//if (pathIndex_ == 0) {
	//    // Snap the enemy to the center of the starting grid cell when the path starts
	//    glm::vec3 startCellCenter = glm::vec3(path[pathIndex_].x * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f, getPosition().z, path[pathIndex_].y * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f);
	//    setPosition(startCellCenter);
	//}

	if (glm::distance(getPosition(), targetPos) < grid_->GetCellSize() / 3.0f)
	{
		//	grid_->OccupyCell(path[pathIndex_].x, path[pathIndex_].y, id_);
	}

	//    if (pathIndex_ >= 1)
	 //      grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

		// Check if the enemy has reached the current target position within a tolerance
	if (glm::distance(getPosition(), targetPos) < tolerance) {
		//		grid_->VacateCell(path[pathIndex_].x, path[pathIndex_].y, id_);

		pathIndex_++;
		if (pathIndex_ >= path.size()) {
			//			grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);
			pathIndex_ = 0; // Reset path index if the end is reached
		}
	}
}

void Enemy::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
	model->playAnimation(animNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards)
{
	model->playAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::Shoot()
{
	glm::vec3 accuracyOffset = glm::vec3(0.0f);
	glm::vec3 accuracyOffsetFactor = glm::vec3(0.1f);

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist100(0, 100); // distribution in range [1, 100]

	bool enemyMissed = false;

	if (dist100(rng) < 60)
	{
		enemyMissed = true;
		if (dist100(rng) % 2 == 0)
		{
			accuracyOffset = accuracyOffset + (accuracyOffsetFactor * -(float)dist100(rng));
		}
		else
		{
			accuracyOffset = accuracyOffset + (accuracyOffsetFactor * (float)dist100(rng));
		}
	}

	enemyShootPos = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	enemyShootDir = (player.getPosition() - getPosition()) + accuracyOffset;
	glm::vec3 hitPoint = glm::vec3(0.0f);

	glm::vec3 playerDir = glm::normalize(player.getPosition() - getPosition());

	yaw = glm::degrees(glm::atan(playerDir.z, playerDir.x));
	UpdateEnemyVectors();

	bool hit = false;
	hit = GetGameManager()->GetPhysicsWorld()->rayPlayerIntersect(enemyShootPos, enemyShootDir, enemyHitPoint, aabb);

	if (hit)
	{
		enemyHasHit = true;
	}
	else
	{
		enemyHasHit = false;
	}

	if (!resetBlend && destAnim != 2)
	{
		SetSourceAnimNum(destAnim);
		SetDestAnimNum(2);
		blendAnim = true;
		resetBlend = true;
	}

	//shootAC->PlayEvent("event:/EnemyShoot");
	if (shootAudioCooldown <= 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		std::uniform_int_distribution<> distrib(1, 3);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		int randomFloat = distribReal(gen);

		int enemyAudioIndex;
		if (id_ == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = id_;
		}


		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Attacking-Shooting" + std::to_string(randomIndex);
		Speak(clipName, 1.0f, randomFloat);
		shootAudioCooldown = 3.0f;
	}


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

void Enemy::Speak(const std::string& clipName, float priority, float cooldown)
{
	mGameManager->GetAudioManager()->SubmitAudioRequest(id_, clipName, priority, cooldown);
}

void Enemy::OnHit()
{
	Logger::log(1, "Enemy was hit!\n", __FUNCTION__);
	setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
	TakeDamage(20.0f);
	isTakingDamage_ = true;
	//takeDamageAC->PlayEvent("event:/EnemyTakeDamage");
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	int randomFloat = distribReal(gen);

	int enemyAudioIndex;
	if (id_ == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = id_;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Damage" + std::to_string(randomIndex);
	Speak(clipName, 2.0f, randomFloat);

	damageTimer = 0.2f;
	eventManager_.Publish(NPCDamagedEvent{ id_ });
}

void Enemy::TakeDamage(float damage)
{
	SetHealth(GetHealth() - damage);
	if (health_ <= 0)
	{
		OnDeath();
		return;
	}

	if (!resetBlend && destAnim != 3)
	{
		SetSourceAnimNum(destAnim);
		SetDestAnimNum(3);
		blendAnim = true;
		resetBlend = true;
	}

	isTakingDamage_ = true;
	hasTakenDamage_ = true;
}

void Enemy::OnDeath()
{
	Logger::log(1, "%s Enemy Died!\n", __FUNCTION__);
	isDying_ = true;
	dyingTimer = 0.2f;
	if (!hasDied_ && !resetBlend && destAnim != 0)
	{
		SetSourceAnimNum(destAnim);
		SetDestAnimNum(0);
		blendAnim = true;
		resetBlend = true;
	}
	//deathAC->PlayEvent("event:/EnemyDeath");
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	int randomFloat = distribReal(gen);

	int enemyAudioIndex;
	if (id_ == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = id_;
	}

	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Damage" + std::to_string(randomIndex);
	Speak(clipName, 3.0f, randomFloat);
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


glm::vec3 Enemy::selectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints)
{
	if (isDestroyed) return glm::vec3(0.0f);

	std::vector<glm::vec3> availableWaypoints;
	for (const auto& wp : allWaypoints) {
		if (wp != currentWaypoint) {
			availableWaypoints.push_back(wp);
		}
	}

	// Select a random way point from the available way points
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(0, availableWaypoints.size() - 1);
	int randomIndex = distrib(gen);
	return availableWaypoints[randomIndex];
}

float Enemy::CalculateReward(const NashState& state, NashAction action, int enemyId, const std::vector<NashAction>& squadActions)
{
	float reward = 0.0f;

	if (action == ATTACK) {
		reward += (state.playerVisible && state.playerDetected) ? 40.0f : -35.0f;
		if (hasDealtDamage_)
		{
			reward += 12.0f;
			hasDealtDamage_ = false;

			if (hasKilledPlayer_)
			{
				reward += 25.0f;
				hasKilledPlayer_ = false;
			}
		}

		if (state.health <= 20)
		{
			reward -= 5.0f;
		}
		else if (state.health >= 60 && (state.playerDetected && state.playerVisible))
		{
			reward += 15.0f;
		}

		if (!state.playerVisible)
		{
			reward -= 15.0f;
		}


		if (state.distanceToPlayer < 15.0f)
		{
			reward += 10.0f;

			if (state.health >= 60)
			{
				reward += 7.0f;
			}
		}

	}
	else if (action == ADVANCE) {
		reward += ((state.distanceToPlayer > 15.0f && state.playerDetected && state.health >= 60) || (state.playerDetected && !state.playerVisible && state.health >= 60.0f)) ? 12.0f : -2.0f;

		if (state.distanceToPlayer < 10.0f)
		{
			reward -= 8.0f;
		}
	}
	else if (action == RETREAT) {
		reward += (state.health <= 40) ? 12.0f : -5.0f;

		if (state.health <= 20 && state.distanceToPlayer > 20.0f)
		{
			reward += 8.0f;
		}
	}
	else if (action == PATROL) {
		reward += (!state.playerDetected && !state.playerVisible) ? 45.0f : -15.0f;

		if (state.health == 100)
		{
			reward += 3.0f;
		}
	}

	// Additional reward for coordinated behavior
	int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
	if (action == ATTACK && numAttacking > 1 && (state.playerVisible && state.playerDetected)) {
		reward += 10.0f;
	}

	if (action == RETREAT && numAttacking >= 2 && state.health <= 40) {
		reward += 5.0f;
	}

	if (hasTakenDamage_)
	{
		reward -= 3.0f;
		hasTakenDamage_ = false;
	}

	if (hasDied_)
	{
		reward -= 30.0f;
		hasDied_ = false;

		if (numDeadAllies = 3)
		{
			reward -= 50.0f;
			numDeadAllies = 0;
		}
	}

	if (allyHasDied)
	{
		reward -= 10.0f;
		allyHasDied = false;

	}

	hasDealtDamage_ = false;
	hasKilledPlayer_ = false;

	return reward;
}

float Enemy::GetMaxQValue(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{
	float maxQ = -std::numeric_limits<float>::infinity();
	int targetBucket = getDistanceBucket(state.distanceToPlayer);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dis(0.0, 1.0);

	std::vector<NashAction> actions = { ATTACK, ADVANCE, RETREAT, PATROL };

	std::shuffle(actions.begin(), actions.end(), gen);

	for (auto action : actions) {
		for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset) {
			int bucket = targetBucket + bucketOffset;
			NashState modifiedState = state;
			modifiedState.distanceToPlayer = bucket * BUCKET_SIZE; // Discretized distance

			auto it = qTable[enemyId].find({ modifiedState, action });
			if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <= TOLERANCE) {
				maxQ = std::max(maxQ, it->second);
			}
		}
	}
	return (maxQ == -std::numeric_limits<float>::infinity()) ? 0.0f : maxQ;
}

NashAction Enemy::ChooseAction(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dis(0.0, 1.0);

	int currentQTableSize = qTable[enemyId].size();
	explorationRate = DecayExplorationRate(initialExplorationRate, minExplorationRate, currentQTableSize, targetQTableSize);

	std::vector<NashAction> actions = { ATTACK, ADVANCE, RETREAT, PATROL };

	std::shuffle(actions.begin(), actions.end(), gen);

	if (dis(gen) < explorationRate) {
		// Exploration: choose a random action
		std::uniform_int_distribution<> actionDist(0, 3);
		return static_cast<NashAction>(actionDist(gen));
	}
	else {
		// Exploitation: choose the action with the highest Q-value
		float maxQ = -std::numeric_limits<float>::infinity();
		NashAction bestAction = actions.at(0);
		for (auto action : actions) {
			float qValue = qTable[enemyId][{state, action}];
			if (qValue > maxQ) {
				maxQ = qValue;
				bestAction = action;
			}
		}
		return bestAction;
	}
}

void Enemy::UpdateQValue(const NashState& currentState, NashAction action, const NashState& nextState, float reward,
	int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{
	float currentQ = qTable[enemyId][{currentState, action}];
	float maxFutureQ = GetMaxQValue(nextState, enemyId, qTable);
	float updatedQ = (1 - learningRate) * currentQ + learningRate * (reward + discountFactor * maxFutureQ);
	qTable[enemyId][{currentState, action}] = updatedQ;
}

void Enemy::EnemyDecision(NashState& currentState, int enemyId, std::vector<NashAction>& squadActions, float deltaTime, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{

	if (enemyHasShot)
	{
		enemyRayDebugRenderTimer -= dt_;
		enemyShootCooldown -= dt_;
	}
	if (enemyShootCooldown <= 0.0f)
	{
		enemyHasShot = false;
	}

	if (damageTimer > 0.0f)
	{
		damageTimer -= deltaTime;
		return;
	}

	if (dyingTimer > 0.0f && isDying_)
	{
		dyingTimer -= deltaTime;
		return;
	}
	else if (dyingTimer <= 0.0f && isDying_)
	{
		isDying_ = false;
		isDead_ = true;
		isDestroyed = true;
		dyingTimer = 100000.0f;
		return;
	}

	NashAction chosenAction = ChooseAction(currentState, enemyId, qTable);

	// Simulate taking action and getting a reward
	NashState nextState = currentState;
	int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
	bool isSuppressionFire = numAttacking > 0;
	float playerDistance = glm::distance(getPosition(), player.getPosition());

	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (chosenAction == ADVANCE) {
		EDBTState = "ADVANCE";
		currentPath_ = grid_->findPath(
			glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
			glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
			grid_->GetGrid(),
			enemyId
		);

		VacatePreviousCell();

		moveEnemy(currentPath_, deltaTime, 1.0f, false);

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == RETREAT) {
		EDBTState = "RETREAT";

		if (!selectedCover_ || grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
		{
			ScoreCoverLocations(player);
		}

		glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
		glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);


		currentPath_ = grid_->findPath(
			glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
			glm::ivec2(selectedCover_->gridX, selectedCover_->gridZ),
			grid_->GetGrid(),
			id_
		);

		VacatePreviousCell();

		moveEnemy(currentPath_, dt_, 1.0f, false);

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == ATTACK) {
		EDBTState = "ATTACK";

		if (enemyShootCooldown > 0.0f)
		{
			return;
		}

		Shoot();

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == PATROL) {
		EDBTState = "PATROL";

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

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}

	float reward = CalculateReward(currentState, chosenAction, enemyId, squadActions);

	// Update Q-value
	UpdateQValue(currentState, chosenAction, nextState, reward, enemyId, qTable);

	// Update current state
	currentState = nextState;
	squadActions[enemyId] = chosenAction;

	// Print chosen action
	Logger::log(1, "Enemy %d Chosen Action: %d with reward: %d", enemyId, chosenAction, reward);
}

NashAction Enemy::ChooseActionFromTrainedQTable(const NashState& state, int enemyId, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{
	float maxQ = -std::numeric_limits<float>::infinity();
	NashAction bestAction = PATROL;
	int targetBucket = getDistanceBucket(state.distanceToPlayer);

	for (auto action : { ATTACK, ADVANCE, RETREAT, PATROL }) {
		for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset) {
			int bucket = targetBucket + bucketOffset;
			NashState modifiedState = state;
			modifiedState.distanceToPlayer = bucket * BUCKET_SIZE;  // Use discretized distance

			auto it = qTable[enemyId].find({ modifiedState, action });
			// Check if entry exists and is within tolerance range
			if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <= TOLERANCE) {
				if (it->second > maxQ) {
					maxQ = it->second;
					bestAction = action;
				}
			}
		}
	}

	return bestAction;
}

void Enemy::EnemyDecisionPrecomputedQ(NashState& currentState, int enemyId, std::vector<NashAction>& squadActions, float deltaTime, std::unordered_map<std::pair<NashState, NashAction>, float, PairHash>* qTable)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("Q-Learning Update");
#endif
	if (enemyHasShot)
	{
		enemyRayDebugRenderTimer -= dt_;
		enemyShootCooldown -= dt_;
	}
	if (enemyShootCooldown <= 0.0f)
	{
		enemyHasShot = false;
	}

	if (damageTimer > 0.0f)
	{
		damageTimer -= deltaTime;
		return;
	}

	if (dyingTimer > 0.0f && isDying_)
	{
		dyingTimer -= deltaTime;
		return;
	}
	else if (dyingTimer <= 0.0f && isDying_)
	{
		isDying_ = false;
		isDead_ = true;
		isDestroyed = true;
		dyingTimer = 100000.0f;
		return;
	}

	if (decisionDelayTimer <= 0.0f)
	{
		chosenAction = ChooseActionFromTrainedQTable(currentState, enemyId, qTable);

		switch (chosenAction)
		{
		case ATTACK:
			decisionDelayTimer = 1.0f;
			break;
		case ADVANCE:
			decisionDelayTimer = 1.0f;
			break;
		case RETREAT:
			decisionDelayTimer = 3.0f;
			break;
		case PATROL:
			decisionDelayTimer = 2.0f;
			break;
		}
	}

	int numAttacking = (int)std::count(squadActions.begin(), squadActions.end(), ATTACK);
	bool isSuppressionFire = numAttacking > 0;
	float playerDistance = glm::distance(getPosition(), player.getPosition());
	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (chosenAction == ADVANCE) {
		EDBTState = "ADVANCE";
		currentPath_ = grid_->findPath(
			glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
			glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
			grid_->GetGrid(),
			enemyId
		);

		VacatePreviousCell();

		for (glm::ivec2 cell : currentPath_)
		{
			if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
					grid_->GetGrid(),
					enemyId
				);
		}

		moveEnemy(currentPath_, deltaTime, 1.0f, false);

		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == RETREAT)
	{
		EDBTState = "RETREAT";

		if (!selectedCover_ || grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
		{
			ScoreCoverLocations(player);
		}

		glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
		glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);


		currentPath_ = grid_->findPath(
			glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
			glm::ivec2(selectedCover_->worldPosition.x / grid_->GetCellSize(), selectedCover_->worldPosition.z / grid_->GetCellSize()),
			grid_->GetGrid(),
			id_
		);

		VacatePreviousCell();

		for (glm::ivec2 cell : currentPath_)
		{
			if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(selectedCover_->worldPosition.x / grid_->GetCellSize(), selectedCover_->worldPosition.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					enemyId
				);
		}

		moveEnemy(currentPath_, dt_, 1.0f, false);
		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == ATTACK)
	{
		EDBTState = "ATTACK";

		if (enemyShootCooldown > 0.0f)
		{
			return;
		}

		Shoot();

		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == PATROL) {
		EDBTState = "PATROL";

		if (reachedDestination == false)
		{
			currentPath_ = grid_->findPath(
				glm::ivec2((int)(getPosition().x / grid_->GetCellSize()), (int)(getPosition().z / grid_->GetCellSize())),
				glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
				grid_->GetGrid(),
				id_
			);

			VacatePreviousCell();

			for (glm::ivec2 cell : currentPath_)
			{
				if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
					currentPath_ = grid_->findPath(
						glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
						glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
						grid_->GetGrid(),
						enemyId
					);
			}

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

			for (glm::ivec2 cell : currentPath_)
			{
				if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
					currentPath_ = grid_->findPath(
						glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
						glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
						grid_->GetGrid(),
						enemyId
					);
			}


			moveEnemy(currentPath_, dt_, 1.0f, false);
		}


		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = glm::distance(getPosition(), player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}

	squadActions[enemyId] = chosenAction;

	// Print chosen action
	Logger::log(1, "Enemy %d Chosen Action: %d", enemyId, chosenAction);
}

void Enemy::HasDealtDamage()
{
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(1, 2);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	int randomFloat = distribReal(gen);

	int enemyAudioIndex;
	if (id_ == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = id_;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Deals Damage" + std::to_string(randomIndex);
	Speak(clipName, 3.5f, randomFloat);

	hasDealtDamage_ = true;
}

void Enemy::HasKilledPlayer()
{
	hasKilledPlayer_ = true;
}

void Enemy::ResetState()
{
	isPlayerDetected_ = false;
	isPlayerVisible_ = false;
	isPlayerInRange_ = false;
	isTakingDamage_ = false;
	hasTakenDamage_ = false;
	isDying_ = false;
	hasDied_ = false;
	isInCover_ = false;
	isSeekingCover_ = false;
	isTakingCover_ = false;
	isAttacking_ = false;
	hasDealtDamage_ = false;
	hasKilledPlayer_ = false;
	isPatrolling_ = false;
	provideSuppressionFire_ = false;
	allyHasDied = false;

	numDeadAllies = 0;

	selectedCover_ = nullptr;

	takingDamage = false;
	damageTimer = 0.0f;
	dyingTimer = 0.0f;
	inCover = false;
	coverTimer = 0.0f;
	reachedCover = false;

	reachedDestination = false;
	reachedPlayer = false;

	aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	updateAABB();

	animNum = 1;
	sourceAnim = 1;
	destAnim = 1;
	destAnimSet = false;
	blendSpeed = 5.0f;
	blendFactor = 0.0f;
	blendAnim = false;
	resetBlend = false;

	enemyShootCooldown = 0.0f;
	enemyRayDebugRenderTimer = 0.3f;
	enemyHasShot = false;
	enemyHasHit = false;
	playerIsVisible = false;
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
			//           grid_->VacateCell(prevPath_[i].x, prevPath_[i].y, id_);
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
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	int randomFloat = distribReal(gen);
	int enemyAudioIndex;
	if (id_ == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = id_;
	}

	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Player Detected" + std::to_string(randomIndex);
	Speak(clipName, 5.0f, randomFloat);

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
	//SetAnimNum(0);

	if (!isDying_)
	{
		//deathAC->PlayEvent("event:/EnemyDeath");
		//std::string clipName = "event:/enemy" + std::to_string(id_) + "_Taking Damage1";
		//Speak(clipName, 1.0f, 0.5f);

		dyingTimer = 0.5f;
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
	//SetAnimNum(3);
	if (destAnim != 3)
	{
		SetSourceAnimNum(destAnim);
		SetDestAnimNum(3);
		blendAnim = true;
		resetBlend = true;
	}

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

		if (StartingSuppressionFire)
		{
			std::random_device rd;
			std::mt19937 gen{ rd() };
			std::uniform_int_distribution<> distrib(1, 3);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			int randomFloat = distribReal(gen);

			int enemyAudioIndex;
			if (id_ == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = id_;
			}

			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Providing Suppression Fire" + std::to_string(randomIndex);
			Speak(clipName, 1.0f, randomFloat);
			StartingSuppressionFire = false;
		}

		coverTimer += dt_;
		if (coverTimer > 1.0f)
		{
			health_ += 10.0f;
			coverTimer = 0.0f;

			if (health_ > 40.0f)
			{
				isInCover_ = false;
				provideSuppressionFire_ = false;
				StartingSuppressionFire = true;
				return NodeStatus::Success;
			}
		}
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

	for (glm::ivec2 cell : currentPath_)
	{
		if (cell.x >= 0 && cell.x < grid_->GetGridSize() && cell.y >= 0 && cell.y < grid_->GetGridSize() && grid_->GetGrid()[cell.x][cell.y].IsOccupied())
			currentPath_ = grid_->findPath(
				glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
				glm::ivec2(player.getPosition().x / grid_->GetCellSize(), player.getPosition().z / grid_->GetCellSize()),
				grid_->GetGrid(),
				id_
			);
	}


	moveEnemy(currentPath_, dt_, 1.0f, false);


	if (!IsPlayerVisible())
	{
		if (playNotVisibleAudio)
		{
			std::random_device rd;
			std::mt19937 gen{ rd() };
			std::uniform_int_distribution<> distrib(1, 3);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			int randomFloat = distribReal(gen);

			int enemyAudioIndex;
			if (id_ == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = id_;
			}


			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Chasing(Out of Sight)" + std::to_string(randomIndex);
			Speak(clipName, 3.0f, randomFloat);
			playNotVisibleAudio = false;
		}
		return NodeStatus::Running;
	}

	playNotVisibleAudio = true;
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

	if (!isTakingCover_)
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		std::uniform_int_distribution<> distrib(1, 4);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		int randomFloat = distribReal(gen);

		int enemyAudioIndex;
		if (id_ == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = id_;
		}


		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Cover" + std::to_string(randomIndex);
		Speak(clipName, 5.0f, randomFloat);

		eventManager_.Publish(NPCTakingCoverEvent{ id_ });
	}

	isTakingCover_ = true;

	if (grid_->GetGrid()[selectedCover_->gridX][selectedCover_->gridZ].IsOccupied())
	{
		ScoreCoverLocations(player);
	}

	glm::vec3 snappedCurrentPos = grid_->snapToGrid(getPosition());
	glm::vec3 snappedCoverPos = grid_->snapToGrid(selectedCover_->worldPosition);


	currentPath_ = grid_->findPath(
		glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
		glm::ivec2(selectedCover_->worldPosition.x / grid_->GetCellSize(), selectedCover_->worldPosition.z / grid_->GetCellSize()),
		grid_->GetGrid(),
		id_
	);

	VacatePreviousCell();

	for (glm::ivec2 cell : currentPath_)
	{
		if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
			currentPath_ = grid_->findPath(
				glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
				glm::ivec2(selectedCover_->worldPosition.x / grid_->GetCellSize(), selectedCover_->worldPosition.z / grid_->GetCellSize()),
				grid_->GetGrid(),
				id_
			);
	}


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
	coverTimer = 0.0f;
	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(1, 2);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	int randomFloat = distribReal(gen);

	int enemyAudioIndex;
	if (id_ == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = id_;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_In Cover" + std::to_string(randomIndex);
	Speak(clipName, 5.0f, randomFloat);

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

		for (glm::ivec2 cell : currentPath_)
		{
			if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);
		}


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

		for (glm::ivec2 cell : currentPath_)
		{
			if (grid_->GetGrid()[cell.x][cell.y].IsOccupied())
				currentPath_ = grid_->findPath(
					glm::ivec2(getPosition().x / grid_->GetCellSize(), getPosition().z / grid_->GetCellSize()),
					glm::ivec2(currentWaypoint.x / grid_->GetCellSize(), currentWaypoint.z / grid_->GetCellSize()),
					grid_->GetGrid(),
					id_
				);
		}


		moveEnemy(currentPath_, dt_, 1.0f, false);
	}
	return NodeStatus::Running;
}

NodeStatus Enemy::InCoverAction()
{
	coverTimer += dt_;
	if (coverTimer > 2.5f)
	{
		health_ += 10.0f;
		coverTimer = 0.0f;

		if (health_ > 40.0f)
		{
			isInCover_ = false;
			std::random_device rd;
			std::mt19937 gen{ rd() };
			std::uniform_int_distribution<> distrib(1, 2);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			int randomFloat = distribReal(gen);

			int enemyAudioIndex;
			if (id_ == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = id_;
			}


			std::string clipName;
			if (randomIndex == 1)
				clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Moving Out of Cover1";
			else
				clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Moving Out of Cover";
			Speak(clipName, 4.0f, 1.5f);

			return NodeStatus::Success;
		}
	}

	if (provideSuppressionFire_)
	{
		isInCover_ = false;
		return NodeStatus::Success;
	}

	EDBTState = "In Cover";

	glm::vec3 rayOrigin = getPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 rayDirection = glm::normalize(player.getPosition() - rayOrigin);
	glm::vec3 hitPoint = glm::vec3(0.0f);

	bool visibleToPlayer = mGameManager->GetPhysicsWorld()->checkPlayerVisibility(rayOrigin, rayDirection, hitPoint, aabb);

	if (visibleToPlayer)
	{
		isInCover_ = false;
		return NodeStatus::Success;
	}

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
