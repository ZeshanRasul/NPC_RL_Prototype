#pragma once

#include <string>
#include <iostream>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "Model/GLTFPrimitive.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

enum PlayerState
{
	MOVING,
	AIMING,
	SHOOTING,
	PLAYER_STATE_COUNT
};

class Player : public GameObject
{
public:
	Player(glm::vec3 pos, glm::vec3 scale, Shader* shdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, float yaw = -90.0f)
		: GameObject(pos, scale, yaw, shdr, shadowMapShader, applySkinning, gameMgr)
	{
		SetInitialPos(pos);
		m_initialYaw = yaw;

		//m_model = std::make_shared<GltfModel>();

		//std::string modelFilename =
		//	"src/Assets/Models/GLTF/SwatPlayer/Swat.gltf";
		//std::string modelTextureFilename =
		//	"src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_BaseColor.png";

		//if (!m_model->LoadModel(modelFilename))
		//{
		//	Logger::Log(1, "%s: loading glTF m_model '%s' failed\n", __FUNCTION__, modelFilename.c_str());
		//}

		playerModel = new tinygltf::Model;

		std::string modelFilename = "src/Assets/Models/Soldier/soldier5.gltf";


		tinygltf::TinyGLTF gltfLoader;
		std::string loaderErrors;
		std::string loaderWarnings;
		bool result = false;

		result = gltfLoader.LoadASCIIFromFile(playerModel, &loaderErrors, &loaderWarnings,
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

		SetupGLTFMeshes(playerModel);

		for (int texID : LoadGLTFTextures(playerModel))
			glTextures.push_back(texID);

		//m_tex = m_model->LoadTexture(modelTextureFilename, false);
		//
		//m_normal.LoadTexture(
		//	"src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Normal.png");
		//m_metallic.LoadTexture(
		//	"src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Metallic.png");
		//m_roughness.LoadTexture(
		//	"src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_Roughness.png");
		//m_ao.LoadTexture(
			//"src/Assets/Models/GLTF/SwatPlayer/Swat_Ch15_body_AO.png");

		//m_model->uploadIndexBuffer();
		//Logger::Log(1, "%s: glTF m_model '%s' successfully loaded\n", __FUNCTION__, modelFilename.c_str());
		//
		//size_t playerModelJointDualQuatBufferSize = getJointDualQuatsSize() *
		//	sizeof(glm::mat2x4);
		//m_playerDualQuatSsBuffer.Init(playerModelJointDualQuatBufferSize);
		//Logger::Log(1, "%s: glTF joint dual quaternions shader storage buffer (size %i bytes) successfully created\n",
		//            __FUNCTION__, playerModelJointDualQuatBufferSize);

		m_takeDamageAc = new AudioComponent(this);
		m_deathAc = new AudioComponent(this);
		m_shootAc = new AudioComponent(this);

		ComputeAudioWorldTransform();

		UpdatePlayerVectors();
		SetPlayerAimFront(GetPlayerFront());
		m_playerAimRight = GetPlayerRight();
		SetPlayerAimUp(m_playerUp);
	}

	std::vector<glm::tvec4<uint16_t>> m_jointVec{};
	std::vector<glm::vec4> m_weightVec{};
	std::vector<glm::mat4> m_inverseBindMatrices{};
	std::vector<glm::mat4> m_jointMatrices{};
	std::vector<glm::mat2x4> m_jointDualQuats{};

	std::vector<int> m_attribAccessors{};
	std::vector<int> m_nodeToJoint{};

	std::shared_ptr<GltfNode> m_rootNode = nullptr;

	std::vector<std::shared_ptr<GltfNode>> m_nodeList;
	int m_nodeCount = 0;

	std::vector<std::shared_ptr<GltfAnimationClip>> m_animClips{};
	size_t m_clipsSize;

	std::vector<bool> m_additiveAnimationMask{};
	std::vector<bool> m_invertedAdditiveAnimationMask{};

	~Player()
	{
		m_model->cleanup();
	}

	void GetInvBindMatrices()
	{
		const tinygltf::Skin& skin = playerModel->skins.at(0);
		int invBindMatAccessor = skin.inverseBindMatrices;

		const tinygltf::Accessor& accessor = playerModel->accessors.at(invBindMatAccessor);
		const tinygltf::BufferView& bufferView = playerModel->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = playerModel->buffers.at(bufferView.buffer);

		m_inverseBindMatrices.resize(skin.joints.size());
		m_jointMatrices.resize(skin.joints.size());
		m_jointDualQuats.resize(skin.joints.size());

		std::memcpy(m_inverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset,
			bufferView.byteLength);
	}

	void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode)
	{
		int nodeNum = treeNode->GetNodeNum();
		m_jointMatrices.at(m_nodeToJoint.at(nodeNum)) =
			treeNode->GetNodeMatrix() * m_inverseBindMatrices.at(m_nodeToJoint.at(nodeNum));

		/* extract components from node matrix */
		glm::quat orientation;
		glm::vec3 scale;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::dualquat dq;

		/* create dual quaternion */
		if (decompose(m_jointMatrices.at(m_nodeToJoint.at(nodeNum)), scale, orientation,
			translation, skew, perspective))
		{
			dq[0] = orientation;
			dq[1] = glm::quat(0.0, translation.x, translation.y, translation.z) * orientation * 0.5f;
			m_jointDualQuats.at(m_nodeToJoint.at(nodeNum)) = mat2x4_cast(dq);
			glm::mat2x4 newDq = m_jointDualQuats.at(m_nodeToJoint.at(nodeNum));
			newDq;
		}
		else {
			//Logger::Log(1, "%s error: could not decompose matrix for node %i\n", __FUNCTION__,
		//		nodeNum);
		}
	}

