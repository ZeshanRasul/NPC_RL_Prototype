#include <random>

#include "Enemy.h"
#include "src/Pathfinding/Grid.h"
#include "GameManager.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

Enemy::Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
	GameManager* gameMgr, Grid* grd, std::string texFilename, int id, EventManager& eventManager, Player& player,
	float yaw) : GameObject(pos, scale, yaw, sdr, shadowMapShader, applySkinning, gameMgr), m_grid(grd), m_player(player),
	m_initialPosition(pos), m_id(id), m_eventManager(eventManager),
	m_health(100.0f), m_isPlayerDetected(false), m_isPlayerVisible(false), m_isPlayerInRange(false),
	m_isTakingDamage(false), m_isDead(false), m_isInCover(false), m_isSeekingCover(false), m_isTakingCover(false)
{
	m_isEnemy = true;

	m_id = id;

	m_model = std::make_shared<GltfModel>();

	std::string modelFilename =
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly.gltf";

	if (!m_model->loadModel(modelFilename, true))
	{
		Logger::Log(1, "%s: loading glTF m_model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
	}

	if (!m_tex.loadTexture(texFilename, false))
	{
		Logger::Log(1, "%s: texture loading failed\n", __FUNCTION__);
	}
	Logger::Log(1, "%s: glTF m_model texture '%s' successfully loaded\n", __FUNCTION__, texFilename.c_str());

	m_normal.loadTexture(
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_Normal.png");
	m_metallic.loadTexture(
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_Metallic.png");
	m_roughness.loadTexture(
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_Roughness.png");
	m_ao.loadTexture(
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_AO.png");
	m_emissive.loadTexture(
		"C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/GLTF/Enemies/Ely/EnemyEly_ely_vanguardsoldier_kerwinatienza_M2_Emissive.png");


	SetUpModel();

	ComputeAudioWorldTransform();

	UpdateEnemyCameraVectors();
	UpdateEnemyVectors();

	std::random_device rd;
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(0, (int)m_waypointPositions.size() - 1);
	int randomIndex = distrib(gen);
	m_currentWaypoint = m_waypointPositions[randomIndex];
	m_takeDamageAc = new AudioComponent(this);
	m_deathAc = new AudioComponent(this);
	m_shootAc = new AudioComponent(this);

	BuildBehaviorTree();

	m_eventManager.Subscribe<PlayerDetectedEvent>([this](const Event& e) { OnEvent(e); });
	m_eventManager.Subscribe<NPCDamagedEvent>([this](const Event& e) { OnEvent(e); });
	m_eventManager.Subscribe<NPCDiedEvent>([this](const Event& e) { OnEvent(e); });
	m_eventManager.Subscribe<NPCTakingCoverEvent>([this](const Event& e) { OnEvent(e); });
}

void Enemy::SetUpModel()
{
	if (m_uploadVertexBuffer)
	{
		m_model->uploadEnemyVertexBuffers();
		m_uploadVertexBuffer = false;
	}

	m_model->uploadIndexBuffer();
	Logger::Log(1, "%s: glTF m_model '%s' successfully loaded\n", __FUNCTION__, m_model->filename.c_str());

	size_t enemyModelJointDualQuatBufferSize = m_model->getJointDualQuatsSize() *
		sizeof(glm::mat2x4);
	m_enemyDualQuatSsBuffer.init(enemyModelJointDualQuatBufferSize);
	Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n",
	            __FUNCTION__, enemyModelJointDualQuatBufferSize);
}

void Enemy::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                       GLuint shadowMapTexture, glm::vec3 camPos)
{
	auto modelMat = glm::mat4(1.0f);
	modelMat = translate(modelMat, m_position);
	modelMat = rotate(modelMat, glm::radians(-m_yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, m_scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	m_uniformBuffer.uploadUboData(matrixData, 0);

	if (shadowMap)
	{
		m_shadowShader->use();
		m_enemyDualQuatSsBuffer.uploadSsboData(m_model->getJointDualQuats(), 2);
		m_model->draw(m_tex);
	}
	else
	{
		GetShader()->use();
		m_shader->setVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);
		m_enemyDualQuatSsBuffer.uploadSsboData(m_model->getJointDualQuats(), 2);

		m_tex.bind();
		m_shader->setInt("albedoMap", 0);
		m_normal.bind(1);
		m_shader->setInt("normalMap", 1);
		m_metallic.bind(2);
		m_shader->setInt("metallicMap", 2);
		m_roughness.bind(3);
		m_shader->setInt("roughnessMap", 3);
		m_ao.bind(4);
		m_shader->setInt("aoMap", 4);
		m_emissive.bind(5);
		m_shader->setInt("emissiveMap", 5);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->setInt("shadowMap", 6);
		m_model->draw(m_tex);
#ifdef _DEBUG
		m_aabb->Render(viewMat, proj, modelMat, m_aabbColor);
#endif
	}
}

void Enemy::Update(bool shouldUseEDBT)
{
	if (!m_isDead || !m_isDestroyed)
	{
		if (shouldUseEDBT)
		{
#ifdef TRACY_ENABLE
			ZoneScopedN("EDBT Update");
#endif
			float playerEnemyDistance = distance(GetPosition(), m_player.getPosition());
			if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
			{
				DetectPlayer();
			}

			m_behaviorTree->Tick();


			if (m_enemyHasShot)
			{
				m_enemyRayDebugRenderTimer -= m_dt;
				m_enemyShootCooldown -= m_dt;
			}
			if (m_enemyShootCooldown <= 0.0f)
			{
				m_enemyHasShot = false;
			}

			if (m_shootAudioCooldown > 0.0f)
			{
				m_shootAudioCooldown -= m_dt;
			}
		}
		else
		{
			m_decisionDelayTimer -= m_dt;
		}

		if (m_isDestroyed)
		{
			GetGameManager()->GetPhysicsWorld()->RemoveCollider(GetAABB());
			GetGameManager()->GetPhysicsWorld()->RemoveEnemyCollider(GetAABB());
		}

		if (m_resetBlend)
		{
			m_blendAnim = true;
			m_blendFactor = 0.0f;
			m_resetBlend = false;
		}

		if (m_blendAnim)
		{
			m_blendFactor += (1.0f - m_blendFactor) * m_blendSpeed * m_dt;
			if (m_blendFactor > 1.0f)
				m_blendFactor = 1.0f;
			SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), 0.5f, m_blendFactor, false);
			if (m_blendFactor >= 1.0f)
			{
				m_blendAnim = false;
				m_blendFactor = 0.0f;
				//SetSourceAnimNum(GetDestAnimNum());
			}
		}
		else
		{
			SetAnimation(GetSourceAnimNum(), 1.0f, 1.0f, false);
			m_blendFactor = 0.0f;
		}
	}
}

void Enemy::OnEvent(const Event& event)
{
	if (auto e = dynamic_cast<const PlayerDetectedEvent*>(&event))
	{
		if (e->npcID != m_id)
		{
			m_isPlayerDetected = true;
			Logger::Log(1, "Player detected by enemy %d\n", m_id);
		}
	}
	else if (auto e = dynamic_cast<const NPCDamagedEvent*>(&event))
	{
		if (e->npcID != m_id)
		{
			if (m_isInCover)
			{
				// Come out of cover and provide suppression fire
				m_isInCover = false;
				m_isSeekingCover = false;
				m_isTakingCover = false;
				m_provideSuppressionFire = true;
			}
		}
	}
	else if (auto e = dynamic_cast<const NPCDiedEvent*>(&event))
	{
		m_allyHasDied = true;
		m_numDeadAllies++;
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> distrib(1, 3);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		float randomFloat = (float)distribReal(gen);

		int enemyAudioIndex;
		if (m_id == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = m_id;
		}

		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Enemy Squad Member Death" +
			std::to_string(randomIndex);
		Speak(clipName, 6.0f, randomFloat);
	}
	else if (auto e = dynamic_cast<const NPCTakingCoverEvent*>(&event))
	{
		if (e->npcID != m_id)
		{
			if (m_isInCover)
			{
				// Come out of cover and provide suppression fire
				m_isInCover = false;
				m_isSeekingCover = false;
				m_isTakingCover = false;
				m_provideSuppressionFire = true;
			}
		}
	}
}

void Enemy::SetPosition(glm::vec3 newPos)
{
	m_position = newPos;
	UpdateAABB();
	m_recomputeWorldTransform = true;
	ComputeAudioWorldTransform();
}

void Enemy::ComputeAudioWorldTransform()
{
	if (m_recomputeWorldTransform)
	{
		m_recomputeWorldTransform = false;
		auto worldTransform = glm::mat4(1.0f);
		// Scale, then rotate, then translate
		m_audioWorldTransform = translate(worldTransform, m_position);
		m_audioWorldTransform = rotate(worldTransform, glm::radians(-m_yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_audioWorldTransform = glm::scale(worldTransform, m_scale);

		// Inform components world transform updated
		for (auto comp : m_components)
		{
			comp->OnUpdateWorldTransform();
		}
	}
};

void Enemy::UpdateEnemyCameraVectors()
{
	auto front = glm::vec3(1.0f);
	front.x = glm::cos(glm::radians(m_enemyCameraYaw)) * glm::cos(glm::radians(m_enemyCameraPitch));
	front.y = glm::sin(glm::radians(m_enemyCameraPitch));
	front.z = glm::sin(glm::radians(m_enemyCameraYaw)) * glm::cos(glm::radians(m_enemyCameraPitch));
	m_enemyFront = normalize(front);
	m_enemyRight = normalize(cross(m_enemyFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_enemyUp = normalize(cross(m_enemyRight, m_enemyFront));
}

void Enemy::UpdateEnemyVectors()
{
	auto front = glm::vec3(1.0f);
	front.x = glm::cos(glm::radians(m_yaw));
	front.y = 0.0f;
	front.z = glm::sin(glm::radians(m_yaw));
	m_front = normalize(front);
	m_right = normalize(cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
	m_up = normalize(cross(m_right, m_front));
}

void Enemy::EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
	// TODO: Update this
	//    xOffset *= SENSITIVITY;  

	m_enemyCameraYaw += xOffset;
	m_enemyCameraPitch += yOffset;

	if (constrainPitch)
	{
		if (m_enemyCameraPitch > 13.0f)
			m_enemyCameraPitch = 13.0f;
		if (m_enemyCameraPitch < -89.0f)
			m_enemyCameraPitch = -89.0f;
	}

	UpdateEnemyCameraVectors();
}

void Enemy::MoveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards)
{
	//    static size_t pathIndex = 0;
	if (path.empty())
	{
		return;
	}

	constexpr float tolerance = 0.1f; // Smaller tolerance for better alignment
	constexpr float agentRadius = 0.5f; // Adjust this value to match the agent's radius

	if (!m_reachedPlayer && !m_isInCover)
	{
		if (!m_resetBlend && m_destAnim != 1)
		{
			SetSourceAnimNum(m_destAnim);
			SetDestAnimNum(1);
			m_blendAnim = true;
			m_resetBlend = true;
		}
		//SetAnimation(GetAnimNum(), 1.0f, m_blendFactor, playAnimBackwards);
	}

	if (m_pathIndex >= path.size())
	{
		Logger::Log(1, "%s success: Agent has reached its destination.\n", __FUNCTION__);
		m_grid->VacateCell(path[m_pathIndex - 1].x, path[m_pathIndex - 1].y, m_id);

		if (IsPatrolling() || m_state == "Patrol" || m_state == "PATROL")
		{
			m_reachedDestination = true;
			std::random_device rd;
			std::mt19937 gen{rd()};
			std::uniform_int_distribution<> distrib(1, 2);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			float randomFloat = (float)distribReal(gen);

			int enemyAudioIndex;
			if (m_id == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = m_id;
			}

			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Patrolling" + std::to_string(
				randomIndex);
			Speak(clipName, 2.0f, randomFloat);
		}

		if (m_isTakingCover)
		{
			if (distance(GetPosition(), m_selectedCover->worldPosition) < m_grid->GetCellSize() / 4.0f)
			{
				m_reachedCover = true;
				m_isTakingCover = false;
				m_isInCover = true;
				m_grid->OccupyCell(m_selectedCover->gridX, m_selectedCover->gridZ, m_id);

				if (!m_resetBlend && m_destAnim != 2)
				{
					SetSourceAnimNum(m_destAnim);
					SetDestAnimNum(2);
					m_blendAnim = true;
					m_resetBlend = true;
					//SetAnimation(GetAnimNum(), 1.0f, m_blendFactor, playAnimBackwards);
				}
			}
			else
			{
				SetPosition(
					GetPosition() + (normalize(m_selectedCover->worldPosition - GetPosition()) * 2.0f) * m_speed *
					deltaTime);
			}
		}

		return; // Stop moving if the agent has reached its destination
	}

	// Calculate the target m_position from the current path node
	auto targetPos = glm::vec3(path[m_pathIndex].x * m_grid->GetCellSize() + m_grid->GetCellSize() / 2.0f, GetPosition().y,
	                           path[m_pathIndex].y * m_grid->GetCellSize() + m_grid->GetCellSize() / 2.0f);

	// Calculate the direction to the target m_position
	glm::vec3 direction = normalize(targetPos - GetPosition());

	//enemy.Yaw = glm::degrees(glm::acos(glm::dot(glm::normalize(enemy.m_front), direction)));
	m_yaw = glm::degrees(glm::atan(direction.z, direction.x));
	m_recomputeWorldTransform = true;

	UpdateEnemyVectors();

	// Calculate the new m_position
	glm::vec3 newPos = GetPosition() + direction * m_speed * deltaTime;

	// Ensure the new m_position is not within an obstacle by checking the bounding box
	bool isObstacleFree = true;
	for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2)
	{
		for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2)
		{
			auto checkPos = glm::ivec2((newPos.x + xOffset) / m_grid->GetCellSize(),
			                           (newPos.z + zOffset) / m_grid->GetCellSize());
			if (checkPos.x < 0 || checkPos.x >= m_grid->GetGridSize() || checkPos.y < 0 || checkPos.y >= m_grid->
				GetGridSize()
				|| m_grid->GetGrid()[checkPos.x][checkPos.y].IsObstacle() || (m_grid->GetGrid()[checkPos.x][checkPos.y].
					IsOccupied()
					&& !m_grid->GetGrid()[checkPos.x][checkPos.y].IsOccupiedBy(m_id)))
			{
				isObstacleFree = false;
				break;
			}
		}
		if (!isObstacleFree) break;
	}

	if (isObstacleFree)
	{
		SetPosition(newPos);
	}
	else
	{
		// If the new m_position is within an obstacle, try to adjust the m_position slightly
		newPos = GetPosition() + direction * (m_speed * deltaTime * 0.01f);
		isObstacleFree = true;
		for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2)
		{
			for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2)
			{
				auto checkPos = glm::ivec2((newPos.x + xOffset) / m_grid->GetCellSize(),
				                           (newPos.z + zOffset) / m_grid->GetCellSize());
				if (checkPos.x < 0 || checkPos.x >= m_grid->GetGridSize() || checkPos.y < 0 || checkPos.y >= m_grid->
					GetGridSize()
					|| m_grid->GetGrid()[checkPos.x][checkPos.y].IsObstacle() || (m_grid->GetGrid()[checkPos.x][checkPos.
							y].IsOccupied()
						&& !m_grid->GetGrid()[checkPos.x][checkPos.y].IsOccupiedBy(m_id)))
				{
					isObstacleFree = false;
					break;
				}
			}
			if (!isObstacleFree) break;
		}

		if (isObstacleFree)
		{
			SetPosition(newPos);
		}
	}

	//if (m_pathIndex == 0) {
	//    // Snap the enemy to the center of the starting grid cell when the path starts
	//    glm::vec3 startCellCenter = glm::vec3(path[m_pathIndex].x * m_grid->GetCellSize() + m_grid->GetCellSize() / 2.0f, GetPosition().z, path[m_pathIndex].y * m_grid->GetCellSize() + m_grid->GetCellSize() / 2.0f);
	//    SetPosition(startCellCenter);
	//}

	if (distance(GetPosition(), targetPos) < m_grid->GetCellSize())
	{
		m_grid->OccupyCell(path[m_pathIndex].x, path[m_pathIndex].y, m_id);
	}

	if (m_pathIndex >= 1)
		m_grid->VacateCell(path[m_pathIndex - 1].x, path[m_pathIndex - 1].y, m_id);

	// Check if the enemy has reached the current target m_position within a tolerance
	if (distance(GetPosition(), targetPos) < tolerance)
	{
		m_grid->VacateCell(path[m_pathIndex].x, path[m_pathIndex].y, m_id);

		m_pathIndex++;
		if (m_pathIndex >= path.size())
		{
			m_grid->VacateCell(path[m_pathIndex - 1].x, path[m_pathIndex - 1].y, m_id);
			m_pathIndex = 0; // Reset path index if the end is reached
		}
	}
}

