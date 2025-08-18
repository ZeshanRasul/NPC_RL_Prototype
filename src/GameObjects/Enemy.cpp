#include <random>

#include "Enemy.h"
#include "GameManager.h"
#include "src/Tools/Logger.h"

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

Enemy::Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
	GameManager* gameMgr, std::string texFilename, int id, EventManager& eventManager, Player& player,
	float yaw) : GameObject(pos, scale, yaw, sdr, shadowMapShader, applySkinning, gameMgr),m_player(player),
	m_initialPosition(pos), m_id(id), m_eventManager(eventManager),
	m_health(100.0f), m_isPlayerDetected(false), m_isPlayerVisible(false), m_isPlayerInRange(false),
	m_isTakingDamage(false), m_isDead(false), m_isInCover(false), m_isSeekingCover(false), m_isTakingCover(false)
{
	m_isEnemy = true;

	m_id = id;
	enemyModel = new tinygltf::Model;

	std::string modelFilename = "Assets/Models/New_Enemies/Armour7/First.gltf";


	tinygltf::TinyGLTF gltfLoader;
	std::string loaderErrors;
	std::string loaderWarnings;
	bool result = false;

	result = gltfLoader.LoadASCIIFromFile(enemyModel, &loaderErrors, &loaderWarnings,
		modelFilename);

	if (!loaderWarnings.empty()) {
		Logger::Log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__,
			loaderWarnings.c_str());
	}

	if (!loaderErrors.empty()) {
		Logger::Log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__,
			loaderErrors.c_str());
	}

	if (!result) {
		Logger::Log(1, "%s error: could not load file '%s'\n", __FUNCTION__,
			modelFilename.c_str());
	}

	SetupGLTFMeshes(enemyModel);

	for (int texID : LoadGLTFTextures(enemyModel))
		glTextures.push_back(texID);

	//m_tex = m_model->LoadTexture(modelTextureFilename, false);
	//
	//m_normal.LoadTexture(
	//	"Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Normal.png");
	//m_metallic.LoadTexture(
	//	"Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Metallic.png");
	//m_roughness.LoadTexture(
	//	"Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Roughness.png");
	//m_ao.LoadTexture(
		//"Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_AO.png");

	//m_model->uploadIndexBuffer();
	//Logger::Log(1, "%s: glTF m_model '%s' successfully loaded\n", __FUNCTION__, modelFilename.c_str());
	//
	size_t enemyModelJointDualQuatBufferSize = GetJointDualQuatsSize() *
		sizeof(glm::mat2x4);
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

					glEnableVertexAttribArray(location);
					glVertexAttribPointer(location, numComponents, glType,
						accessor.normalized ? GL_TRUE : GL_FALSE,
						bufferView.byteStride ? bufferView.byteStride : 0,
						(const void*)0);
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

	GetJointData();
	GetWeightData();
	GetInvBindMatrices();

	m_nodeCount = (int)enemyModel->nodes.size();
	int rootNode = enemyModel->scenes.at(0).nodes.at(0);
	Logger::Log(1, "%s: model has %i nodes, root node is %i\n", __FUNCTION__, m_nodeCount, rootNode);

	m_nodeList.resize(m_nodeCount);

	m_rootNode = GltfNode::CreateRoot(rootNode);

	m_nodeList.at(rootNode) = m_rootNode;

	GetNodeData(m_rootNode, glm::mat4(1.0f));
	GetNodes(m_rootNode);

	m_rootNode->PrintTree();

	GetAnimations();

	m_additiveAnimationMask.resize(m_nodeCount);
	m_invertedAdditiveAnimationMask.resize(m_nodeCount);

	std::fill(m_additiveAnimationMask.begin(), m_additiveAnimationMask.end(), true);
	m_invertedAdditiveAnimationMask = m_additiveAnimationMask;
	m_invertedAdditiveAnimationMask.flip();

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

	m_ao.LoadTexture("C:/dev/NPC_RL_Prototype/NPC_RL_Prototype/Assets/Models/New/Updated/Atlas_00001.png", false);


	glBindTexture(GL_TEXTURE_2D, 0);
	return textureIDs;
}

