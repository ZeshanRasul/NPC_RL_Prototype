#pragma once

#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GameObject.h"
#include "../Camera.h"
#include "Logger.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "Model/GLTFPrimitive.h"

#include "Scene/Components/TransformComponent.h"

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
		GameManager* gameMgr, float yaw);

	std::vector<glm::mat2x4> getJointDualQuats()
	{
		return m_jointDualQuats;
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

	std::string GetNodeName(int nodeNum)
	{
		if (nodeNum >= 0 && nodeNum < (m_nodeList.size()) && m_nodeList.at(nodeNum))
		{
			return m_nodeList.at(nodeNum)->GetNodeName();
		}
		return "(Invalid)";
	}

	void ResetNodeData()
	{
		GetNodeData(m_rootNode, glm::mat4(1.0f));
		ResetNodeData(m_rootNode, glm::mat4(1.0f));
	}

	void ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
	{
		glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();
		for (auto& childNode : treeNode->GetChilds())
		{
			GetNodeData(childNode, treeNodeMatrix);
			ResetNodeData(childNode, treeNodeMatrix);
		}
	}

	void UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
	{
		treeNode->CalculateNodeMatrix(parentNodeMatrix);
		UpdateJointMatricesAndQuats(treeNode);

		glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();

		for (auto& childNode : treeNode->GetChilds())
		{
			//Logger::Log(1, "%s: updating node %i (%s)\n", __FUNCTION__,
			//	childNode->GetNodeNum(), childNode->GetNodeName().c_str());
			UpdateNodeMatrices(childNode, treeNodeMatrix);
		}
	}

	void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode)
	{
		const int nodeNum = treeNode->GetNodeNum();

		if (nodeNum < 0 || nodeNum >= static_cast<int>(m_nodeToJoint.size()))
			return;

		const int j = m_nodeToJoint[nodeNum];
		if (j < 0)                      return; // not a joint
		if (j >= static_cast<int>(m_jointMatrices.size()) ||
			j >= static_cast<int>(m_inverseBindMatrices.size()) ||
			j >= static_cast<int>(m_jointDualQuats.size())) {
			Logger::Log(0, "%s: joint %d out of range for buffers\n", __FUNCTION__, j);
			return;
		}

		const glm::mat4 J = treeNode->GetNodeMatrix() * m_inverseBindMatrices[j];
		m_jointMatrices[j] = J;

		/* extract components from node matrix */
		glm::quat orientation;
		glm::vec3 scale;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;

		/* create dual quaternion */
		if (glm::decompose(J, scale, orientation, translation, skew, perspective)) {
			glm::dualquat dq;
			dq.real = orientation;
			dq.dual = glm::quat(0.0f, translation) * orientation * 0.5f;
			m_jointDualQuats[j] = glm::mat2x4_cast(dq);
		}
	}

	void BlendAnimationFrame(int animNum, float time, float blendFactor)
	{
		m_animClips.at(animNum)->BlendAnimationFrame(m_nodeList, m_additiveAnimationMask, time, blendFactor);
		UpdateNodeMatrices(m_rootNode, glm::mat4(1.0f));
	}

	void PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
	{
		//Logger::Log(1, "%s: playing animation %i at speed %f with blend factor %f\n",
		//	__FUNCTION__, animNum, speedDivider, blendFactor);
		double currentTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		if (playBackwards)
		{
			BlendAnimationFrame(animNum, m_animClips.at(animNum)->GetClipEndTime() -
				static_cast<float>(std::fmod(currentTime / 1000.0 * speedDivider,
					m_animClips.at(animNum)->GetClipEndTime())), blendFactor);
		}
		else
		{
			BlendAnimationFrame(animNum, static_cast<float>(std::fmod(currentTime / 1000.0 * speedDivider,
				m_animClips.at(animNum)->GetClipEndTime())), blendFactor);
		}
	}

	float GetAnimationEndTime(int animNum)
	{
		return m_animClips.at(animNum)->GetClipEndTime();
	}

	std::string GetClipName(int animNum)
	{
		return m_animClips.at(animNum)->GetClipName();
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

	int GetJointDualQuatsSize()
	{
		return (int)m_jointDualQuats.size();
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
	// Helpers to look up an attribute accessor for a primitive
	bool GetAttrAccessorIndex(const tinygltf::Primitive& prim,
		const char* name, int& outAccessorIndex)
	{
		auto it = prim.attributes.find(name);
		if (it == prim.attributes.end()) return false;
		outAccessorIndex = it->second;
		return true;
	}

	void GetJointData()
	{
		const std::string attr = "JOINTS_0";

		m_jointVec.clear();
		std::vector<std::pair<size_t, size_t>> primRanges; // [start,count] per primitive if you need it

		auto elemSize = [](int gltfType, int compType) -> size_t {
			int ncomp = (gltfType == TINYGLTF_TYPE_VEC4) ? 4 :
				(gltfType == TINYGLTF_TYPE_VEC3) ? 3 :
				(gltfType == TINYGLTF_TYPE_VEC2) ? 2 :
				(gltfType == TINYGLTF_TYPE_SCALAR) ? 1 : 0;
			size_t csize = (compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
				compType == TINYGLTF_COMPONENT_TYPE_BYTE) ? 1 :
				(compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
					compType == TINYGLTF_COMPONENT_TYPE_SHORT) ? 2 :
				(compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
					compType == TINYGLTF_COMPONENT_TYPE_INT ||
					compType == TINYGLTF_COMPONENT_TYPE_FLOAT) ? 4 : 0;
			return ncomp * csize;
			};

		for (size_t mi = 0; mi < playerModel->meshes.size(); ++mi)
		{
			const auto& mesh = playerModel->meshes[mi];
			for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
			{
				const auto& prim = mesh.primitives[pi];
				auto it = prim.attributes.find(attr);
				if (it == prim.attributes.end())
					continue; // rigid primitive

				int accIdx = it->second;
				const auto& acc = playerModel->accessors[accIdx];
				const auto& bv = playerModel->bufferViews[acc.bufferView];
				const auto& buf = playerModel->buffers[bv.buffer];

				const uint8_t* srcBase =
					buf.data.data() + bv.byteOffset + acc.byteOffset;

				size_t stride = bv.byteStride;
				if (stride == 0) stride = elemSize(acc.type, acc.componentType);

				// Only u8x4 or u16x4 are valid for JOINTS_0
				if (acc.type != TINYGLTF_TYPE_VEC4 ||
					(acc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
						acc.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT))
				{
					Logger::Log(0, "%s: unexpected JOINTS_0 type/compType\n", __FUNCTION__);
					continue;
				}

				size_t start = m_jointVec.size();
				m_jointVec.resize(start + acc.count);

				for (size_t k = 0; k < acc.count; ++k)
				{
					const uint8_t* s = srcBase + k * stride;
					glm::u16vec4 j(0);
					if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
						const uint8_t* v = reinterpret_cast<const uint8_t*>(s);
						j = glm::u16vec4(v[0], v[1], v[2], v[3]);
					}
					else {
						const uint16_t* v = reinterpret_cast<const uint16_t*>(s);
						j = glm::u16vec4(v[0], v[1], v[2], v[3]);
					}
					m_jointVec[start + k] = j;
				}

				Logger::Log(1, "%s: mesh %zu prim %zu JOINTS_0 acc %d count %zu\n",
					__FUNCTION__, mi, pi, accIdx, size_t(acc.count));
				primRanges.emplace_back(start, acc.count);
			}
		}

		// Build node->joint map once per skin
		const tinygltf::Skin& skin = playerModel->skins.at(0);
		m_nodeToJoint.assign(playerModel->nodes.size(), -1);
		for (int i = 0; i < static_cast<int>(skin.joints.size()); ++i) {
			int jointNode = skin.joints[i];
			if (jointNode >= 0 && jointNode < static_cast<int>(m_nodeToJoint.size()))
				m_nodeToJoint[jointNode] = i;
		}

		m_inverseBindMatrices.resize(skin.joints.size());
		m_jointMatrices.resize(skin.joints.size());
		m_jointDualQuats.resize(skin.joints.size());
	}


	void GetWeightData();

	//void GetWeightData()
	//{
	//	int weightAccessor = playerModel->meshes.at(0).primitives.at(0).attributes.at(weightsAccessorAttrib);
	//	Logger::Log(1, "%s: using accessor %i to get %s\n", __FUNCTION__, weightAccessor,
	//		weightsAccessorAttrib.c_str());

	//	const tinygltf::Accessor& accessor = playerModel->accessors.at(weightAccessor);
	//	const tinygltf::BufferView& bufferView = playerModel->bufferViews.at(accessor.bufferView);
	//	const tinygltf::Buffer& buffer = playerModel->buffers.at(bufferView.buffer);

	//	int weightVecSize = accessor.count;
	//	Logger::Log(1, "%s: %i vec4 in WEIGHTS_0\n", __FUNCTION__, weightVecSize);
	//	m_weightVec.resize(weightVecSize);

	//	std::memcpy(m_weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset,
	//		bufferView.byteLength);
	//}

	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
		glm::vec3 camPos) override;

	void Update(float dt, bool isPaused, bool isTimeScaled);

	glm::vec3 GetPosition();

	float GetInitialYaw() const { return m_initialYaw; }

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
	void SetPlayerYaw(float val);

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

	void SetupGLTFMeshes(tinygltf::Model* model);

	

	void DrawGLTFModel(glm::mat4 viewMat, glm::mat4 projMat, glm::vec3 camPos);

	std::vector<GLuint> glTextures;


	std::vector<GLuint> LoadGLTFTextures(tinygltf::Model* model);

	static const void* getDataPointer(tinygltf::Model* model, const tinygltf::Accessor& accessor) {
		const tinygltf::BufferView& bufferView = model->bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model->buffers[bufferView.buffer];
		return &buffer.data[accessor.byteOffset + bufferView.byteOffset];
	}
	int m_animNum = 3;


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

	float m_movementSpeed = 30.5f;
	float m_velocity = 0.0f;

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_playerDualQuatSsBuffer{};

	PlayerState m_playerState = MOVING;

	float m_health = 100.0f;

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
