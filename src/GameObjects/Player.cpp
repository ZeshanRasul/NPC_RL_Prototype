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
	m_uniformBuffer.uploadUboData(matrixData, 0);

	mPlayerDualQuatSSBuffer.uploadSsboData(m_model->getJointDualQuats(), 2);

	if (uploadVertexBuffer)
	{
		m_model->uploadVertexBuffers();
		aabb = new AABB();
		aabb->CalculateAABB(m_model->getVertices());
		aabb->SetShader(aabbShader);
		aabb->SetUpMesh();
		aabb->SetOwner(this);
		aabb->SetIsPlayer(true);
		updateAABB();
		GameManager* gameMgr = GetGameManager();
		gameMgr->GetPhysicsWorld()->AddCollider(GetAABB());
		uploadVertexBuffer = false;
	}

	updateAABB();
	if (shadowMap)
	{
		m_shadowShader->use();
		m_model->draw(m_tex);
	}
	else
	{
		m_shader->setVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);
		m_tex.bind();
		m_shader->setInt("albedoMap", 0);
		mNormal.bind(1);
		m_shader->setInt("normalMap", 1);
		mMetallic.bind(2);
		m_shader->setInt("metallicMap", 2);
		mRoughness.bind(3);
		m_shader->setInt("roughnessMap", 3);
		mAO.bind(4);
		m_shader->setInt("aoMap", 4);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		m_shader->setInt("shadowMap", 5);
		m_model->draw(m_tex);

#ifdef _DEBUG
		m_aabb->Render(viewMat, proj, modelMat, m_aabbColor);
#endif
	}
}