void Enemy::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
	m_model->playAnimation(animNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards)
{
	m_model->playAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::Shoot()
{
	auto accuracyOffset = glm::vec3(0.0f);
	auto accuracyOffsetFactor = glm::vec3(0.1f);

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist100(0, 100); // distribution in range [1, 100]

	bool enemyMissed = false;

	if (dist100(rng) < 60)
	{
		enemyMissed = true;
		if (dist100(rng) % 2 == 0)
		{
			accuracyOffset = accuracyOffset + (accuracyOffsetFactor * -static_cast<float>(dist100(rng)));
		}
		else
		{
			accuracyOffset = accuracyOffset + (accuracyOffsetFactor * static_cast<float>(dist100(rng)));
		}
	}

	m_enemyShootPos = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	m_enemyShootDir = (m_player.getPosition() - GetPosition()) + accuracyOffset;
	auto hitPoint = glm::vec3(0.0f);

	glm::vec3 playerDir = normalize(m_player.getPosition() - GetPosition());

	m_yaw = glm::degrees(glm::atan(playerDir.z, playerDir.x));
	UpdateEnemyVectors();

	bool hit = false;
	hit = GetGameManager()->GetPhysicsWorld()->RayPlayerIntersect(m_enemyShootPos, m_enemyShootDir, m_enemyHitPoint, m_aabb);

	if (hit)
	{
		m_enemyHasHit = true;
	}
	else
	{
		m_enemyHasHit = false;
	}

	if (!m_resetBlend && m_destAnim != 2)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(2);
		m_blendAnim = true;
		m_resetBlend = true;
	}

	//m_shootAc->PlayEvent("event:/EnemyShoot");
	if (m_shootAudioCooldown <= 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> distrib(1, 3);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		float randomFloat = (float)distribReal(gen);

		int enemyAudioIndex;
		if (m_id == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = m_id;
		}


		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Attacking-Shooting" +
			std::to_string(randomIndex);
		Speak(clipName, 1.0f, randomFloat);
		m_shootAudioCooldown = 3.0f;
	}


	m_enemyRayDebugRenderTimer = 0.3f;
	m_enemyHasShot = true;
	m_enemyShootCooldown = 0.3f;
}

