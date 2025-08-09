#include "Player.h"
#include "GameManager.h"

std::vector<GLuint> Player::LoadGLTFTextures(tinygltf::Model* model) {
	std::vector<GLuint> textureIDs(model->textures.size(), 0);

	for (size_t i = 0; i < model->textures.size(); ++i) {
		const tinygltf::Texture& tex = model->textures[i];
		if (tex.source < 0 || tex.source >= model->images.size()) {
			continue; // Invalid texture
		}

		const tinygltf::Image& image = model->images[tex.source];

		GLuint texID;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		GLenum format = GL_RGBA;
		if (image.component == 1) format = GL_RED;
		else if (image.component == 2) format = GL_RG;
		else if (image.component == 3) format = GL_RGB;
		else if (image.component == 4) format = GL_RGBA;

		GLenum type = (image.bits == 16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

		glTexImage2D(GL_TEXTURE_2D,
			0,
			format,
			image.width,
			image.height,
			0,
			format,
			type,
			image.image.data());

		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		textureIDs[i] = texID;
	}

	m_ao.LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Atlas_00001.png", false);


	glBindTexture(GL_TEXTURE_2D, 0);
	return textureIDs;
}

void Player::DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos) {
	glDisable(GL_CULL_FACE);

	int texIndex = 1;
	for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
		for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
			const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];


			m_shader->Use();
			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, m_position);
			//modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelMat = glm::scale(modelMat, m_scale);
			std::vector<glm::mat4> matrixData;
			matrixData.push_back(viewMat);
			matrixData.push_back(projMat);
			matrixData.push_back(modelMat);
			m_uniformBuffer.UploadUboData(matrixData, 0);
			m_shader->SetVec3("cameraPos", camPos);

			bool hasTexture = false;
			glBindVertexArray(prim.vao);
			int matIndex = prim.material;
			if (matIndex >= 0 && matIndex < playerModel->materials.size()) {
				const tinygltf::Material& mat = playerModel->materials[matIndex];
				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					hasTexture = true;
					texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
				}

				if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
					texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, glTextures[texIndex]);
					m_shader->SetInt("albedoMap", 0);
					m_shader->SetBool("useAlbedo", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("baseColour", 1.0f, 1.0f, 1.0f);
				} else {
					glm::vec3 baseColor = glm::vec3(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2]);
					m_shader->SetBool("useAlbedo", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetVec3("baseColour", baseColor);
				}


				if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index]);
					m_shader->SetInt("metallicRoughnessMap", 1);
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);

				}
				else {
					m_shader->SetBool("useMetallicRoughness", mat.pbrMetallicRoughness.baseColorTexture.index >= 0);
					m_shader->SetFloat("metallicFactor", mat.pbrMetallicRoughness.metallicFactor);
					m_shader->SetFloat("roughnessFactor", mat.pbrMetallicRoughness.roughnessFactor);
				}

				if (mat.normalTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.normalTexture.index]);
					m_shader->SetInt("normalMap", 2);
					m_shader->SetBool("useNormalMap", mat.normalTexture.index >= 0);
				}
				else {
					m_shader->SetBool("useNormalMap", false);
				}

				if (mat.occlusionTexture.index >= 0) {
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, glTextures[mat.occlusionTexture.index]);
					m_shader->SetInt("occlusionTex", 3);
					m_shader->SetBool("useOcclusionMap", mat.occlusionTexture.index >= 0);
				}
				else {
					m_shader->SetInt("occlusionTex", 3);
					m_shader->SetBool("useOcclusionMap", false);
				}

				m_shader->SetBool("useEmissiveFactor", false);
				m_shader->SetVec3("emissiveFactor", 0.0f, 0.0f, 0.0f);
				m_shader->SetFloat("emissiveStrength", 0.0f);
			}

			if (prim.indexBuffer) {
				glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, 0);
			}
			else {
				glDrawArrays(prim.mode, 0, prim.vertexCount);
			}

			glBindVertexArray(0);

		}

		if (texIndex < glTextures.size() - 3)
			texIndex += 3;
	}
}

void Player::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	DrawGLTFModel(viewMat, proj, camPos);
}