	void GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
	{
		int nodeNum = treeNode->GetNodeNum();
		const tinygltf::Node& node = playerModel->nodes.at(nodeNum);
		treeNode->SetNodeName(node.name);

		if (node.translation.size())
		{
			treeNode->SetTranslation(glm::make_vec3(node.translation.data()));
		}
		if (node.rotation.size())
		{
			treeNode->SetRotation(glm::make_quat(node.rotation.data()));
		}
		if (node.scale.size())
		{
			treeNode->SetScale(glm::make_vec3(node.scale.data()));
		}

		treeNode->CalculateLocalTrsMatrix();
		treeNode->CalculateNodeMatrix(parentNodeMatrix);

		UpdateJointMatricesAndQuats(treeNode);
	}


	void GetJointData()
	{
		std::string jointsAccessorAttrib = "JOINTS_0";

		for (auto& mesh : playerModel->meshes)
		{
			for (auto& primitive : mesh.primitives)
			{
			}
		}
		int jointsAccessor = playerModel->meshes.at(0).primitives.at(0).attributes[jointsAccessorAttrib];
		Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, jointsAccessor,
			jointsAccessorAttrib.c_str());


		const tinygltf::Accessor& accessor = playerModel->accessors.at(jointsAccessor);
		const tinygltf::BufferView& bufferView = playerModel->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = playerModel->buffers.at(bufferView.buffer);
		int jointVecSize = accessor.count;
		Logger::Log(1, "%s: %i short vec4 in JOINTS_0\n", __FUNCTION__, jointVecSize);
		m_jointVec.resize(jointVecSize);


		std::memcpy(m_jointVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
			bufferView.byteLength);