void Enemy::SetUpAABB()
{
	m_aabb = new AABB();
	m_aabb->CalculateAABB(m_model->getVertices());
	m_aabb->SetShader(m_aabbShader);
	UpdateAABB();
	m_aabb->SetUpMesh();
	m_aabb->SetOwner(this);
	m_aabb->SetIsEnemy(true);
	m_gameManager->GetPhysicsWorld()->AddCollider(GetAABB());
	m_gameManager->GetPhysicsWorld()->AddEnemyCollider(GetAABB());
}

void Enemy::Speak(const std::string& clipName, float priority, float cooldown)
{
	m_gameManager->GetAudioManager()->SubmitAudioRequest(m_id, clipName, priority, cooldown);
}

void Enemy::OnHit()
{
	Logger::Log(1, "Enemy was hit!\n", __FUNCTION__);
	SetAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
	TakeDamage(20.0f);
	m_isTakingDamage = true;
	//m_takeDamageAc->PlayEvent("event:/EnemyTakeDamage");
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	float randomFloat = (float)distribReal(gen);

	int enemyAudioIndex;
	if (m_id == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = m_id;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Damage" +
		std::to_string(randomIndex);
	Speak(clipName, 2.0f, randomFloat);

	m_damageTimer = 0.2f;
	m_eventManager.Publish(NPCDamagedEvent{m_id});
}

void Enemy::TakeDamage(float damage)
{
	SetHealth(GetHealth() - damage);
	if (m_health <= 0)
	{
		OnDeath();
		return;
	}

	if (!m_resetBlend && m_destAnim != 3)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(3);
		m_blendAnim = true;
		m_resetBlend = true;
	}

	m_isTakingDamage = true;
	m_hasTakenDamage = true;
}