//void Player::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat,
//                        GLuint shadowMapTexture, glm::vec3 camPos)
//{
//	auto modelMat = glm::mat4(1.0f);
//	modelMat = translate(modelMat, m_position);
//	modelMat = rotate(modelMat, glm::radians(-m_yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//	modelMat = glm::scale(modelMat, m_scale);
//	std::vector<glm::mat4> matrixData;
//	matrixData.push_back(viewMat);
//	matrixData.push_back(proj);
//	matrixData.push_back(modelMat);
//	matrixData.push_back(lightSpaceMat);
//	m_uniformBuffer.UploadUboData(matrixData, 0);
//
//	m_playerDualQuatSsBuffer.UploadSsboData(m_model->getJointDualQuats(), 2);
//
//	if (m_uploadVertexBuffer)
//	{
//		m_model->uploadVertexBuffers();
//		GameManager* gameMgr = GetGameManager();
//		gameMgr->GetPhysicsWorld()->AddCollider(GetAABB());
//		m_uploadVertexBuffer = false;
//	}
//
//	if (shadowMap)
//	{
//		m_shadowShader->Use();
//		m_model->draw(m_tex);
//	}
//	else
//	{
//		m_shader->SetVec3("cameraPos", m_gameManager->GetCamera()->GetPosition().x, m_gameManager->GetCamera()->GetPosition().y, m_gameManager->GetCamera()->GetPosition().z);
//		m_tex.Bind();
//		m_shader->SetInt("albedoMap", 0);
//		m_normal.Bind(1);
//		m_shader->SetInt("normalMap", 1);
//		m_metallic.Bind(2);
//		m_shader->SetInt("metallicMap", 2);
//		m_roughness.Bind(3);
//		m_shader->SetInt("roughnessMap", 3);
//		m_ao.Bind(4);
//		m_shader->SetInt("aoMap", 4);
//		glActiveTexture(GL_TEXTURE5);
//		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
//		m_shader->SetInt("shadowMap", 5);
//		m_model->draw(m_tex);
//
//#ifdef _DEBUG
//		m_aabb->Render(viewMat, proj, modelMat, m_aabbColor);
//#endif
//	}
//}

void Player::Update(float dt, bool isPaused, bool isTimeScaled)
{
	//UpdateAabb();
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

	//if (m_playGameStartAudio && m_playGameStartAudioTimer < 0.0f)
	//{
	//	std::random_device rd;
	//	std::mt19937 gen{rd()};
	//	std::uniform_int_distribution<> distrib(1, 2);
	//	int randomIndex = distrib(gen);
	//	if (randomIndex == 1)
	//		m_takeDamageAc->PlayEvent("event:/Player2_Game Start");
	//	else
	//		m_takeDamageAc->PlayEvent("event:/Player2_Game Start2");
	//	m_playGameStartAudio = false;
	//}
	//
	//if (m_destAnim != 0 && m_velocity == 0.0f)
	//{
	//	SetSourceAnimNum(m_destAnim);
	//	SetDestAnimNum(0);
	//	m_resetBlend = true;
	//	m_blendAnim = true;
	//	SetPrevDirection(STATIONARY);
	//}
	//
	//if (m_resetBlend)
	//{
	//	m_blendAnim = true;
	//	m_blendFactor = 0.0f;
	//	m_resetBlend = false;
	//}
	//
	//float animSpeedDivider = 1.0f;
	//
	//if (isPaused)
	//	animSpeedDivider = 0.0f;
	//
	//if (isTimeScaled)
	//	animSpeedDivider = 0.25f;
	//
	//if (m_blendAnim)
	//{
	//	m_blendFactor += m_blendSpeed * dt;
	//
	//	SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), animSpeedDivider, m_blendFactor, false);
	//
	//	
	//	if (m_blendFactor >= 1.0f)
	//	{
	//		m_blendAnim = false;
	//		m_resetBlend = true;
	//		SetSourceAnimNum(GetDestAnimNum());
	//		m_resetBlend = true;
	//	}
	//}
	//else
	//{
	//	SetAnimation(m_destAnim, animSpeedDivider, 1.0f, false);
	//}
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
	//m_model->PlayAnimation(animNum, speedDivider, blendFactor, playAnimBackwards);
}

void Player::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor,
                          bool playAnimBackwards)
{
	//m_model->PlayAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playAnimBackwards);
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
	/*m_aabb = new AABB();
	m_aabb->CalculateAABB(playerModel->GetVertices());
	m_aabb->SetShader(m_aabbShader);
	m_aabb->SetUpMesh();
	m_aabb->SetOwner(this);
	m_aabb->SetIsPlayer(true);
	//UpdateAabb();*/
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
