#include <random>

#include "Enemy.h"
#include "GameManager.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

Enemy::Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
	GameManager* gameMgr, std::string texFilename, int id, EventManager& eventManager, Player& player,
	const EnemyConfig& config, float yaw)
	: GameObject(pos, scale, yaw, sdr, shadowMapShader, applySkinning, gameMgr), m_player(player),
	m_initialPosition(pos), m_id(id), m_eventManager(eventManager),
	m_isPlayerDetected(false), m_isPlayerVisible(false), m_isPlayerInRange(false),
	m_isTakingDamage(false), m_isInCover(false), m_isSeekingCover(false), m_isTakingCover(false),
	m_config(config)
{
	m_isEnemy = true;
	m_combat.accuracy  = m_config.accuracy;
	m_combat.maxHealth = m_config.maxHealth;
	m_combat.health    = m_config.maxHealth;
	m_speed            = m_config.moveSpeed;
	m_aabbScale        = m_config.aabbScale;

	m_id = id;

	m_skinnedMesh.Load(m_config.modelPath);

	SetupGLTFMeshes(m_skinnedMesh.GetModel());

	for (int texID : LoadGLTFTextures(m_skinnedMesh.GetModel()))
		glTextures.push_back(texID);

	if (m_config.hasSkin)
		m_skinnedMesh.InitSkeleton(false);

	size_t enemyModelJointDualQuatBufferSize = GetJointDualQuatsSize() * sizeof(glm::mat2x4);
	m_enemyDualQuatSsBuffer.Init(enemyModelJointDualQuatBufferSize);
	Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n",
		__FUNCTION__, enemyModelJointDualQuatBufferSize);

	ComputeAudioWorldTransform();

	UpdateEnemyCameraVectors();
	UpdateEnemyVectors();

	//std::random_device rd;
	//std::mt19937 gen{ rd() };
	//std::uniform_int_distribution<> distrib(0, (int)m_waypointPositions.size() - 1);
	//int randomIndex = distrib(gen);
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
	Logger::Log(1, "%s: glTF m_model '%s' successfully loaded\n", __FUNCTION__, m_model->GetFilename().c_str());

	size_t enemyModelJointDualQuatBufferSize = m_model->getJointDualQuatsSize() *
		sizeof(glm::mat2x4);
	m_enemyDualQuatSsBuffer.Init(enemyModelJointDualQuatBufferSize);
	Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n",
		__FUNCTION__, enemyModelJointDualQuatBufferSize);
}