void Enemy::OnDeath()
{
	Logger::Log(1, "%s Enemy Died!\n", __FUNCTION__);
	m_isDying = true;
	m_dyingTimer = 0.2f;
	if (!m_hasDied && !m_resetBlend && m_destAnim != 0)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(0);
		m_blendAnim = true;
		m_resetBlend = true;
	}
	//m_deathAc->PlayEvent("event:/EnemyDeath");
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	float randomFloat = (float)distribReal(gen);

	int enemyAudioIndex;
	if (m_id == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = m_id;
	}

	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Damage" +
		std::to_string(randomIndex);
	Speak(clipName, 3.0f, randomFloat);
	m_hasDied = true;
	m_eventManager.Publish(NPCDiedEvent{m_id});
}

void Enemy::UpdateAABB()
{
	glm::mat4 modelMatrix = translate(glm::mat4(1.0f), m_position) *
		rotate(glm::mat4(1.0f), glm::radians(m_yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), m_aabbScale);
	m_aabb->Update(modelMatrix);
};

void Enemy::ScoreCoverLocations(Player& player)
{
	float bestScore = -100000.0f;

	Grid::Cover* bestCover = m_selectedCover;

	for (Grid::Cover* cover : m_grid->GetCoverLocations())
	{
		float score = 0.0f;

		//float distanceToPlayer = distance(cover->worldPosition, player.getPosition());
		//score += distanceToPlayer * 0.1f;

		float distanceToEnemy = distance(cover->worldPosition, GetPosition());
		score += (1.0f / (distanceToEnemy * distanceToEnemy + 1.0f)) * 2.0f;

		glm::vec3 rayOrigin = cover->worldPosition + glm::vec3(0.0f, 2.5f, 0.0f);
		glm::vec3 rayDirection = normalize(player.getPosition() - rayOrigin);
		auto hitPoint = glm::vec3(0.0f);

		bool visibleToPlayer = m_gameManager->GetPhysicsWorld()->CheckPlayerVisibility(
			rayOrigin, rayDirection, hitPoint, m_aabb);
		if (!visibleToPlayer)
		{
			score += 20.0f;
		}

		if (m_grid->GetGrid()[cover->gridX][cover->gridZ].IsOccupied())
			continue;

		if (score > bestScore)
		{
			bestScore = score;
			m_selectedCover = cover;
		}
	}
}


glm::vec3 Enemy::SelectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints)
{
	if (m_isDestroyed) return glm::vec3(0.0f);

	std::vector<glm::vec3> availableWaypoints;
	for (const auto& wp : allWaypoints)
	{
		if (wp != currentWaypoint)
		{
			availableWaypoints.push_back(wp);
		}
	}

	// Select a random way point from the available way points
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(0, (int)availableWaypoints.size() - 1);
	int randomIndex = distrib(gen);
	return availableWaypoints[randomIndex];
}