void Enemy::GetWeightData()
{
	const std::string attr = "WEIGHTS_0";

	m_weightVec.clear(); // make this std::vector<glm::vec4>
	std::vector<std::pair<size_t, size_t>> primRanges;

	auto elemSize = [](int gltfType, int compType) -> size_t {
		int ncomp =
			(gltfType == TINYGLTF_TYPE_SCALAR) ? 1 :
			(gltfType == TINYGLTF_TYPE_VEC2) ? 2 :
			(gltfType == TINYGLTF_TYPE_VEC3) ? 3 :
			(gltfType == TINYGLTF_TYPE_VEC4) ? 4 : 0;
		size_t csize =
			(compType == TINYGLTF_COMPONENT_TYPE_BYTE ||
				compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) ? 1 :
			(compType == TINYGLTF_COMPONENT_TYPE_SHORT ||
				compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ? 2 :
			(compType == TINYGLTF_COMPONENT_TYPE_INT ||
				compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
				compType == TINYGLTF_COMPONENT_TYPE_FLOAT) ? 4 : 0;
		return ncomp * csize;
		};

	for (size_t mi = 0; mi < enemyModel->meshes.size(); ++mi)
	{
		const auto& mesh = enemyModel->meshes[mi];
		for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
		{
			const auto& prim = mesh.primitives[pi];
			auto it = prim.attributes.find(attr);
			if (it == prim.attributes.end())
				continue;

			int accIdx = it->second;
			const auto& acc = enemyModel->accessors[accIdx];
			const auto& bv = enemyModel->bufferViews[acc.bufferView];
			const auto& buf = enemyModel->buffers[bv.buffer];

			const uint8_t* srcBase = buf.data.data() + bv.byteOffset + acc.byteOffset;

			size_t stride = bv.byteStride;
			if (stride == 0) stride = elemSize(acc.type, acc.componentType);

			if (acc.type != TINYGLTF_TYPE_VEC4) {
				Logger::Log(0, "%s: WEIGHTS_0 must be vec4\n", __FUNCTION__);
				continue;
			}

			size_t start = m_weightVec.size();
			m_weightVec.resize(start + acc.count);

			for (size_t k = 0; k < acc.count; ++k)
			{
				const uint8_t* s = srcBase + k * stride;
				glm::vec4 w(0.0f);

				switch (acc.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {
					const float* v = reinterpret_cast<const float*>(s);
					w = glm::vec4(v[0], v[1], v[2], v[3]);
				} break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
					const uint8_t* v = reinterpret_cast<const uint8_t*>(s);
					w = glm::vec4(v[0], v[1], v[2], v[3]) / 255.0f;
				} break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
					const uint16_t* v = reinterpret_cast<const uint16_t*>(s);
					w = glm::vec4(v[0], v[1], v[2], v[3]) / 65535.0f;
				} break;
				default:
					Logger::Log(0, "%s: unexpected WEIGHTS_0 component type\n", __FUNCTION__);
					continue;
				}

				// Safety: renormalize
				float sum = w.x + w.y + w.z + w.w;
				if (sum > 0.0f) w /= sum;
				m_weightVec[start + k] = w;
			}

			Logger::Log(1, "%s: mesh %zu prim %zu WEIGHTS_0 acc %d count %zu\n",
				__FUNCTION__, mi, pi, accIdx, size_t(acc.count));
			primRanges.emplace_back(start, acc.count);
		}
	}
}


void Enemy::DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos) {
	glDisable(GL_CULL_FACE);
	int texIndex = 1;
	for (size_t meshIndex = 0; meshIndex < meshData.size(); ++meshIndex) {
		for (size_t primIndex = 0; primIndex < meshData[meshIndex].primitives.size(); ++primIndex) {
			const GLTFPrimitive& prim = meshData[meshIndex].primitives[primIndex];

			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, m_position);
			//modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
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
			if (matIndex >= 0 && matIndex < enemyModel->materials.size()) {
				const tinygltf::Material& mat = enemyModel->materials[matIndex];
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
					m_shader->SetFloat("metallicFactor", static_cast<float>(mat.pbrMetallicRoughness.metallicFactor));
					m_shader->SetFloat("roughnessFactor", static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor));
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
			glBindTexture(GL_TEXTURE_2D, 0);
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
//	if (!isDead_ || !isDestroyed)
//	{
//		if (shouldUseEDBT)
//		{
//#ifdef TRACY_ENABLE
//			ZoneScopedN("EDBT Update");
//#endif
//			float playerEnemyDistance = glm::distance(getPosition(), player.getPosition());
//			if (playerEnemyDistance < 35.0f && !IsPlayerDetected())
//			{
//				DetectPlayer();
//			}
//
//			behaviorTree_->Tick();
//
//
//			if (enemyHasShot)
//			{
//				enemyRayDebugRenderTimer -= dt_;
//				enemyShootCooldown -= dt_;
//			}
//			if (enemyShootCooldown <= 0.0f)
//			{
//				enemyHasShot = false;
//			}
//
//			if (shootAudioCooldown > 0.0f)
//			{
//				shootAudioCooldown -= dt_;
//			}
//		}
//		else
//		{
//			decisionDelayTimer -= dt_;
//		}
//
//	}
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
			SetAnimation(GetSourceAnimNum(), GetDestAnimNum(), animSpeedDivider / 2.0f, m_blendFactor, false);
			if (m_blendFactor >= 1.0f)
			{
				m_blendAnim = false;
				m_blendFactor = 0.0f;
				//SetSourceAnimNum(GetDestAnimNum());
			}
		}
		else
		{
			SetAnimation(GetSourceAnimNum(), animSpeedDivider, 1.0f, false);
			m_blendFactor = 0.0f;
		}
}