void Enemy::SetupGLTFMeshes(tinygltf::Model* model)
{
	meshData.resize(model->meshes.size());

	for (size_t meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex) {
		const tinygltf::Mesh& mesh = model->meshes[meshIndex];
		GLTFMesh gltfMesh;

		for (size_t primIndex = 0; primIndex < mesh.primitives.size(); ++primIndex) {
			const tinygltf::Primitive& primitive = mesh.primitives[primIndex];
			GLTFPrimitive gltfPrim = {};
			gltfPrim.mode = primitive.mode; // usually GL_TRIANGLES

			gltfPrim.material = primitive.material;

			// --- Create VAO ---
			glGenVertexArrays(1, &gltfPrim.vao);
			glBindVertexArray(gltfPrim.vao);

			// --- Upload vertex attributes ---
			for (const auto& attrib : primitive.attributes) {
				const std::string& attribName = attrib.first; // "POSITION", "NORMAL", "TEXCOORD_0", etc.
				int accessorIndex = attrib.second;
				const tinygltf::Accessor& accessor = model->accessors[accessorIndex];
				const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];

				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);

				if (attribName == "POSITION") {
					int numPositionEntries = static_cast<int>(accessor.count);
					Logger::Log(1, "%s: loaded %i vertices from glTF file\n", __FUNCTION__,
						numPositionEntries);

					// Extract vertices
					const float* positions = reinterpret_cast<const float*>(
						buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

					for (int i = 0; i < numPositionEntries; i++)
					{
						gltfPrim.verts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
						gltfPrim.vertexCount++;
						verts.push_back(glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]));
					}
				}

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				size_t dataSize = accessor.count * tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);

				glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

				// Determine attribute layout location (you must match your shader locations)
				GLint location = -1;
				if (attribName == "POSITION") location = 0;
				else if (attribName == "NORMAL") location = 1;
				else if (attribName == "TEXCOORD_0") location = 2;
				else if (attribName == "JOINTS_0") location = 3;
				else if (attribName == "WEIGHTS_0") location = 4;

				Logger::Log(1, "%s: loading attribute: %s\n", __FUNCTION__, attribName);

				if (location >= 0) {
					GLint numComponents = tinygltf::GetNumComponentsInType(accessor.type); // e.g. VEC3 -> 3
					GLenum glType = accessor.componentType; // GL_FLOAT, GL_UNSIGNED_SHORT, etc.

					if (attribName == "JOINTS_0")
					{
						glEnableVertexAttribArray(location);
						glVertexAttribIPointer(
							location,
							numComponents,
							glType,
							bufferView.byteStride ? bufferView.byteStride : 0,
							(const void*)0
						);
					}
					else
					{
						glEnableVertexAttribArray(location);
						glVertexAttribPointer(
							location,
							numComponents,
							glType,
							accessor.normalized ? GL_TRUE : GL_FALSE,
							bufferView.byteStride ? bufferView.byteStride : 0,
							(const void*)0
						);
					}
				}


			}


			if (primitive.indices >= 0) {
				// Get the accessor, bufferview, and buffer for the index data
				const tinygltf::Accessor& indexAccessor = model->accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = model->bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];



				// Pointer to the actual index data
				const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

				// Loop through and extract indices based on the component type
				for (size_t i = 0; i < indexAccessor.count; ++i) {
					switch (indexAccessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						gltfPrim.indices.push_back(static_cast<unsigned int>(reinterpret_cast<const uint8_t*>(dataPtr)[i]));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						gltfPrim.indices.push_back(static_cast<unsigned int>(reinterpret_cast<const uint16_t*>(dataPtr)[i]));
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						gltfPrim.indices.push_back(reinterpret_cast<const uint32_t*>(dataPtr)[i]);
						break;
					}
					default:
						Logger::Log(1, " << indexAccessor.componentType, %zu", indexAccessor.componentType);
						break;
					}
				}
			}
			else {
				// No index buffer: assume the primitive is non-indexed (each vertex is used once)
				int posAccessorIndex = primitive.attributes.at("POSITION");
				const tinygltf::Accessor& posAccessor = model->accessors[posAccessorIndex];
				for (size_t i = 0; i < posAccessor.count; ++i) {
					gltfPrim.indices.push_back(static_cast<unsigned int>(i));
				}
			}

			if (!gltfPrim.indices.empty()) {
				glGenBuffers(1, &gltfPrim.indexBuffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gltfPrim.indexBuffer);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					gltfPrim.indices.size() * sizeof(unsigned int),
					gltfPrim.indices.data(),
					GL_STATIC_DRAW);

				gltfPrim.indexCount = static_cast<GLsizei>(gltfPrim.indices.size());
				gltfPrim.indexType = GL_UNSIGNED_INT;
			}
			else {
				gltfPrim.indexBuffer = 0;
				gltfPrim.indexCount = 0;
			}

			glBindVertexArray(0);

			gltfMesh.primitives.push_back(gltfPrim);
		}

		meshData[meshIndex] = gltfMesh;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::vector<GLuint> Enemy::LoadGLTFTextures(tinygltf::Model* model) {
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

//	m_ao.LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/src/Assets/Models/New/Updated/Atlas_00001.png", false);


	glBindTexture(GL_TEXTURE_2D, 0);
	return textureIDs;
}



void Enemy::DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos) {
	glDisable(GL_CULL_FACE);
	int texIndex = 1;
	for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
		for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
			const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];

			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, m_position);
			if (m_config.rotateOnDraw)
				modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			
			modelMat = glm::scale(modelMat, m_scale);
			m_aabb->Render(viewMat, projMat, modelMat, glm::vec3(1.0f, 1.0f, 0.0f));

			m_shader->Use();
			std::vector<glm::mat4> matrixData;
			matrixData.push_back(viewMat);
			matrixData.push_back(projMat);
			matrixData.push_back(modelMat);
			m_uniformBuffer.UploadUboData(matrixData, 0);
			m_shader->SetVec3("cameraPos", camPos);

			m_enemyDualQuatSsBuffer.UploadSsboData(getJointDualQuats(), 2);

			bool hasTexture = false;
			glBindVertexArray(prim.vao);
			int matIndex = prim.material;
			if (matIndex >= 0 && matIndex < static_cast<int>(m_skinnedMesh.GetModel()->materials.size())) {
				const tinygltf::Material& mat = m_skinnedMesh.GetModel()->materials[matIndex];
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
				}
				else {
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

void Enemy::DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture, glm::vec3 camPos)
{
	DrawGLTFModel(viewMat, proj, camPos);
}