float Enemy::CalculateReward(const State& state, Action action, int enemyId, const std::vector<Action>& squadActions)
{
	float reward = 0.0f;

	if (action == ATTACK)
	{
		reward += (state.playerVisible && state.playerDetected) ? 40.0f : -35.0f;
		if (m_hasDealtDamage)
		{
			reward += 12.0f;
			m_hasDealtDamage = false;

			if (m_hasKilledPlayer)
			{
				reward += 25.0f;
				m_hasKilledPlayer = false;
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
	else if (action == ADVANCE)
	{
		reward += ((state.distanceToPlayer > 15.0f && state.playerDetected && state.health >= 60) || (state.
			          playerDetected && !state.playerVisible && state.health >= 60.0f))
			          ? 12.0f
			          : -2.0f;

		if (state.distanceToPlayer < 10.0f)
		{
			reward -= 8.0f;
		}
	}
	else if (action == RETREAT)
	{
		reward += (state.health <= 40) ? 12.0f : -5.0f;

		if (state.health <= 20 && state.distanceToPlayer > 20.0f)
		{
			reward += 8.0f;
		}
	}
	else if (action == PATROL)
	{
		reward += (!state.playerDetected && !state.playerVisible) ? 45.0f : -15.0f;

		if (state.health == 100)
		{
			reward += 3.0f;
		}
	}

	// Additional reward for coordinated behavior
	int numAttacking = static_cast<int>(std::count(squadActions.begin(), squadActions.end(), ATTACK));
	if (action == ATTACK && numAttacking > 1 && (state.playerVisible && state.playerDetected))
	{
		reward += 10.0f;
	}

	if (action == RETREAT && numAttacking >= 2 && state.health <= 40)
	{
		reward += 5.0f;
	}

	if (m_hasTakenDamage)
	{
		reward -= 3.0f;
		m_hasTakenDamage = false;
	}

	if (m_hasDied)
	{
		reward -= 30.0f;
		m_hasDied = false;

		if (m_numDeadAllies = 3)
		{
			reward -= 50.0f;
			m_numDeadAllies = 0;
		}
	}

	if (m_allyHasDied)
	{
		reward -= 10.0f;
		m_allyHasDied = false;
	}

	m_hasDealtDamage = false;
	m_hasKilledPlayer = false;

	return reward;
}

float Enemy::GetMaxQValue(const State& state, int enemyId,
                          std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
	float maxQ = -std::numeric_limits<float>::infinity();
	int targetBucket = GetDistanceBucket(state.distanceToPlayer);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dis(0.0, 1.0);

	std::vector<Action> actions = {ATTACK, ADVANCE, RETREAT, PATROL};

	std::shuffle(actions.begin(), actions.end(), gen);

	for (auto action : actions)
	{
		for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset)
		{
			int bucket = targetBucket + bucketOffset;
			State modifiedState = state;
			modifiedState.distanceToPlayer = bucket * BUCKET_SIZE; // Discretized distance

			auto it = qTable[enemyId].find({modifiedState, action});
			if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <=
				TOLERANCE)
			{
				maxQ = std::max(maxQ, it->second);
			}
		}
	}
	return (maxQ == -std::numeric_limits<float>::infinity()) ? 0.0f : maxQ;
}

Action Enemy::ChooseAction(const State& state, int enemyId,
                           std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<> dis(0.0, 1.0);

	int currentQTableSize = qTable[enemyId].size();
	m_explorationRate = DecayExplorationRate(m_initialExplorationRate, m_minExplorationRate, currentQTableSize,
	                                       m_targetQTableSize);

	std::vector<Action> actions = {ATTACK, ADVANCE, RETREAT, PATROL};

	std::shuffle(actions.begin(), actions.end(), gen);

	if (dis(gen) < m_explorationRate)
	{
		// Exploration: choose a random action
		std::uniform_int_distribution<> actionDist(0, 3);
		return static_cast<Action>(actionDist(gen));
	}
	// Exploitation: choose the action with the highest Q-value
	float maxQ = -std::numeric_limits<float>::infinity();
	Action bestAction = actions.at(0);
	for (auto action : actions)
	{
		float qValue = qTable[enemyId][{state, action}];
		if (qValue > maxQ)
		{
			maxQ = qValue;
			bestAction = action;
		}
	}
	return bestAction;
}

void Enemy::UpdateQValue(const State& currentState, Action action, const State& nextState, float reward,
                         int enemyId, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
	float currentQ = qTable[enemyId][{currentState, action}];
	float maxFutureQ = GetMaxQValue(nextState, enemyId, qTable);
	float updatedQ = (1 - m_learningRate) * currentQ + m_learningRate * (reward + m_discountFactor * maxFutureQ);
	qTable[enemyId][{currentState, action}] = updatedQ;
}

void Enemy::EnemyDecision(State& currentState, int enemyId, std::vector<Action>& squadActions, float deltaTime,
                          std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
	if (m_enemyHasShot)
	{
		m_enemyRayDebugRenderTimer -= m_dt;
		m_enemyShootCooldown -= m_dt;
	}
	if (m_enemyShootCooldown <= 0.0f)
	{
		m_enemyHasShot = false;
	}

	if (m_damageTimer > 0.0f)
	{
		m_damageTimer -= deltaTime;
		return;
	}

	if (m_dyingTimer > 0.0f && m_isDying)
	{
		m_dyingTimer -= deltaTime;
		return;
	}
	if (m_dyingTimer <= 0.0f && m_isDying)
	{
		m_isDying = false;
		m_isDead = true;
		m_isDestroyed = true;
		m_dyingTimer = 100000.0f;
		return;
	}

	Action chosenAction = ChooseAction(currentState, enemyId, qTable);

	// Simulate taking action and getting a reward
	State nextState = currentState;
	int numAttacking = static_cast<int>(std::count(squadActions.begin(), squadActions.end(), ATTACK));
	bool isSuppressionFire = numAttacking > 0;
	float playerDistance = distance(GetPosition(), m_player.getPosition());

	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (chosenAction == ADVANCE)
	{
		m_state = "ADVANCE";
		m_currentPath = m_grid->findPath(
			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			glm::ivec2(m_player.getPosition().x / m_grid->GetCellSize(), m_player.getPosition().z / m_grid->GetCellSize()),
			m_grid->GetGrid(),
			enemyId
		);

		VacatePreviousCell();

		MoveEnemy(m_currentPath, deltaTime, 1.0f, false);

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == RETREAT)
	{
		m_state = "RETREAT";

		if (!m_selectedCover || m_grid->GetGrid()[m_selectedCover->gridX][m_selectedCover->gridZ].IsOccupied())
		{
			ScoreCoverLocations(m_player);
		}

		glm::vec3 snappedCurrentPos = m_grid->snapToGrid(GetPosition());
		glm::vec3 snappedCoverPos = m_grid->snapToGrid(m_selectedCover->worldPosition);


		m_currentPath = m_grid->findPath(
			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			glm::ivec2(m_selectedCover->gridX, m_selectedCover->gridZ),
			m_grid->GetGrid(),
			m_id
		);

		VacatePreviousCell();

		MoveEnemy(m_currentPath, m_dt, 1.0f, false);

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == ATTACK)
	{
		m_state = "ATTACK";

		if (m_enemyShootCooldown > 0.0f)
		{
			return;
		}

		Shoot();

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == PATROL)
	{
		m_state = "PATROL";

		if (m_reachedDestination == false)
		{
			m_currentPath = m_grid->findPath(
				glm::ivec2(static_cast<int>(GetPosition().x / m_grid->GetCellSize()),
				           static_cast<int>(GetPosition().z / m_grid->GetCellSize())),
				glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
				m_grid->GetGrid(),
				m_id
			);

			VacatePreviousCell();

			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}
		else
		{
			m_currentWaypoint = SelectRandomWaypoint(m_currentWaypoint, m_waypointPositions);

			m_currentPath = m_grid->findPath(
				glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
				glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
				m_grid->GetGrid(),
				m_id
			);

			VacatePreviousCell();

			m_reachedDestination = false;

			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
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
	Logger::Log(1, "Enemy %d Chosen Action: %d with reward: %d", enemyId, chosenAction, reward);
}

Action Enemy::ChooseActionFromTrainedQTable(const State& state, int enemyId,
                                            std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
	float maxQ = -std::numeric_limits<float>::infinity();
	Action bestAction = PATROL;
	int targetBucket = GetDistanceBucket(state.distanceToPlayer);

	for (auto action : {ATTACK, ADVANCE, RETREAT, PATROL})
	{
		for (int bucketOffset = -1; bucketOffset <= 1; ++bucketOffset)
		{
			int bucket = targetBucket + bucketOffset;
			State modifiedState = state;
			modifiedState.distanceToPlayer = bucket * BUCKET_SIZE; // Use discretized distance

			auto it = qTable[enemyId].find({modifiedState, action});
			// Check if entry exists and is within tolerance range
			if (it != qTable[enemyId].end() && std::abs(it->first.first.distanceToPlayer - state.distanceToPlayer) <=
				TOLERANCE)
			{
				if (it->second > maxQ)
				{
					maxQ = it->second;
					bestAction = action;
				}
			}
		}
	}

	return bestAction;
}

void Enemy::EnemyDecisionPrecomputedQ(State& currentState, int enemyId, std::vector<Action>& squadActions,
                                      float deltaTime,
                                      std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable)
{
#ifdef TRACY_ENABLE
	ZoneScopedN("Q-Learning Update");
#endif
	if (m_enemyHasShot)
	{
		m_enemyRayDebugRenderTimer -= m_dt;
		m_enemyShootCooldown -= m_dt;
	}
	if (m_enemyShootCooldown <= 0.0f)
	{
		m_enemyHasShot = false;
	}

	if (m_damageTimer > 0.0f)
	{
		m_damageTimer -= deltaTime;
		return;
	}

	if (m_dyingTimer > 0.0f && m_isDying)
	{
		m_dyingTimer -= deltaTime;
		return;
	}
	if (m_dyingTimer <= 0.0f && m_isDying)
	{
		m_isDying = false;
		m_isDead = true;
		m_isDestroyed = true;
		m_dyingTimer = 100000.0f;
		return;
	}

	if (m_decisionDelayTimer <= 0.0f)
	{
		m_chosenAction = ChooseActionFromTrainedQTable(currentState, enemyId, qTable);

		switch (m_chosenAction)
		{
		case ATTACK:
			m_decisionDelayTimer = 1.0f;
			break;
		case ADVANCE:
			m_decisionDelayTimer = 1.0f;
			break;
		case RETREAT:
			m_decisionDelayTimer = 3.0f;
			break;
		case PATROL:
			m_decisionDelayTimer = 2.0f;
			break;
		}
	}

	int numAttacking = static_cast<int>(std::count(squadActions.begin(), squadActions.end(), ATTACK));
	bool isSuppressionFire = numAttacking > 0;
	float playerDistance = distance(GetPosition(), m_player.getPosition());
	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (m_chosenAction == ADVANCE)
	{
		m_state = "ADVANCE";
		m_currentPath = m_grid->findPath(
			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			glm::ivec2(m_player.getPosition().x / m_grid->GetCellSize(), m_player.getPosition().z / m_grid->GetCellSize()),
			m_grid->GetGrid(),
			enemyId
		);

		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->findPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_player.GetPosition().x / m_grid->GetCellSize(), m_player.GetPosition().z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			enemyId
		//		);
		//}

		MoveEnemy(m_currentPath, deltaTime, 1.0f, false);

		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (m_chosenAction == RETREAT)
	{
		m_state = "RETREAT";

		if (!m_selectedCover || m_grid->GetGrid()[m_selectedCover->gridX][m_selectedCover->gridZ].IsOccupied())
		{
			ScoreCoverLocations(m_player);
		}

		glm::vec3 snappedCurrentPos = m_grid->snapToGrid(GetPosition());
		glm::vec3 snappedCoverPos = m_grid->snapToGrid(m_selectedCover->worldPosition);


		m_currentPath = m_grid->findPath(
			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			glm::ivec2(m_selectedCover->worldPosition.x / m_grid->GetCellSize(),
			           m_selectedCover->worldPosition.z / m_grid->GetCellSize()),
			m_grid->GetGrid(),
			m_id
		);

		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->findPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_selectedCover->worldPosition.x / m_grid->GetCellSize(), m_selectedCover->worldPosition.z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			enemyId
		//		);
		//}

		MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (m_chosenAction == ATTACK)
	{
		m_state = "ATTACK";

		if (m_enemyShootCooldown > 0.0f)
		{
			return;
		}

		Shoot();

		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (m_chosenAction == PATROL)
	{
		m_state = "PATROL";

		if (m_reachedDestination == false)
		{
			m_currentPath = m_grid->findPath(
				glm::ivec2(static_cast<int>(GetPosition().x / m_grid->GetCellSize()),
				           static_cast<int>(GetPosition().z / m_grid->GetCellSize())),
				glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
				m_grid->GetGrid(),
				m_id
			);

			VacatePreviousCell();

			//for (glm::ivec2& cell : m_currentPath)
			//{
			//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
			//		m_currentPath = m_grid->findPath(
			//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			//			glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
			//			m_grid->GetGrid(),
			//			enemyId
			//		);
			//}

			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}
		else
		{
			m_currentWaypoint = SelectRandomWaypoint(m_currentWaypoint, m_waypointPositions);

			m_currentPath = m_grid->findPath(
				glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
				glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
				m_grid->GetGrid(),
				m_id
			);

			VacatePreviousCell();

			m_reachedDestination = false;

			/*		for (glm::ivec2& cell : m_currentPath)
					{
						if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
							m_currentPath = m_grid->findPath(
								glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
								glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
								m_grid->GetGrid(),
								enemyId
							);
					}*/


			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}


		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPosition(), m_player.getPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}

	squadActions[enemyId] = m_chosenAction;

	// Print chosen action
	Logger::Log(1, "Enemy %d Chosen Action: %d", enemyId, m_chosenAction);
}

void Enemy::HasDealtDamage()
{
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 2);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	float randomFloat = (float)distribReal(gen);

	int enemyAudioIndex;
	if (m_id == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = m_id;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Deals Damage" +
		std::to_string(randomIndex);
	Speak(clipName, 3.5f, randomFloat);

	m_hasDealtDamage = true;
}

void Enemy::HasKilledPlayer()
{
	m_hasKilledPlayer = true;
}

void Enemy::ResetState()
{
	m_isPlayerDetected = false;
	m_isPlayerVisible = false;
	m_isPlayerInRange = false;
	m_isTakingDamage = false;
	m_hasTakenDamage = false;
	m_isDying = false;
	m_hasDied = false;
	m_isInCover = false;
	m_isSeekingCover = false;
	m_isTakingCover = false;
	m_isAttacking = false;
	m_hasDealtDamage = false;
	m_hasKilledPlayer = false;
	m_isPatrolling = false;
	m_provideSuppressionFire = false;
	m_allyHasDied = false;

	m_numDeadAllies = 0;

	m_selectedCover = nullptr;

	m_takingDamage = false;
	m_damageTimer = 0.0f;
	m_dyingTimer = 0.0f;
	m_coverTimer = 0.0f;
	m_reachedCover = false;

	m_reachedDestination = false;
	m_reachedPlayer = false;

	m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	UpdateAABB();

	m_animNum = 1;
	m_sourceAnim = 1;
	m_destAnim = 1;
	m_destAnimSet = false;
	m_blendSpeed = 5.0f;
	m_blendFactor = 0.0f;
	m_blendAnim = false;
	m_resetBlend = false;

	m_enemyShootCooldown = 0.0f;
	m_enemyRayDebugRenderTimer = 0.3f;
	m_enemyHasShot = false;
	m_enemyHasHit = false;
	m_playerIsVisible = false;
}

void Enemy::VacatePreviousCell()
{
	if (m_prevPath.empty())
	{
		m_prevPathIndex = m_pathIndex;
		m_prevPath = m_currentPath;
	}
	else if (m_prevPath != m_currentPath)
	{
		for (size_t i = 0; i < m_prevPath.size(); i++)
		{
			m_grid->VacateCell(m_prevPath[i].x, m_prevPath[i].y, m_id);
		}
		m_prevPathIndex = m_pathIndex;
		m_prevPath = m_currentPath;
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
	takingDamageSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	takingDamageSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsTakingDamage(); }));
	takingDamageSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterTakingDamageState(); }));

	// Attack Selector
	auto attackSelector = std::make_shared<SelectorNode>();

	// Player detected sequence
	auto playerDetectedSequence = std::make_shared<SequenceNode>();
	playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsPlayerDetected(); }));

	// Suppression Fire sequence
	auto suppressionFireSequence = std::make_shared<SequenceNode>();
	suppressionFireSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	suppressionFireSequence->AddChild(std::make_shared<ConditionNode>([this]()
	{
		return ShouldProvideSuppressionFire();
	}));

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
	seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsInCover(); }));
	seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !ShouldProvideSuppressionFire(); }));
	seekCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsHealthBelowThreshold(); }));
	seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return SeekCover(); }));
	seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return TakeCover(); }));
	seekCoverSequence->AddChild(std::make_shared<ActionNode>([this]() { return EnterInCoverState(); }));

	// In Cover condition
	auto inCoverCondition = std::make_shared<ConditionNode>([this]() { return IsInCover(); });
	auto inCoverAction = std::make_shared<ActionNode>([this]() { return InCoverAction(); });

	auto inCoverSequence = std::make_shared<SequenceNode>();
	inCoverSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	inCoverSequence->AddChild(inCoverCondition);
	inCoverSequence->AddChild(inCoverAction);

	inCoverSequence->AddChild(playerVisibleSequence);
	inCoverSequence->AddChild(playerNotVisibleSequence);

	// Patrol action
	auto patrolSequence = std::make_shared<SequenceNode>();
	patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
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

	m_behaviorTree = root;
}