		m_nodeToJoint.resize(playerModel->nodes.size());
		const tinygltf::Skin& skin = playerModel->skins.at(0);
		for (int i = 0; i < skin.joints.size(); ++i)
		{
			int destinationNode = skin.joints.at(i);
			m_nodeToJoint.at(destinationNode) = i;
			Logger::Log(2, "%s: joint %i affects node %i\n", __FUNCTION__, i, destinationNode);
		}

	}

	void GetWeightData()
	{
		std::string weightsAccessorAttrib = "WEIGHTS_0";
		int weightAccessor = playerModel->meshes.at(0).primitives.at(0).attributes.at(weightsAccessorAttrib);
		Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, weightAccessor,
			weightsAccessorAttrib.c_str());

		const tinygltf::Accessor& accessor = playerModel->accessors.at(weightAccessor);
		const tinygltf::BufferView& bufferView = playerModel->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = playerModel->buffers.at(bufferView.buffer);

		int weightVecSize = accessor.count;
		Logger::Log(1, "%s: %i vec4 in WEIGHTS_0\n", __FUNCTION__, weightVecSize);
		m_weightVec.resize(weightVecSize);

		std::memcpy(m_weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
			bufferView.byteLength);
	}

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
		glm::vec3 camPos) override;

	void Update(float dt, bool isPaused, bool isTimeScaled);

	glm::vec3 GetPosition()
	{
		return m_position;
	}

	void SetPosition(glm::vec3 newPos)
	{
		m_position = newPos;
		m_recomputeWorldTransform = true;
	}

	float GetInitialYaw() const { return m_initialYaw; }

	void SetYaw(float newYaw)
	{
		m_yaw = newYaw;
		m_recomputeWorldTransform = true;
	};

	void ComputeAudioWorldTransform() override;

	void UpdatePlayerVectors();
	void UpdatePlayerAimVectors();

	void PlayerProcessKeyboard(CameraMovement direction, float deltaTime);
	void PlayerProcessMouseMovement(float xOffset);

	float GetVelocity() const { return m_velocity; }
	void SetVelocity(float newVelocity) { m_velocity = newVelocity; }

	PlayerState GetPlayerState() const { return m_playerState; }
	void SetPlayerState(PlayerState newState);

	glm::vec3 GetShootPos()
	{
		return GetPosition() + glm::vec3(0.0f, 4.5f, 0.0f) + (4.5f * m_playerAimFront) + (-0.5f * m_playerAimRight);
	}

	float GetShootDistance() const { return m_shootDistance; }
	glm::vec3 GetPlayerHitPoint() const { return m_playerShootHitPoint; }

	void Shoot();

	void SetCameraMatrices(glm::mat4 viewMat, glm::mat4 proj)
	{
		m_view = viewMat;
		m_projection = proj;
	}

	void SetUpAABB();
	void SetAABBShader(Shader* aabbShdr) { m_aabbShader = aabbShdr; }
	void UpdateAabb()
	{
		glm::mat4 modelMatrix = translate(glm::mat4(1.0f), m_position) *
			rotate(glm::mat4(1.0f), glm::radians(-m_yaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), m_scale);
		m_aabb->Update(modelMatrix);
	}

	AABB* GetAABB() const { return m_aabb; }
	void SetAabbColor(glm::vec3 color) { m_aabbColor = color; }

	void OnHit() override;

	void OnMiss() override
	{
		std::cout << "Player was missed!" << std::endl;
		SetAabbColor(glm::vec3(1.0f, 1.0f, 1.0f));
	};


	void OnDeath();

	float GetHealth() const { return m_health; }
	void SetHealth(float newHealth) { m_health = newHealth; }

	void TakeDamage(float damage)
	{
		/*m_takeDamageAc->PlayEvent("event:/PlayerTakeDamage");*/
		SetHealth(GetHealth() - damage);
		if (GetHealth() <= 0.0f)
		{
			OnDeath();
		}
	}

	int GetAnimNum() const { return m_animNum; }
	int GetSourceAnimNum() const { return m_sourceAnim; }
	int GetDestAnimNum() const { return m_destAnim; }
	void SetAnimNum(int newAnimNum) { m_animNum = newAnimNum; }
	void SetSourceAnimNum(int newSrcAnim) { m_sourceAnim = newSrcAnim; }

	void SetDestAnimNum(int newDestAnim)
	{
		m_destAnim = newDestAnim;
		m_destAnimSet = true;
	}

	void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playAnimBackwards);
	void SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playAnimBackwards);


	void ResetGame();

	void HasDealtDamage() override
	{
	};

	void HasKilledPlayer() override
	{
	};

	float GetPlayerYaw() const { return m_playerYaw; }
	void SetPlayerYaw(float val) { m_playerYaw = val; }

	float GetAimPitch() const { return m_aimPitch; }
	void SetAimPitch(float val) { m_aimPitch = val; }

	CameraMovement GetPrevDirection() const { return m_prevDirection; }
	void SetPrevDirection(CameraMovement val) { m_prevDirection = val; }

	glm::vec3 GetPlayerFront() const { return m_playerFront; }
	void SetPlayerFront(glm::vec3 val) { m_playerFront = val; }

	glm::vec3 GetPlayerRight() const { return m_playerRight; }
	void SetPlayerRight(glm::vec3 val) { m_playerRight = val; }

	glm::vec3 GetPlayerAimFront() const { return m_playerAimFront; }
	void SetPlayerAimFront(glm::vec3 val) { m_playerAimFront = val; }

	glm::vec3 GetPlayerAimUp() const { return m_playerAimUp; }
	void SetPlayerAimUp(glm::vec3 val) { m_playerAimUp = val; }

	glm::vec3 GetInitialPos() const { return m_initialPos; }
	void SetInitialPos(glm::vec3 val) { m_initialPos = val; }
	Shader* m_aabbShader;
	CameraMovement m_prevDirection = STATIONARY;


	float m_playerYaw;
	glm::vec3 m_playerFront;
	glm::vec3 m_playerRight;
	glm::vec3 m_playerUp;
	glm::vec3 m_playerAimFront;
	glm::vec3 m_playerAimRight;
	glm::vec3 m_playerAimUp;
	float m_aimPitch = 0.0f;
	float m_initialYaw = -90.0f;

	tinygltf::Model* playerModel;

	struct GLTFMesh {
		std::vector<GLTFPrimitive> primitives;
	};

	std::vector<GLTFMesh> meshData;

	void GetNodes(std::shared_ptr<GltfNode> treeNode)
	{
		int nodeNum = treeNode->GetNodeNum();
		std::vector<int> childNodes = playerModel->nodes.at(nodeNum).children;

		/* remove the child node with skin/mesh metadata, confuses skeleton */
		auto removeIt = std::remove_if(childNodes.begin(), childNodes.end(),
			[&](int num) { return playerModel->nodes.at(num).skin != -1; }
		);
		childNodes.erase(removeIt, childNodes.end());

		treeNode->AddChilds(childNodes);
		glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();

		for (auto& childNode : treeNode->GetChilds())
		{
			m_nodeList.at(childNode->GetNodeNum()) = childNode;
			GetNodeData(childNode, treeNodeMatrix);
			GetNodes(childNode);
		}
	}

	void GetAnimations()
	{
		for (const auto& anim : playerModel->animations)
		{
			Logger::Log(1, "%s: loading animation '%s' with %i channels\n", __FUNCTION__, anim.name.c_str(),
				anim.channels.size());
			auto clip = std::make_shared<GltfAnimationClip>(anim.name);
			for (const auto& channel : anim.channels)
			{
				clip->AddChannel(playerModel, anim, channel);
			}
			m_animClips.push_back(clip);
		}
	}

	void SetupGLTFMeshes(tinygltf::Model* model)
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
					// Add JOINTS_0, WEIGHTS_0 etc. if needed
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

		m_nodeCount = (int)playerModel->nodes.size();
		int rootNode = playerModel->scenes.at(0).nodes.at(0);
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

	

	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos);

	std::vector<GLuint> glTextures;


	std::vector<GLuint> LoadGLTFTextures(tinygltf::Model* model);

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}