void Enemy::Update(bool shouldUseEDBT, bool isPaused, bool isTimeScaled)
{
	if (!m_combat.isDead || !m_isDestroyed)
	{
		if (shouldUseEDBT)
		{
#ifdef TRACY_ENABLE
			ZoneScopedN("EDBT Update");
#endif
			// --- Detection state machine ---
			if (CanSeePlayer())
			{
				m_lastKnownPlayerPos = m_player.GetPosition();
				m_losLostTimer       = 0.0f;
				m_isSearching        = false;
				if (!m_isPlayerDetected)
					DetectPlayer();
			}
			else if (m_isPlayerDetected)
			{
				m_losLostTimer += m_dt;
				m_isSearching   = true;
				if (m_losLostTimer >= m_config.alertTimeout)
				{
					m_isPlayerDetected    = false;
					m_isSearching         = false;
					m_losLostTimer        = 0.0f;
					m_hasSearchWanderTarget = false;
				}
			}

			// Clear movement target each frame; behaviour tree will re-set it if needed
			m_hasMovementTarget = false;

			m_behaviorTree->Tick();


			if (m_combat.hasShot)
			{
				m_enemyRayDebugRenderTimer -= m_dt;
				m_combat.shootCooldown -= m_dt;
			}
			if (m_combat.shootCooldown <= 0.0f)
			{
				m_combat.hasShot = false;
			}

			if (m_combat.shootAudioCooldown > 0.0f)
			{
				m_combat.shootAudioCooldown -= m_dt;
			}
		}
		else
		{
			m_decisionDelayTimer -= m_dt;
		}

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

	float animSpeedDivider = 1.0f;

	if (isPaused)
		animSpeedDivider = 0.0f;

	if (isTimeScaled)
		animSpeedDivider = 0.25f;

	if (m_blendAnim)
	{
		m_blendFactor += (1.0f - m_blendFactor) * m_blendSpeed * m_dt;
		if (m_blendFactor > 1.0f)
			m_blendFactor = 1.0f;
	//	SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), animSpeedDivider / 2.0f, m_blendFactor, false);
		if (m_blendFactor >= 1.0f)
		{
			m_blendAnim = false;
			m_blendFactor = 0.0f;
			SetSourceAnimNum(GetDestAnimNum());
		}
	}
	else
	{
	//	SetAnimation(GetSourceAnimNum(), animSpeedDivider, 1.0f, false);
		m_blendFactor = 0.0f;
	}

	static bool printed = false;
	if (!printed)
	{
		Logger::Log(1, "Enemy Update running, anim clips = %i, jointDQs = %i\n",
			m_skinnedMesh.GetAnimClipsSize(),
			m_skinnedMesh.GetJointDualQuatsSize());
		printed = true;
	}

	if (m_config.hasSkin)
		PlayAnimation(m_config.animWalk, 1.0f, 1.0f, false);
}

void Enemy::OnEvent(const Event& event)
{
	if (auto e = dynamic_cast<const PlayerDetectedEvent*>(&event))
	{
		if (e->m_npcId != m_id)
		{
			float dist = glm::distance(GetPosition(), e->m_detectorPos);
			if (dist <= m_config.alertRadius)
			{
				m_isPlayerDetected      = true;
				m_lastKnownPlayerPos    = m_player.GetPosition();
				m_losLostTimer          = 0.0f;
				m_isSearching           = false;
				Logger::Log(1, "[Alert] Enemy %d alerted by enemy %d (dist %.1f)\n",
				            m_id, e->m_npcId, dist);
			}
		}
	}
	else if (auto e = dynamic_cast<const NPCDamagedEvent*>(&event))
	{
		if (e->m_npcId != m_id)
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
		std::mt19937 gen{ rd() };
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
		if (e->m_npcId != m_id)
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
	if (newPos != m_position)
	{
		SetAnimNum(0);
	}
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
	//if (path.empty())
	//{
	//	return;
	//}

	//const float tolerance = 0.1f; // Smaller tolerance for better alignment
	//const float agentRadius = 0.5f; // Adjust this value to match the agent's radius

	//if (!reachedPlayer && !inCover)
	//{
	//	if (!resetBlend && destAnim != 1)
	//	{
	//		SetSourceAnimNum(destAnim);
	//		SetDestAnimNum(1);
	//		blendAnim = true;
	//		resetBlend = true;
	//	}
	//	//SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
	//}

	//	if (IsPatrolling() || EDBTState == "Patrol" || EDBTState == "PATROL")
	//	{
	//		reachedDestination = true;
	//		std::random_device rd;
	//		std::mt19937 gen{ rd() };
	//		std::uniform_int_distribution<> distrib(1, 2);
	//		int randomIndex = distrib(gen);
	//		std::uniform_real_distribution<> distribReal(2.0, 3.0);
	//		int randomFloat = distribReal(gen);

	//		int enemyAudioIndex;
	//		if (id_ == 3)
	//		{
	//			enemyAudioIndex = 4;
	//		}
	//		else
	//		{
	//			enemyAudioIndex = id_;
	//		}

	//		std::string clipName = "event:/enemy" + std::to_string(enemyAudioIndex) + "_Patrolling" + std::to_string(randomIndex);
	//		Speak(clipName, 2.0f, randomFloat);
	//	}

	//	if (isTakingCover_)
	//	{
	//		if (glm::distance(getPosition(), selectedCover_->worldPosition) < grid_->GetCellSize() / 4.0f)
	//		{
	//			reachedCover = true;
	//			isTakingCover_ = false;
	//			isInCover_ = true;
	//			//			grid_->OccupyCell(selectedCover_->gridX, selectedCover_->gridZ, id_);

	//			if (!resetBlend && destAnim != 2)
	//			{
	//				SetSourceAnimNum(destAnim);
	//				SetDestAnimNum(2);
	//				blendAnim = true;
	//				resetBlend = true;
	//				//SetAnimation(GetAnimNum(), 1.0f, blendFactor, playAnimBackwards);
	//			}
	//		}
	//		else
	//		{
	//			setPosition(getPosition() + (glm::normalize(selectedCover_->worldPosition - getPosition()) * 2.0f) * speed * deltaTime);
	//		}
	//	}

	//	return; // Stop moving if the agent has reached its destination
	//}

	//// Calculate the target position from the current path node
	//glm::vec3 targetPos = glm::vec3(path[pathIndex_].x * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f, getPosition().y, path[pathIndex_].y * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f);

	//// Calculate the direction to the target position
	//glm::vec3 direction = glm::normalize(targetPos - getPosition());

	////enemy.Yaw = glm::degrees(glm::acos(glm::dot(glm::normalize(enemy.Front), direction)));
	//yaw = glm::degrees(glm::atan(direction.z, direction.x));
	//mRecomputeWorldTransform = true;

	//UpdateEnemyVectors();

	//// Calculate the new position
	//glm::vec3 newPos = getPosition() + direction * speed * deltaTime;

	//// Ensure the new position is not within an obstacle by checking the bounding box
	//bool isObstacleFree = true;

	//	if (!isObstacleFree) break;
	//}

	//if (isObstacleFree) {

	//	}

	//	if (isObstacleFree) {
	//		setPosition(newPos);
	//	}
	//}


	//}
}

void Enemy::SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
	m_model->PlayAnimation(animNum, speedDivider, blendFactor, playBackwards);
}

void Enemy::SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards)
{
	m_model->PlayAnimation(srcAnimNum, destAnimNum, speedDivider, blendFactor, playBackwards);
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
	m_enemyShootDir = (m_player.GetPosition() - GetPosition()) + accuracyOffset;
	auto hitPoint = glm::vec3(0.0f);

	glm::vec3 playerDir = normalize(m_player.GetPosition() - GetPosition());

	m_yaw = glm::degrees(glm::atan(playerDir.z, playerDir.x));
	UpdateEnemyVectors();

	bool hit = false;
	hit = GetGameManager()->GetPhysicsWorld()->RayIntersect(m_enemyShootPos, m_enemyShootDir, m_enemyHitPoint, m_aabb);

	if (hit)
	{
		m_combat.hasHit = true;
	}
	else
	{
		m_combat.hasHit = false;
	}

	if (!m_resetBlend && m_destAnim != 2)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(2);
		m_blendAnim = true;
		m_resetBlend = true;
	}

	//m_shootAc->PlayEvent("event:/EnemyShoot");
	if (m_combat.shootAudioCooldown <= 0.0f)
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
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
		m_combat.shootAudioCooldown = 3.0f;
	}


	m_enemyRayDebugRenderTimer = 0.3f;
	m_combat.hasShot = true;
	m_combat.shootCooldown = 0.5f;
}