void Player::Update(float dt)
{
	//   UpdateAABB();
	ComputeAudioWorldTransform();
	UpdateComponents(dt);

	if (playerShootAudioCooldown > 0.0f)
	{
		playerShootAudioCooldown -= dt;
	}


	if (playGameStartAudioTimer > 0.0f)
	{
		playGameStartAudioTimer -= dt;
	}

	if (playGameStartAudio && playGameStartAudioTimer < 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};
		std::uniform_int_distribution<> distrib(1, 2);
		int randomIndex = distrib(gen);
		if (randomIndex == 1)
			takeDamageAC->PlayEvent("event:/Player2_Game Start");
		else
			takeDamageAC->PlayEvent("event:/Player2_Game Start2");
		playGameStartAudio = false;
	}

	if (destAnim != 0 && mVelocity == 0.0f)
	{
		SetSourceAnimNum(destAnim);
		SetDestAnimNum(0);
		resetBlend = true;
		//		m_blendFactor = 0.0f;
		blendAnim = true;
		prevDirection = STATIONARY;
	}

	if (resetBlend)
	{
		blendAnim = true;
		blendFactor = 0.0f;
		resetBlend = false;
	}

	if (blendAnim)
	{
		blendFactor += (1.0f - blendFactor) * blendSpeed * dt;

		if (blendFactor > 1.0f)
			blendFactor = 1.0f;

		SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), 1.0f, blendFactor, false);
		if (blendFactor >= 1.0f)
		{
			blendAnim = false;
			blendFactor = 0.0f;
			//SetSourceAnimNum(GetDestAnimNum());
		}
	}
	else
	{
		blendFactor = 0.0f;
		SetAnimation(GetSourceAnimNum(), 1.0f, 1.0f, false);
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
	PlayerFront = normalize(front);
	PlayerRight = normalize(cross(PlayerFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	PlayerUp = normalize(cross(PlayerRight, PlayerFront));
}

void Player::UpdatePlayerAimVectors()
{
	glm::vec3 front;
	front.x = glm::cos(glm::radians(m_yaw - 90.0f)) * glm::cos(glm::radians(aimPitch));
	front.y = glm::sin(glm::radians(aimPitch));
	front.z = glm::sin(glm::radians(m_yaw - 90.0f)) * glm::cos(glm::radians(aimPitch));

	PlayerAimFront = normalize(front);
	PlayerAimRight = normalize(cross(PlayerAimFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	PlayerAimUp = normalize(cross(PlayerAimRight, PlayerAimFront));
}

void Player::PlayerProcessKeyboard(CameraMovement direction, float deltaTime)
{
	mVelocity = MovementSpeed * deltaTime;

	if (direction == FORWARD)
	{
		m_position += PlayerFront * mVelocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		if (destAnim != 2 && prevDirection != direction)
		{
			SetSourceAnimNum(destAnim);
			SetDestAnimNum(2);
			blendAnim = true;
			resetBlend = true;
		}
	}
	if (direction == BACKWARD)
	{
		m_position -= PlayerFront * mVelocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		if (destAnim != 2 && prevDirection != direction)
		{
			SetSourceAnimNum(destAnim);
			SetDestAnimNum(2);
			blendAnim = true;
			resetBlend = true;
		}
	}
	if (direction == LEFT)
	{
		m_position -= PlayerRight * mVelocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		if (destAnim != 4 && prevDirection != direction)
		{
			SetSourceAnimNum(destAnim);
			SetDestAnimNum(4);
			blendAnim = true;
			resetBlend = true;
		}
	}
	if (direction == RIGHT)
	{
		m_position += PlayerRight * mVelocity;
		m_recomputeWorldTransform = true;
		ComputeAudioWorldTransform();
		UpdateComponents(deltaTime);
		if (destAnim != 5 && prevDirection != direction)
		{
			SetSourceAnimNum(destAnim);
			SetDestAnimNum(5);
			blendAnim = true;
			resetBlend = true;
		}
	}

	prevDirection = direction;
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
	m_model->playAnimation(animNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor,
                          bool playAnimBackwards)
{
	m_model->playAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetPlayerState(PlayerState newState)
{
	mPlayerState = newState;
	if (mPlayerState == MOVING)
		UpdatePlayerVectors();
	else if (mPlayerState == AIMING)
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

	if (playerShootAudioCooldown < 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{rd()};

		std::uniform_int_distribution<> distrib(1, 2);
		int randomIndex = distrib(gen);

		if (randomIndex == 1)
			shootAC->PlayEvent("event:/Player2_Firing Weapon");
		else
			shootAC->PlayEvent("event:/Player2_Firing Weapon2");
		playerShootAudioCooldown = 2.0f;
	}
	//std::string clipName = "event:/player2_Firing Weapon";
	//Speak(clipName, 1.0f, 0.5f);


	SetDestAnimNum(3);
	blendFactor = 0.0f;
	blendAnim = true;
	UpdatePlayerVectors();
	UpdatePlayerAimVectors();

	glm::vec3 rayO = GetShootPos();
	glm::vec3 rayD;
	float dist = GetShootDistance();

	auto clipCoords = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);

	glm::vec4 cameraCoords = inverse(projection) * clipCoords;
	cameraCoords /= cameraCoords.w;

	glm::vec4 worldCoords = inverse(view) * cameraCoords;
	glm::vec3 rayEnd = glm::vec3(worldCoords) / worldCoords.w;

	rayD = normalize(rayEnd - rayO);

	auto hitPoint = glm::vec3(0.0f);

	GameManager* gmeMgr = GetGameManager();
	//gmeMgr->GetPhysicsWorld()->RayEnemyIntersect(rayO, rayD, hitPoint);

	bool hit = false;
	hit = gmeMgr->GetPhysicsWorld()->RayIntersect(rayO, rayD, hitPoint, aabb);

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

void Player::OnHit()
{
	Logger::Log(1, "Player Hit!");
	setAABBColor(glm::vec3(1.0f, 0.0f, 1.0f));
	TakeDamage(0.4f);

	std::random_device rd;
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<> distrib(1, 3);
	int randomIndex = distrib(gen);
	std::string clipName = "event:/player2_Taking Damage" + std::to_string(randomIndex);
	takeDamageAC->PlayEvent(clipName);
}

void Player::OnDeath()
{
	Logger::Log(1, "Player Died!");
	m_isDestroyed = true;
	deathAC->PlayEvent("event:/Player2_Death");
}

void Player::ResetGame()
{
	m_gameManager->ResetGame();
	playGameStartAudio = true;
}