void Enemy::DetectPlayer()
{
	m_isPlayerDetected = true;
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	float randomFloat = (float)distribReal(gen);

	int enemyAudioIndex;
	if (m_id == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = m_id;
	}

	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Player Detected" +
		std::to_string(randomIndex);
	Speak(clipName, 5.0f, randomFloat);

	m_eventManager.Publish(PlayerDetectedEvent{m_id});
}

bool Enemy::IsDead()
{
	return m_isDead;
}

bool Enemy::IsHealthZeroOrBelow()
{
	return m_health <= 0;
}

bool Enemy::IsTakingDamage()
{
	return m_isTakingDamage;
}

bool Enemy::IsPlayerDetected()
{
	return m_isPlayerDetected;
}

bool Enemy::IsPlayerVisible()
{
	glm::vec3 tempEnemyShootPos = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 tempEnemyShootDir = normalize(m_player.getPosition() - GetPosition());
	auto hitPoint = glm::vec3(0.0f);


	m_isPlayerVisible = m_gameManager->GetPhysicsWorld()->CheckPlayerVisibility(
		tempEnemyShootPos, tempEnemyShootDir, hitPoint, m_aabb);

	return m_isPlayerVisible;
}

bool Enemy::IsCooldownComplete()
{
	return m_enemyShootCooldown <= 0.0f;
}