void Enemy::SetUpAABB()
{
	m_aabb = new AABB();
	m_aabb->CalculateAABB(verts);
	m_aabb->SetShader(m_aabbShader);
	m_aabb->SetUpMesh();
	m_aabb->SetOwner(this);
	m_aabb->SetIsEnemy(true);
	m_gameManager->GetPhysicsWorld()->AddCollider(GetAABB());
	m_gameManager->GetPhysicsWorld()->AddEnemyCollider(GetAABB());
	UpdateAABB();
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
	std::mt19937 gen{ rd() };
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
	m_eventManager.Publish(NPCDamagedEvent{ m_id });
}

void Enemy::TakeDamage(float damage)
{
	m_combat.ApplyDamage(damage);
	if (m_combat.isDead)
	{
		OnDeath();
		return;
	}

	if (!m_resetBlend && m_destAnim != 3 && m_damageTimer <= 0.0f)
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
	std::mt19937 gen{ rd() };
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
	m_eventManager.Publish(NPCDiedEvent{ m_id });
	m_combat.isDead = true;
	m_isDestroyed = true;
}

void Enemy::UpdateAABB()
{
	glm::mat4 modelMatrix = translate(glm::mat4(1.0f), m_position) *
		rotate(glm::mat4(1.0f), glm::radians(m_yaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::scale(glm::mat4(1.0f), m_scale);
	m_aabb->Update(modelMatrix);
};

void Enemy::ScoreCoverLocations(Player& player)
{
	float bestScore = -100000.0f;
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
	std::mt19937 gen{ rd() };
	std::uniform_int_distribution<> distrib(0, (int)availableWaypoints.size() - 1);
	int randomIndex = distrib(gen);
	return availableWaypoints[randomIndex];
}





void Enemy::HasDealtDamage()
{
	std::random_device rd;
	std::mt19937 gen{ rd() };
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

	// Detection / search state
	m_isSearching          = false;
	m_losLostTimer         = 0.0f;
	m_lastKnownPlayerPos   = glm::vec3(0.0f);

	// Patrol wander state
	m_patrolWanderTarget    = glm::vec3(0.0f);
	m_patrolWaitTimer       = 0.0f;
	m_isPatrolWaiting       = false;
	m_hasPatrolWanderTarget = false;

	// Search wander state
	m_searchWanderTarget    = glm::vec3(0.0f);
	m_hasSearchWanderTarget = false;

	// Movement target
	m_movementTarget    = glm::vec3(0.0f);
	m_hasMovementTarget = false;

	// Stuck detection
	m_stuckCheckPos   = glm::vec3(0.0f);
	m_stuckCheckTimer = STUCK_CHECK_INTERVAL;

	m_takingDamage = false;
	m_damageTimer = 0.0f;
	m_dyingTimer = 0.0f;
	m_coverTimer = 0.0f;
	m_reachedCover = false;

	m_reachedDestination = false;
	m_reachedPlayer = false;

	m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	m_animNum = 1;
	m_sourceAnim = 1;
	m_destAnim = 1;
	m_destAnimSet = false;
	m_blendSpeed = 5.0f;
	m_blendFactor = 0.0f;
	m_blendAnim = false;
	m_resetBlend = false;

	m_combat.shootCooldown = 0.0f;
	m_enemyRayDebugRenderTimer = 0.3f;
	m_combat.hasShot = false;
	m_combat.hasHit = false;
	m_playerIsVisible = false;
}

void Enemy::VacatePreviousCell()
{

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

	// Player detected sequence (attack — only when actively seeing or recently saw player, NOT searching)
	auto playerDetectedSequence = std::make_shared<SequenceNode>();
	playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsPlayerDetected(); }));
	playerDetectedSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsSearching(); }));

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

	// Search sequence — wander near last known position while alert timer ticks down
	auto searchSequence = std::make_shared<SequenceNode>();
	searchSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	searchSequence->AddChild(std::make_shared<ConditionNode>([this]() { return IsSearching(); }));
	searchSequence->AddChild(std::make_shared<ActionNode>([this]() { return Search(); }));

	// Patrol action — random navmesh wander when fully unalerted
	auto patrolSequence = std::make_shared<SequenceNode>();
	patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthZeroOrBelow(); }));
	patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsPlayerDetected(); }));
	patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsSearching(); }));
	patrolSequence->AddChild(std::make_shared<ConditionNode>([this]() { return !IsHealthBelowThreshold(); }));
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

	// Add sequences to attack selector: attack → search → patrol
	attackSelector->AddChild(playerDetectedSequence);
	attackSelector->AddChild(searchSequence);
	attackSelector->AddChild(patrolSequence);

	takingDamageSequence->AddChild(attackSelector);

	// Add sequences to root
	root->AddChild(isDeadSequence);
	root->AddChild(takingDamageSequence);
	root->AddChild(seekCoverSequence);
	root->AddChild(inCoverSequence);
	root->AddChild(suppressionFireSequence);
	root->AddChild(attackSelector);

	m_behaviorTree = root;
}

