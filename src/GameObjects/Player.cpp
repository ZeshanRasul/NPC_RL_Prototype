#include "Player.h"
#include "GameManager.h"

void Player::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
                        GLuint shadowMapTexture, glm::vec3 camPos)
{
	auto modelMat = glm::mat4(1.0f);
	modelMat = translate(modelMat, m_position);
	modelMat = rotate(modelMat, glm::radians(-m_yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMat = glm::scale(modelMat, m_scale);
	std::vector<glm::mat4> matrixData;
	matrixData.push_back(viewMat);
	matrixData.push_back(proj);
	matrixData.push_back(modelMat);
	matrixData.push_back(lightSpaceMat);
	m_uniformBuffer.UploadUboData(matrixData, 0);

	m_playerDualQuatSsBuffer.UploadSsboData(m_model->GetJointDualQuats(), 2);

	if (m_uploadVertexBuffer)
	{
		m_model->UploadVertexBuffers();
		GameManager* gameMgr = GetGameManager();
		gameMgr->GetPhysicsWorld()->AddCollider(GetAABB());
		m_uploadVertexBuffer = false;
	}

	if (shadowMap)
	{
		m_shadowShader->Use();
		m_model->Draw(m_tex);
	}
	else
	{
		m_shader->SetVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);
		m_tex.Bind();
		m_shader->SetInt("albedoMap", 0);
		m_normal.Bind(1);
		m_shader->SetInt("normalMap", 1);
		m_metallic.Bind(2);
		m_shader->SetInt("metallicMap", 2);
		m_roughness.Bind(3);
		m_shader->SetInt("roughnessMap", 3);
		m_ao.Bind(4);
		m_shader->SetInt("aoMap", 4);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->SetInt("shadowMap", 5);
		m_model->Draw(m_tex);

#ifdef _DEBUG
		m_aabb->Render(viewMat, proj, modelMat, m_aabbColor);
#endif
	}
}

void Player::Update(float dt)
{
	UpdateAabb();
	ComputeAudioWorldTransform();
	UpdateComponents(dt);

	if (m_playerShootAudioCooldown > 0.0f)
	{
		m_playerShootAudioCooldown -= dt;
	}


	if (m_playGameStartAudioTimer > 0.0f)
	{
		m_playGameStartAudioTimer -= dt;
	}

	if (m_playGameStartAudio && m_playGameStartAudioTimer < 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> distrib(1, 2);
		int randomIndex = distrib(gen);
		if (randomIndex == 1)
			m_takeDamageAc->PlayEvent("event:/Player2_Game Start");
		else
			m_takeDamageAc->PlayEvent("event:/Player2_Game Start2");
		m_playGameStartAudio = false;
	}

	if (m_destAnim != 0 && m_velocity == 0.0f)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(0);
		m_resetBlend = true;
		m_blendAnim = true;
		SetPrevDirection(STATIONARY);
	}

	if (m_resetBlend)
	{
		m_blendAnim = true;
		m_blendFactor = 0.0f;
		m_resetBlend = false;
	}

	if (m_blendAnim)
	{
		m_blendFactor += m_blendSpeed * dt;

		SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), 1.0f, m_blendFactor, false);
	
		
		if (m_blendFactor >= 1.0f)
		{
			m_blendAnim = false;
			m_resetBlend = true;
			SetSourceAnimNum(GetDestAnimNum());
			m_resetBlend = true;
		}
	}
	else
	{
		SetAnimation(m_destAnim, 1.0f, 1.0f, false);
	}
}