bool Enemy::IsHealthBelowThreshold()
{
	return m_health < 40;
}

bool Enemy::IsPlayerInRange()
{
	float playerEnemyDistance = distance(GetPosition(), m_player.getPosition());

	glm::vec3 tempEnemyShootPos = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 tempEnemyShootDir = normalize(m_player.getPosition() - GetPosition());
	auto hitPoint = glm::vec3(0.0f);

	if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
	{
		DetectPlayer();
		m_isPlayerInRange = true;
	}

	return m_isPlayerInRange;
}

bool Enemy::IsTakingCover()
{
	return m_isTakingCover;
}

bool Enemy::IsInCover()
{
	return m_isInCover;
}

bool Enemy::IsAttacking()
{
	return m_isAttacking;
}

bool Enemy::IsPatrolling()
{
	return m_isPatrolling;
}

bool Enemy::ShouldProvideSuppressionFire()
{
	return m_provideSuppressionFire;
}

NodeStatus Enemy::EnterDyingState()
{
	m_state = "Dying";
	//SetAnimNum(0);

	if (!m_isDying)
	{
		//m_deathAc->PlayEvent("event:/EnemyDeath");
		//std::string clipName = "event:/enemy" + std::to_string(m_id) + "_Taking Damage1";
		//Speak(clipName, 1.0f, 0.5f);

		m_dyingTimer = 0.5f;
		m_isDying = true;
	}

	if (m_dyingTimer > 0.0f)
	{
		m_dyingTimer -= m_dt;
		return NodeStatus::Running;
	}

	m_isDead = true;
	m_eventManager.Publish(NPCDiedEvent{m_id});
	return NodeStatus::Success;
}

NodeStatus Enemy::EnterTakingDamageState()
{
	m_state = "Taking Damage";
	SetAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
	//SetAnimNum(3);
	if (m_destAnim != 3)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(3);
		m_blendAnim = true;
		m_resetBlend = true;
	}

	if (m_damageTimer > 0.0f)
	{
		m_damageTimer -= m_dt;
		return NodeStatus::Running;
	}

	m_isTakingDamage = false;
	return NodeStatus::Success;
}

NodeStatus Enemy::AttackShoot()
{
	if (ShouldProvideSuppressionFire())
	{
		m_state = "Providing Suppression Fire";

		if (m_startingSuppressionFire)
		{
			std::random_device rd;
			std::mt19937 gen{rd()};
			std::uniform_int_distribution<> distrib(1, 3);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			float randomFloat = (float)distribReal(gen);

			int enemyAudioIndex;
			if (m_id == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = m_id;
			}

			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Providing Suppression Fire" +
				std::to_string(randomIndex);
			Speak(clipName, 1.0f, randomFloat);
			m_startingSuppressionFire = false;
		}

		m_coverTimer += m_dt;
		if (m_coverTimer > 1.0f)
		{
			m_health += 10.0f;
			m_coverTimer = 0.0f;

			if (m_health > 40.0f)
			{
				m_isInCover = false;
				m_provideSuppressionFire = false;
				m_startingSuppressionFire = true;
				return NodeStatus::Success;
			}
		}
	}
	else
	{
		m_state = "Attacking";
	}

	Shoot();
	m_isAttacking = true;

	if (!IsPlayerVisible())
	{
		return NodeStatus::Failure;
	}

	return NodeStatus::Running;
}

NodeStatus Enemy::AttackChasePlayer()
{
	m_state = "Chasing Player";

	m_currentPath = m_grid->findPath(
		glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		glm::ivec2(m_player.getPosition().x / m_grid->GetCellSize(), m_player.getPosition().z / m_grid->GetCellSize()),
		m_grid->GetGrid(),
		m_id
	);

	VacatePreviousCell();

	m_isAttacking = true;

	//for (glm::ivec2& cell : m_currentPath)
	//{
	//	if (cell.x >= 0 && cell.x < m_grid->GetGridSize() && cell.y >= 0 && cell.y < m_grid->GetGridSize() && m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
	//		m_currentPath = m_grid->findPath(
	//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
	//			glm::ivec2(m_player.GetPosition().x / m_grid->GetCellSize(), m_player.GetPosition().z / m_grid->GetCellSize()),
	//			m_grid->GetGrid(),
	//			m_id
	//		);
	//}


	MoveEnemy(m_currentPath, m_dt, 1.0f, false);


	if (!IsPlayerVisible())
	{
		if (m_playNotVisibleAudio)
		{
			std::random_device rd;
			std::mt19937 gen{rd()};
			std::uniform_int_distribution<> distrib(1, 3);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(2.0, 3.0);
			float randomFloat = (float)distribReal(gen);

			int enemyAudioIndex;
			if (m_id == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = m_id;
			}


			std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Chasing(Out of Sight)" +
				std::to_string(randomIndex);
			Speak(clipName, 3.0f, randomFloat);
			m_playNotVisibleAudio = false;
		}
		return NodeStatus::Running;
	}

	m_playNotVisibleAudio = true;
	return NodeStatus::Success;
}