void Enemy::DetectPlayer()
{
	m_isPlayerDetected = true;
	std::random_device rd;
	std::mt19937 gen{ rd() };
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

	m_eventManager.Publish(PlayerDetectedEvent{ m_id, GetPosition() });
}

bool Enemy::IsDead()
{
	return m_combat.isDead;
}

bool Enemy::IsHealthZeroOrBelow()
{
	return m_combat.health <= 0;
}

bool Enemy::IsTakingDamage()
{
	return m_isTakingDamage;
}

bool Enemy::IsPlayerDetected()
{
	return m_isPlayerDetected;
}

bool Enemy::CanSeePlayer()
{
	glm::vec3 toPlayer = m_player.GetPosition() - GetPosition();
	float dist = glm::length(toPlayer);

	if (dist > m_config.sightRange) return false;

	// FOV cone check — skipped when sightFovDeg >= 180 (all-around, e.g. Drone)
	if (m_config.sightFovDeg < 180.0f)
	{
		glm::vec3 flatFront = glm::vec3(m_front.x, 0.0f, m_front.z);
		if (glm::length(flatFront) < 0.001f) flatFront = glm::vec3(1.0f, 0.0f, 0.0f);
		flatFront = glm::normalize(flatFront);

		glm::vec3 flatDir = glm::vec3(toPlayer.x, 0.0f, toPlayer.z);
		if (glm::length(flatDir) < 0.001f) return false;
		flatDir = glm::normalize(flatDir);

		float cosAngle = glm::dot(flatFront, flatDir);
		if (cosAngle < std::cos(glm::radians(m_config.sightFovDeg)))
			return false;
	}

	// Line-of-sight raycast through AABB colliders
	glm::vec3 rayOrigin = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 rayDir    = glm::normalize(m_player.GetPosition() - rayOrigin);
	glm::vec3 hitPoint;
	return m_gameManager->GetPhysicsWorld()->CheckPlayerVisibility(rayOrigin, rayDir, hitPoint, m_aabb);
}