void Player::ComputeAudioWorldTransform()
{
	if (m_recomputeWorldTransform)
	{
		m_recomputeWorldTransform = false;
		auto worldTransform = glm::mat4(1.0f);
		// Scale, then rotate, then translate
		m_audioWorldTransform = translate(worldTransform, m_position);
		m_audioWorldTransform = rotate(worldTransform, glm::radians(-m_yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_audioWorldTransform = glm::scale(worldTransform, m_scale);

		// Inform components world transform updated
		for (auto comp : m_components)
		{
			comp->OnUpdateWorldTransform();
		}
	}
}

void Player::UpdatePlayerVectors()
{
	auto front = glm::vec3(1.0f);
	front.x = cos(glm::radians(m_yaw - 90.0f));
	front.y = 0.0f;
	front.z = sin(glm::radians(m_yaw - 90.0f));
	SetPlayerFront(normalize(front));
	SetPlayerRight(normalize(cross(GetPlayerFront(), glm::vec3(0.0f, 1.0f, 0.0f))));
	m_playerUp = normalize(cross(GetPlayerRight(), GetPlayerFront()));
}

void Player::UpdatePlayerAimVectors()
{
	glm::vec3 front;
	front.x = glm::cos(glm::radians(m_yaw - 90.0f)) * glm::cos(glm::radians(GetAimPitch()));
	front.y = glm::sin(glm::radians(GetAimPitch()));
	front.z = glm::sin(glm::radians(m_yaw - 90.0f)) * glm::cos(glm::radians(GetAimPitch()));

	SetPlayerAimFront(normalize(front));
	m_playerAimRight = normalize(cross(GetPlayerAimFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
	SetPlayerAimUp(normalize(cross(m_playerAimRight, GetPlayerAimFront())));
}

void Player::PlayerProcessKeyboard(CameraMovement direction, float deltaTime)
{
	m_velocity = m_movementSpeed * deltaTime;

	int nextAnim = -1;

	if (direction == FORWARD)
	{
		m_position += GetPlayerFront() * m_velocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		nextAnim = 2;
	}
	if (direction == BACKWARD)
	{
		m_position -= GetPlayerFront() * m_velocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		nextAnim = 2;
	}
	if (direction == LEFT)
	{
		m_position -= GetPlayerRight() * m_velocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		nextAnim = 4;
	}
	if (direction == RIGHT)
	{
		m_position += GetPlayerRight() * m_velocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		nextAnim = 5;
	}

	if (nextAnim != m_destAnim)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(nextAnim);
		m_blendAnim = true;
		m_resetBlend = true;
	}

	SetPrevDirection(direction);
}

void Player::PlayerProcessMouseMovement(float xOffset)
{
	//    xOffset *= SENSITIVITY;  

	m_yaw += xOffset;
	m_recomputeWorldTransform = true;
	ComputeAudioWorldTransform();
	if (GetPlayerState() == MOVING)
		UpdatePlayerVectors();
	else if (GetPlayerState() == AIMING)
		UpdatePlayerAimVectors();
}

//void Player::Speak(const std::string& clipName, float priority, float cooldown)
//{
//	m_gameManager->GetAudioManager()->SubmitAudioRequest(id_, clipName, priority, cooldown);
//}

void Player::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards)
{
	m_model->PlayAnimation(animNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor,
                          bool playAnimBackwards)
{
	m_model->PlayAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetPlayerState(PlayerState newState)
{
	m_playerState = newState;
	if (m_playerState == MOVING)
		UpdatePlayerVectors();
	else if (m_playerState == AIMING)
	{
		UpdatePlayerAimVectors();
		GameManager* gmeMgr = GetGameManager();
		if (gmeMgr->HasCamSwitchedToAim() == false)
		{
			gmeMgr->SetCamSwitchedToAim(true);
		}
	}
}

void Player::Shoot()
{
	if (GetPlayerState() != SHOOTING)
		return;

	if (m_playerShootAudioCooldown < 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};

		std::uniform_int_distribution<> distrib(1, 2);
		int randomIndex = distrib(gen);

		if (randomIndex == 1)
			m_shootAc->PlayEvent("event:/Player2_Firing Weapon");
		else
			m_shootAc->PlayEvent("event:/Player2_Firing Weapon2");
		m_playerShootAudioCooldown = 2.0f;
	}
	//std::string clipName = "event:/player2_Firing Weapon";
	//Speak(clipName, 1.0f, 0.5f);


	SetDestAnimNum(3);
	m_blendFactor = 0.0f;
	m_blendAnim = true;
	UpdatePlayerVectors();
	UpdatePlayerAimVectors();

	glm::vec3 rayO = GetShootPos();
	glm::vec3 rayD;
	float dist = GetShootDistance();

	auto clipCoords = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f);

	glm::vec4 cameraCoords = inverse(m_projection) * clipCoords;
	cameraCoords /= cameraCoords.w;

	glm::vec4 worldCoords = inverse(m_view) * cameraCoords;
	glm::vec3 rayEnd = glm::vec3(worldCoords) / worldCoords.w;

	rayD = normalize(rayEnd - rayO);

	auto hitPoint = glm::vec3(0.0f);

	GameManager* gmeMgr = GetGameManager();
	//gmeMgr->GetPhysicsWorld()->RayEnemyIntersect(rayO, rayD, hitPoint);

	bool hit = false;
	hit = gmeMgr->GetPhysicsWorld()->RayIntersect(rayO, rayD, hitPoint, m_aabb);

	if (hit)
	{
		std::cout << "\nRay hit at: " << hitPoint.x << ", " << hitPoint.y << ", " << hitPoint.z << std::endl;
	}
	else
	{
		std::cout << "\nNo hit detected." << std::endl;
	}

	UpdatePlayerAimVectors();
}

void Player::SetUpAABB()
{
	m_aabb = new AABB();
	m_aabb->CalculateAABB(m_model->GetVertices());
	m_aabb->SetShader(m_aabbShader);
	m_aabb->SetUpMesh();
	m_aabb->SetOwner(this);
	m_aabb->SetIsPlayer(true);
	UpdateAabb();
}

void Player::OnHit()
{
	Logger::Log(1, "Player Hit!");
	SetAabbColor(glm::vec3(1.0f, 0.0f, 1.0f));
	TakeDamage(0.4f);

	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::string clipName = "event:/player2_Taking Damage" + std::to_string(randomIndex);
	m_takeDamageAc->PlayEvent(clipName);
}

void Player::OnDeath()
{
	Logger::Log(1, "Player Died!");
	m_isDestroyed = true;
	m_deathAc->PlayEvent("event:/Player2_Death");
}

void Player::ResetGame()
{
	m_gameManager->ResetGame();
	m_playGameStartAudio = true;
	m_playGameStartAudioTimer = 1.0f;
}
