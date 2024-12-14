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
		aabb->calculateAABB(m_model->getVertices());
		aabb->setShader(aabbShader);
		aabb->setUpMesh();
		aabb->owner = this;
		aabb->isPlayer = true;
		updateAABB();
		GameManager* gameMgr = GetGameManager();
		gameMgr->GetPhysicsWorld()->addCollider(GetAABB());
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
		m_shader->setVec3("cameraPos", m_gameManager->GetCamera()->Position);
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
		m_aabb->render(viewMat, proj, modelMat, m_aabbColor);
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
	//gmeMgr->GetPhysicsWorld()->rayEnemyIntersect(rayO, rayD, hitPoint);

	bool hit = false;
	hit = gmeMgr->GetPhysicsWorld()->rayIntersect(rayO, rayD, hitPoint, aabb);

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

void Player::renderAABB(glm::mat4 proj, glm::mat4 viewMat, glm::mat4 model, Shader* aabbSdr)
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void*>(nullptr));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	aabbShader->use();

	aabbShader->setMat4("projection", proj);
	aabbShader->setMat4("view", viewMat);
	aabbShader->setMat4("m_model", model);
	aabbShader->setVec3("color", aabbColor);

	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size()));
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Player::OnHit()
{
	Logger::log(1, "Player Hit!");
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
	Logger::log(1, "Player Died!");
	m_isDestroyed = true;
	deathAC->PlayEvent("event:/Player2_Death");
}

void Player::ResetGame()
{
	m_gameManager->ResetGame();
	playGameStartAudio = true;
}