bool Enemy::IsPlayerVisible()
{
	m_isPlayerVisible = CanSeePlayer();
	return m_isPlayerVisible;
}

bool Enemy::IsCooldownComplete()
{
	return m_combat.shootCooldown <= 0.0f;
}

bool Enemy::IsHealthBelowThreshold()
{
	return m_combat.health < 40;
}

bool Enemy::IsPlayerInRange()
{
	float playerEnemyDistance = distance(GetPosition(), m_player.GetPosition());

	glm::vec3 tempEnemyShootPos = GetPosition() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 tempEnemyShootDir = normalize(m_player.GetPosition() - GetPosition());
	auto hitPoint = glm::vec3(0.0f);

	if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
	{
		DetectPlayer();
		m_isPlayerInRange = true;
	}

	return m_isPlayerInRange;
}

bool Enemy::IsSearching()
{
	return m_isSearching;
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

	m_combat.isDead = true;
	m_eventManager.Publish(NPCDiedEvent{ m_id });
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
			std::mt19937 gen{ rd() };
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
			m_combat.health += 10.0f;
			m_coverTimer = 0.0f;

			if (m_combat.health > 40.0f)
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
	m_isAttacking = true;

	// Drive the crowd agent toward the last known player position
	m_movementTarget    = m_lastKnownPlayerPos;
	m_hasMovementTarget = true;

	// Face the movement direction
	glm::vec3 dir = m_lastKnownPlayerPos - GetPosition();
	if (glm::length(dir) > 0.1f)
	{
		dir  = glm::normalize(dir);
		m_yaw = glm::degrees(glm::atan(dir.z, dir.x));
		UpdateEnemyVectors();
	}

	// Walk animation
	if (!m_resetBlend && m_destAnim != m_config.animWalk)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(m_config.animWalk);
		m_blendAnim = true;
		m_resetBlend = true;
	}


	if (!IsPlayerVisible())
	{
		if (m_playNotVisibleAudio)
		{
			std::random_device rd;
			std::mt19937 gen{ rd() };
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
		std::mt19937 gen{ rd() };
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

		m_eventManager.Publish(NPCTakingCoverEvent{ m_id });
	}

	m_isTakingCover = true;

	//if (m_grid->GetGrid()[m_selectedCover->m_gridX][m_selectedCover->m_gridZ].IsOccupied())
	//{
	//	ScoreCoverLocations(m_player);
	//}


	VacatePreviousCell();

	//for (glm::ivec2& cell : m_currentPath)
	//{
	//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
	//		m_currentPath = m_grid->FindPath(
	//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
	//			glm::ivec2(m_selectedCover->m_worldPosition.x / m_grid->GetCellSize(), m_selectedCover->m_worldPosition.z / m_grid->GetCellSize()),
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
	std::mt19937 gen{ rd() };
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
	m_state        = "Patrolling";
	m_isAttacking  = false;
	m_isPatrolling = true;

	// --- Waiting at waypoint ---
	if (m_isPatrolWaiting)
	{
		m_patrolWaitTimer -= m_dt;
		m_hasMovementTarget = false;

		if (!m_resetBlend && m_destAnim != m_config.animIdle)
		{
			SetSourceAnimNum(m_destAnim);
			SetDestAnimNum(m_config.animIdle);
			m_blendAnim  = true;
			m_resetBlend = true;
		}

		if (m_patrolWaitTimer <= 0.0f)
		{
			m_isPatrolWaiting       = false;
			m_hasPatrolWanderTarget = false;
		}
		return NodeStatus::Running;
	}

	// --- Stuck detection: if we haven't moved in STUCK_CHECK_INTERVAL seconds, abandon target ---
	if (m_hasPatrolWanderTarget)
	{
		m_stuckCheckTimer -= m_dt;
		if (m_stuckCheckTimer <= 0.0f)
		{
			float moved = glm::distance(glm::vec3(GetPosition().x, 0.0f, GetPosition().z),
			                            glm::vec3(m_stuckCheckPos.x, 0.0f, m_stuckCheckPos.z));
			if (moved < STUCK_MOVE_THRESHOLD)
				m_hasPatrolWanderTarget = false; // pick a new target next frame
			m_stuckCheckPos   = GetPosition();
			m_stuckCheckTimer = STUCK_CHECK_INTERVAL;
		}
	}

	// --- Pick a new wander target if needed ---
	if (!m_hasPatrolWanderTarget)
	{
		m_stuckCheckPos   = GetPosition();
		m_stuckCheckTimer = STUCK_CHECK_INTERVAL;

		std::mt19937 gen{ std::random_device{}() };
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
		std::uniform_real_distribution<float> radiusDist(5.0f, m_config.patrolWanderRadius);

		NavMeshManager* nav = m_gameManager->GetNavMeshManager();
		glm::vec3 snapped;
		bool found = false;

		for (int attempt = 0; attempt < 8; ++attempt)
		{
			float     angle     = angleDist(gen);
			float     radius    = radiusDist(gen);
			glm::vec3 candidate = m_initialPosition +
			                      glm::vec3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);

			if (nav->SnapToNavMesh(candidate, snapped, 10.0f, 20.0f) &&
			    nav->HasPathTo(GetPosition(), snapped))
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			m_patrolWanderTarget    = snapped;
			m_hasPatrolWanderTarget = true;
		}
		else
		{
			// No reachable target found — stand idle and try again next frame
			m_hasMovementTarget = false;
			return NodeStatus::Running;
		}
	}

	// --- Check if we've reached the target ---
	glm::vec3 flatPos    = glm::vec3(GetPosition().x,         0.0f, GetPosition().z);
	glm::vec3 flatTarget = glm::vec3(m_patrolWanderTarget.x,  0.0f, m_patrolWanderTarget.z);
	if (glm::distance(flatPos, flatTarget) < 3.0f)
	{
		std::mt19937 gen{ std::random_device{}() };
		std::uniform_real_distribution<float> waitDist(1.0f, 3.0f);
		m_patrolWaitTimer       = waitDist(gen);
		m_isPatrolWaiting       = true;
		m_hasPatrolWanderTarget = false;
		m_hasMovementTarget     = false;
		return NodeStatus::Running;
	}

	// --- Walk toward target ---
	m_movementTarget    = m_patrolWanderTarget;
	m_hasMovementTarget = true;

	glm::vec3 dir = glm::normalize(m_patrolWanderTarget - GetPosition());
	m_yaw = glm::degrees(glm::atan(dir.z, dir.x));
	UpdateEnemyVectors();

	if (!m_resetBlend && m_destAnim != m_config.animWalk)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(m_config.animWalk);
		m_blendAnim  = true;
		m_resetBlend = true;
	}

	return NodeStatus::Running;
}