NodeStatus Enemy::SeekCover()
{

	if (!m_isTakingCover)
	{
		m_isSeekingCover = true;
		ScoreCoverLocations(m_player);
	}

	return NodeStatus::Success;
}

NodeStatus Enemy::TakeCover()
{
	m_state = "Taking Cover";
	m_isSeekingCover = false;

	if (!m_isTakingCover)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> distrib(1, 4);
		int randomIndex = distrib(gen);
		std::uniform_real_distribution<> distribReal(2.0, 3.0);
		float randomFloat = (float)distribReal(gen);

		int enemyAudioIndex;
		if (m_id == 3)
		{
			enemyAudioIndex = 4;
		}
		else
		{
			enemyAudioIndex = m_id;
		}


		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Taking Cover" +
			std::to_string(randomIndex);
		Speak(clipName, 5.0f, randomFloat);

		m_eventManager.Publish(NPCTakingCoverEvent{m_id});
	}

	m_isTakingCover = true;

	//if (m_grid->GetGrid()[m_selectedCover->gridX][m_selectedCover->gridZ].IsOccupied())
	//{
	//	ScoreCoverLocations(m_player);
	//}

	glm::vec3 snappedCurrentPos = m_grid->snapToGrid(GetPosition());
	glm::vec3 snappedCoverPos = m_grid->snapToGrid(m_selectedCover->worldPosition);


	m_currentPath = m_grid->findPath(
		glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		glm::ivec2(m_selectedCover->worldPosition.x / m_grid->GetCellSize(),
		           m_selectedCover->worldPosition.z / m_grid->GetCellSize()),
		m_grid->GetGrid(),
		m_id
	);

	VacatePreviousCell();

	//for (glm::ivec2& cell : m_currentPath)
	//{
	//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
	//		m_currentPath = m_grid->findPath(
	//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
	//			glm::ivec2(m_selectedCover->worldPosition.x / m_grid->GetCellSize(), m_selectedCover->worldPosition.z / m_grid->GetCellSize()),
	//			m_grid->GetGrid(),
	//			m_id
	//		);
	//}


	MoveEnemy(m_currentPath, m_dt, 1.0f, false);

	if (m_reachedCover)
		return NodeStatus::Success;

	return NodeStatus::Running;
}

NodeStatus Enemy::EnterInCoverState()
{
	m_isInCover = true;
	m_isSeekingCover = false;
	m_isTakingCover = false;
	m_coverTimer = 0.0f;
	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 2);
	int randomIndex = distrib(gen);
	std::uniform_real_distribution<> distribReal(2.0, 3.0);
	float randomFloat = (float)distribReal(gen);

	int enemyAudioIndex;
	if (m_id == 3)
	{
		enemyAudioIndex = 4;
	}
	else
	{
		enemyAudioIndex = m_id;
	}


	std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_In Cover" + std::to_string(randomIndex);
	Speak(clipName, 5.0f, randomFloat);

	return NodeStatus::Success;
}

NodeStatus Enemy::Patrol()
{
	m_state = "Patrolling";
	m_isAttacking = false;
	m_isPatrolling = true;

	if (m_reachedDestination == false)
	{
		m_currentPath = m_grid->findPath(
			glm::ivec2(static_cast<int>(GetPosition().x / m_grid->GetCellSize()),
			           static_cast<int>(GetPosition().z / m_grid->GetCellSize())),
			glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
			m_grid->GetGrid(),
			m_id
		);

		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->findPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			m_id
		//		);
		//}


		MoveEnemy(m_currentPath, m_dt, 1.0f, false);
	}
	else
	{
		m_currentWaypoint = SelectRandomWaypoint(m_currentWaypoint, m_waypointPositions);

		m_currentPath = m_grid->findPath(
			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
			glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
			m_grid->GetGrid(),
			m_id
		);

		VacatePreviousCell();

		m_reachedDestination = false;

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->findPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			m_id
		//		);
		//}


		MoveEnemy(m_currentPath, m_dt, 1.0f, false);
	}
	return NodeStatus::Running;
}

NodeStatus Enemy::InCoverAction()
{
	m_coverTimer += m_dt;
	if (m_coverTimer > 2.5f)
	{
		m_health += 10.0f;
		m_coverTimer = 0.0f;

		if (m_health > 40.0f)
		{
			m_isInCover = false;
			std::random_device rd;
			std::mt19937 gen{rd()};
			std::uniform_int_distribution<> distrib(1, 2);
			int randomIndex = distrib(gen);
			std::uniform_real_distribution<> distribReal(0.0f, 2.0f);
			float randomFloat = (float)distribReal(gen);

			int enemyAudioIndex;
			if (m_id == 3)
			{
				enemyAudioIndex = 4;
			}
			else
			{
				enemyAudioIndex = m_id;
			}


			std::string clipName;
			if (randomIndex == 1)
				clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Moving Out of Cover1";
			else
				clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Moving Out of Cover";
			Speak(clipName, 4.0f, randomFloat);

			return NodeStatus::Success;
		}
	}

	if (m_provideSuppressionFire)
	{
		m_isInCover = false;
		return NodeStatus::Success;
	}

	m_state = "In Cover";

	glm::vec3 rayOrigin = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 rayDirection = normalize(m_player.getPosition() - rayOrigin);
	auto hitPoint = glm::vec3(0.0f);

	bool visibleToPlayer = m_gameManager->GetPhysicsWorld()->CheckPlayerVisibility(
		rayOrigin, rayDirection, hitPoint, m_aabb);

	if (visibleToPlayer)
	{
		m_isInCover = false;
		return NodeStatus::Success;
	}

	return NodeStatus::Running;
}

NodeStatus Enemy::Die()
{
	m_isDead = true;
	m_isDestroyed = true;
	m_state = "Dead";
	m_eventManager.Publish(NPCDiedEvent{m_id});
	return NodeStatus::Success;
}

float Enemy::DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize)
{
	if (currentSize >= targetSize)
	{
		return minRate;
	}
	float decayedRate = minRate + (initialRate - minRate) * (1.0f - static_cast<float>(currentSize) / targetSize);
	return decayedRate;
}