private:
	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);

	Texture m_normal{};
	Texture m_metallic{};
	Texture m_roughness{};
	Texture m_ao{};

	AudioComponent* m_takeDamageAc;
	AudioComponent* m_shootAc;
	AudioComponent* m_deathAc;



	glm::vec3 m_shootStartPos = GetPosition() + (glm::vec3(0.0f, 2.5f, 0.0f));
	float m_shootDistance = 100000.0f;
	glm::vec3 m_playerShootHitPoint = glm::vec3(0.0f);

	float m_movementSpeed = 10.5f;
	float m_velocity = 0.0f;

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_playerDualQuatSsBuffer{};

	PlayerState m_playerState = MOVING;

	float m_health = 100.0f;

	int m_animNum = 0;
	int m_sourceAnim = 0;
	int m_destAnim = 0;
	bool m_destAnimSet = true;
	float m_blendSpeed = 10.0f;
	float m_blendFactor = 0.0f;
	bool m_blendAnim = false;
	bool m_resetBlend = false;

	glm::vec3 m_initialPos = glm::vec3(0.0f, 0.0f, 0.0f);

	bool m_playGameStartAudio = true;
	float m_playGameStartAudioTimer = 3.0f;
	float m_playerShootAudioCooldown = 2.0f;

	AABB* m_aabb;
	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);

	std::map<std::string, GLint> m_playerModelAttributes =
	{
		{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"JOINTS_0", 4}, {"WEIGHTS_0", 5},
		{"TANGENT", 6}
	};
};