NodeStatus Enemy::Search()
{
	m_state       = "Searching";
	m_isAttacking = false;

	// Pick a new wander point near the last known player position when needed
	glm::vec3 flatPos    = glm::vec3(GetPosition().x,          0.0f, GetPosition().z);
	glm::vec3 flatTarget = glm::vec3(m_searchWanderTarget.x,   0.0f, m_searchWanderTarget.z);
	bool needTarget = !m_hasSearchWanderTarget ||
	                  glm::distance(flatPos, flatTarget) < 2.5f;

	if (needTarget)
	{
		std::mt19937 gen{ std::random_device{}() };
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
		float searchRadius = m_config.sightRange * 0.5f;
		std::uniform_real_distribution<float> radiusDist(3.0f, searchRadius);

		float     angle     = angleDist(gen);
		float     radius    = radiusDist(gen);
		glm::vec3 candidate = m_lastKnownPlayerPos +
		                      glm::vec3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);

		glm::vec3 snapped;
		if (m_gameManager->GetNavMeshManager()->SnapToNavMesh(candidate, snapped, 10.0f, 20.0f))
		{
			m_searchWanderTarget    = snapped;
			m_hasSearchWanderTarget = true;
		}
		else
		{
			m_hasMovementTarget = false;
			return NodeStatus::Running;
		}
	}

	// Walk toward wander target
	m_movementTarget    = m_searchWanderTarget;
	m_hasMovementTarget = true;

	glm::vec3 dir = glm::normalize(m_searchWanderTarget - GetPosition());
	m_yaw = glm::degrees(glm::atan(dir.z, dir.x));
	UpdateEnemyVectors();

	if (!m_resetBlend && m_destAnim != m_config.animWalk)
	{
		SetSourceAnimNum(m_destAnim);
		SetDestAnimNum(m_config.animWalk);
		m_blendAnim  = true;
		m_resetBlend = true;
	}

	return NodeStatus::Running;
}

NodeStatus Enemy::InCoverAction()
{
	m_coverTimer += m_dt;
	if (m_coverTimer > 2.5f)
	{
		m_combat.health += 10.0f;
		m_coverTimer = 0.0f;

		if (m_combat.health > 40.0f)
		{
			m_isInCover = false;
			std::random_device rd;
			std::mt19937 gen{ rd() };
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
	glm::vec3 rayDirection = normalize(m_player.GetPosition() - rayOrigin);
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
	m_combat.isDead = true;
	m_isDestroyed = true;
	m_state = "Dead";
	m_eventManager.Publish(NPCDiedEvent{ m_id });
	return NodeStatus::Success;
}

#ifdef NPC_RL_QLEARNING
float Enemy::DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize)
{
	if (currentSize >= targetSize)
	{
		return minRate;
	}
	float decayedRate = minRate + (initialRate - minRate) * (1.0f - static_cast<float>(currentSize) / targetSize);
	return decayedRate;
}
#endif // NPC_RL_QLEARNING