void Enemy::OnEvent(const Event& event)
{
	if (auto e = dynamic_cast<const PlayerDetectedEvent*>(&event))
	{
		if (e->m_npcId != m_id)
		{
			m_isPlayerDetected = true;
			Logger::Log(1, "Player detected by enemy %d\n", m_id);
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

void Enemy::SetPositionOld(glm::vec3 newPos)
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

	//if (pathIndex_ >= path.size()) {
	//	Logger::Log(1, "%s success: Agent has reached its destination.\n", __FUNCTION__);
	//	grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

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
	//for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
	//	for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
	//		glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid_->GetCellSize(), (newPos.z + zOffset) / grid_->GetCellSize());
	//		if (checkPos.x < 0 || checkPos.x >= grid_->GetGridSize() || checkPos.y < 0 || checkPos.y >= grid_->GetGridSize()
	//			|| grid_->GetGrid()[checkPos.x][checkPos.y].IsObstacle() || (grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupied()
	//				&& !grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupiedBy(id_))) {
	//			isObstacleFree = false;
	//			break;
	//		}
	//	}
	//	if (!isObstacleFree) break;
	//}

	//if (isObstacleFree) {
	//	setPosition(newPos);
	//}
	//else {
	//	// If the new position is within an obstacle, try to adjust the position slightly
	//	newPos = getPosition() + direction * (speed * deltaTime * 0.01f);
	//	isObstacleFree = true;
	//	for (float xOffset = -agentRadius; xOffset <= agentRadius; xOffset += agentRadius * 2) {
	//		for (float zOffset = -agentRadius; zOffset <= agentRadius; zOffset += agentRadius * 2) {
	//			glm::ivec2 checkPos = glm::ivec2((newPos.x + xOffset) / grid_->GetCellSize(), (newPos.z + zOffset) / grid_->GetCellSize());
	//			if (checkPos.x < 0 || checkPos.x >= grid_->GetGridSize() || checkPos.y < 0 || checkPos.y >= grid_->GetGridSize()
	//				|| grid_->GetGrid()[checkPos.x][checkPos.y].IsObstacle() || (grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupied()
	//					&& !grid_->GetGrid()[checkPos.x][checkPos.y].IsOccupiedBy(id_))) {
	//				isObstacleFree = false;
	//				break;
	//			}
	//		}
	//		if (!isObstacleFree) break;
	//	}

	//	if (isObstacleFree) {
	//		setPosition(newPos);
	//	}
	//}

	////if (pathIndex_ == 0) {
	////    // Snap the enemy to the center of the starting grid cell when the path starts
	////    glm::vec3 startCellCenter = glm::vec3(path[pathIndex_].x * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f, getPosition().z, path[pathIndex_].y * grid_->GetCellSize() + grid_->GetCellSize() / 2.0f);
	////    setPosition(startCellCenter);
	////}

	//if (glm::distance(getPosition(), targetPos) < grid_->GetCellSize() / 3.0f)
	//{
	//	//	grid_->OccupyCell(path[pathIndex_].x, path[pathIndex_].y, id_);
	//}

	////    if (pathIndex_ >= 1)
	// //      grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);

	//	// Check if the enemy has reached the current target position within a tolerance
	//if (glm::distance(getPosition(), targetPos) < tolerance) {
	//	//		grid_->VacateCell(path[pathIndex_].x, path[pathIndex_].y, id_);

	//	pathIndex_++;
	//	if (pathIndex_ >= path.size()) {
	//		//			grid_->VacateCell(path[pathIndex_ - 1].x, path[pathIndex_ - 1].y, id_);
	//		pathIndex_ = 0; // Reset path index if the end is reached
	//	}
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

	m_enemyShootPos = GetPositionOld() + glm::vec3(0.0f, 2.5f, 0.0f);
	m_enemyShootDir = (m_player.GetPosition() - GetPositionOld()) + accuracyOffset;
	auto hitPoint = glm::vec3(0.0f);

	glm::vec3 playerDir = normalize(m_player.GetPosition() - GetPositionOld());

	m_yaw = glm::degrees(glm::atan(playerDir.z, playerDir.x));
	UpdateEnemyVectors();

	bool hit = false;
	hit = GetGameManager()->GetPhysicsWorld()->RayIntersect(m_enemyShootPos, m_enemyShootDir, m_enemyHitPoint, m_aabb);

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
	m_enemyShootCooldown = 0.5f;
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
	float playerDistance = distance(GetPositionOld(), m_player.GetPosition());

	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (chosenAction == ADVANCE)
	{
		m_state = "ADVANCE";
			}
	else if (chosenAction == RETREAT)
	{
		m_state = "RETREAT";

		VacatePreviousCell();

		MoveEnemy(m_currentPath, m_dt, 1.0f, false);

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
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
		nextState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
		nextState.playerVisible = IsPlayerVisible();
		nextState.health = GetHealth();
		nextState.isSuppressionFire = isSuppressionFire;
	}
	else if (chosenAction == PATROL)
	{
		m_state = "PATROL";

		if (m_reachedDestination == false)
		{
			
			VacatePreviousCell();

			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}
		else
		{

			VacatePreviousCell();

			m_reachedDestination = false;

			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}

		nextState.playerDetected = IsPlayerDetected();
		nextState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
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
	Logger::Log(1, "Enemy %d Chosen Action: %d with reward: %d\n", enemyId, chosenAction, reward);
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
	float playerDistance = distance(GetPositionOld(), m_player.GetPosition());
	if (!IsPlayerDetected() && (playerDistance < 35.0f) && IsPlayerVisible())
	{
		DetectPlayer();
	}

	if (m_chosenAction == ADVANCE)
	{
		m_state = "ADVANCE";
		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->FindPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_player.GetPosition().x / m_grid->GetCellSize(), m_player.GetPosition().z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			enemyId
		//		);
		//}

		MoveEnemy(m_currentPath, deltaTime, 1.0f, false);

		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (m_chosenAction == RETREAT)
	{
		m_state = "RETREAT";

		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->FindPath(
		//			glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
		//			glm::ivec2(m_selectedCover->m_worldPosition.x / m_grid->GetCellSize(), m_selectedCover->m_worldPosition.z / m_grid->GetCellSize()),
		//			m_grid->GetGrid(),
		//			enemyId
		//		);
		//}

		MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
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
		currentState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}
	else if (m_chosenAction == PATROL)
	{
		m_state = "PATROL";

		if (m_reachedDestination == false)
		{
		
			VacatePreviousCell();

			//for (glm::ivec2& cell : m_currentPath)
			//{
			//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
			//		m_currentPath = m_grid->FindPath(
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
			
			VacatePreviousCell();

			m_reachedDestination = false;

			/*		for (glm::ivec2& cell : m_currentPath)
					{
						if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
							m_currentPath = m_grid->FindPath(
								glm::ivec2(GetPosition().x / m_grid->GetCellSize(), GetPosition().z / m_grid->GetCellSize()),
								glm::ivec2(m_currentWaypoint.x / m_grid->GetCellSize(), m_currentWaypoint.z / m_grid->GetCellSize()),
								m_grid->GetGrid(),
								enemyId
							);
					}*/


			MoveEnemy(m_currentPath, m_dt, 1.0f, false);
		}


		currentState.playerDetected = IsPlayerDetected();
		currentState.distanceToPlayer = distance(GetPositionOld(), m_player.GetPosition());
		currentState.playerVisible = IsPlayerVisible();
		currentState.health = GetHealth();
		currentState.isSuppressionFire = isSuppressionFire;
	}

	squadActions[enemyId] = m_chosenAction;

	Logger::Log(1, "Enemy %d Chosen Action: %d\n", enemyId, m_chosenAction);
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


	m_takingDamage = false;
	m_damageTimer = 0.0f;
	m_dyingTimer = 0.0f;
	m_coverTimer = 0.0f;
	m_reachedCover = false;

	m_reachedDestination = false;
	m_reachedPlayer = false;

	m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	//UpdateAABB();

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
	glm::vec3 tempEnemyShootPos = GetPositionOld() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 tempEnemyShootDir = normalize(m_player.GetPosition() - GetPositionOld());
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
	float playerEnemyDistance = distance(GetPositionOld(), m_player.GetPosition());

	glm::vec3 tempEnemyShootPos = GetPositionOld() + glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 tempEnemyShootDir = normalize(m_player.GetPosition() - GetPositionOld());
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


	m_isAttacking = true;

	//for (glm::ivec2& cell : m_currentPath)
	//{
	//	if (cell.x >= 0 && cell.x < m_grid->GetGridSize() && cell.y >= 0 && cell.y < m_grid->GetGridSize() && m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
	//		m_currentPath = m_grid->FindPath(
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
		
		VacatePreviousCell();

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->FindPath(
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
		
		VacatePreviousCell();

		m_reachedDestination = false;

		//for (glm::ivec2& cell : m_currentPath)
		//{
		//	if (m_grid->GetGrid()[cell.x][cell.y].IsOccupied())
		//		m_currentPath = m_grid->FindPath(
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

	glm::vec3 rayOrigin = GetPositionOld() + glm::vec3(0.0f, 2.5f, 0.0f);
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