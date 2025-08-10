#pragma once
#include <memory>
#include <random>

#include "GameObject.h"
#include "Player.h"
#include "src/OpenGL/ShaderStorageBuffer.h"
#include "Physics/AABB.h"
#include "Components/AudioComponent.h"
#include "AI/BehaviourTree.h"
#include "AI/Event.h"
#include "AI/Events.h"
#include "AI/ConditionNode.h"
#include "AI/ActionNode.h"
#include "Logger.h"

enum Action
{
	PATROL,
	RETREAT,
	ADVANCE,
	ATTACK
};

struct State
{
	bool playerDetected;
	bool playerVisible;
	float distanceToPlayer;
	float health;
	bool isSuppressionFire;

	bool operator==(const State& other) const
	{
		return playerDetected == other.playerDetected &&
			playerVisible == other.playerVisible &&
			distanceToPlayer == other.distanceToPlayer &&
			isSuppressionFire == other.isSuppressionFire &&
			health == other.health;
	}
};


// Custom hash function for State and Action pair
struct PairHash
{
	std::size_t operator()(const std::pair<State, Action>& pair) const
	{
		const State& state = pair.first;
		Action action = pair.second;
		return ((std::hash<bool>()(state.playerDetected) ^ (std::hash<bool>()(state.playerVisible) << 1)) >> 1) ^
			(std::hash<bool>()(state.isSuppressionFire) << 1) ^ (std::hash<int>()(static_cast<int>(state.health)) << 2)
			^
			std::hash<int>()(action);
	}
};

class Enemy : public GameObject
{
public:
	Enemy(glm::vec3 pos, glm::vec3 scale, Shader* sdr, Shader* shadowMapShader, bool applySkinning,
		GameManager* gameMgr, std::string texFilename, int id, EventManager& eventManager, Player& player,
		float yaw = 0.0f);

	~Enemy()
	{
		m_model->cleanup();
	}

	void SetUpModel();

	std::vector<glm::mat2x4> getJointDualQuats()
	{
		return m_jointDualQuats;
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
				std::fmod(currentTime / 1000.0 * speedDivider,
					m_animClips.at(animNum)->GetClipEndTime()), blendFactor);
		}
		else
		{
			BlendAnimationFrame(animNum, std::fmod(currentTime / 1000.0 * speedDivider,
				m_animClips.at(animNum)->GetClipEndTime()), blendFactor);
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
		const tinygltf::Skin& skin = enemyModel->skins.at(0);
		int invBindMatAccessor = skin.inverseBindMatrices;

		const tinygltf::Accessor& accessor = enemyModel->accessors.at(invBindMatAccessor);
		const tinygltf::BufferView& bufferView = enemyModel->bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = enemyModel->buffers.at(bufferView.buffer);

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
		const tinygltf::Node& node = enemyModel->nodes.at(nodeNum);
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

		for (size_t mi = 0; mi < enemyModel->meshes.size(); ++mi)
		{
			const auto& mesh = enemyModel->meshes[mi];
			for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
			{
				const auto& prim = mesh.primitives[pi];
				auto it = prim.attributes.find(attr);
				if (it == prim.attributes.end())
					continue; // rigid primitive

				int accIdx = it->second;
				const auto& acc = enemyModel->accessors[accIdx];
				const auto& bv = enemyModel->bufferViews[acc.bufferView];
				const auto& buf = enemyModel->buffers[bv.buffer];

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
		const tinygltf::Skin& skin = enemyModel->skins.at(0);
		m_nodeToJoint.assign(enemyModel->nodes.size(), -1);
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

	tinygltf::Model* enemyModel;

	struct GLTFMesh {
		std::vector<GLTFPrimitive> primitives;
	};

	std::vector<GLTFMesh> meshData;

	void GetNodes(std::shared_ptr<GltfNode> treeNode)
	{
		int nodeNum = treeNode->GetNodeNum();
		std::vector<int> childNodes = enemyModel->nodes.at(nodeNum).children;

		/* remove the child node with skin/mesh metadata, confuses skeleton */
		auto removeIt = std::remove_if(childNodes.begin(), childNodes.end(),
			[&](int num) { return enemyModel->nodes.at(num).skin != -1; }
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
		for (const auto& anim : enemyModel->animations)
		{
			Logger::Log(1, "%s: loading animation '%s' with %i channels\n", __FUNCTION__, anim.name.c_str(),
				anim.channels.size());
			auto clip = std::make_shared<GltfAnimationClip>(anim.name);
			for (const auto& channel : anim.channels)
			{
				clip->AddChannel(enemyModel, anim, channel);
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


	void DrawObject(glm::mat4 viewMat, glm::mat4 proj, bool shadowMap, glm::mat4 lightSpaceMat, GLuint shadowMapTexture,
		glm::vec3 camPos) override;

	void Update(bool shouldUseEDBT, bool isPaused, bool isTimeScaled);

	void OnEvent(const Event& event);

	int GetID() const { return m_id; }

	AudioComponent* GetAudioComponent() const { return m_takeDamageAc; }

	glm::vec3 GetPosition()
	{
		return m_position;
	}

	glm::vec3 GetInitialPosition() const { return m_initialPosition; }

	void SetPosition(glm::vec3 newPos);

	void ComputeAudioWorldTransform() override;

	void UpdateEnemyCameraVectors();

	void UpdateEnemyVectors();

	void EnemyProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

	void MoveEnemy(const std::vector<glm::ivec2>& path, float deltaTime, float blendFactor, bool playAnimBackwards);

	void SetAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
	void SetAnimation(int srcAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards);

	void SetYaw(float newYaw) { m_yaw = newYaw; }

	void Shoot();

	float GetHealth() const { return m_health; }
	void SetHealth(float newHealth) { m_health = newHealth; }

	void TakeDamage(float damage);
	void OnDeath();
	void SetIsDead(bool newValue) { m_isDead = newValue; }

	void SetAABBShader(Shader* aabbShdr) { m_aabbShader = aabbShdr; }
	void SetUpAABB();
	AABB* GetAABB() const { return m_aabb; }
	void SetAABBColor(glm::vec3 color) { m_aabbColor = color; }
	void UpdateAABB();

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

	glm::vec3 GetEnemyShootPos(float forwardOffset)
	{
		UpdateEnemyVectors();
		return GetPosition() + glm::vec3(0.0f, 4.5f, 0.0f) + (forwardOffset * m_front);
	}

	glm::vec3 GetEnemyShootDir() const { return m_enemyShootDir; }
	glm::vec3 GetEnemyHitPoint() const { return m_enemyHitPoint; }
	bool GetEnemyHasShot() const { return m_enemyHasShot; }
	bool GetEnemyHasHit() const { return m_enemyHasHit; }
	float GetEnemyShootDistance() const { return m_enemyShootDistance; }
	float GetEnemyDebugRayRenderTimer() const { return m_enemyRayDebugRenderTimer; }

	void SetDeltaTime(float newDt) { m_dt = newDt; }

	std::string GetEDBTState() const { return m_state; }

	glm::vec3 GetEnemyFront() const { return m_enemyFront; }

	void Speak(const std::string& clipName, float priority, float cooldown);

	void OnHit() override;

	void OnMiss() override
	{
		m_aabbColor = glm::vec3(1.0f, 1.0f, 1.0f);
	}

	bool IsDead();

	void ScoreCoverLocations(Player& player);

	glm::vec3 SelectRandomWaypoint(const glm::vec3& currentWaypoint, const std::vector<glm::vec3>& allWaypoints);

	// Function to perform enemy decision-making during an attack using Nash Q-learning
	void EnemyDecision(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void EnemyDecisionPrecomputedQ(State& currentState, int enemyId, std::vector<Action>& squadActions,
		float deltaTime,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	void ResetState();

private:
	// NASH LEARNING START
	const float m_learningRate = 0.05f;
	const float m_discountFactor = 0.95f;
	float m_explorationRate;
	float m_initialExplorationRate = 0.7f;
	float m_minExplorationRate = 0.1f;
	int m_targetQTableSize = 1000000;
	Action m_chosenAction;
	float m_decisionDelayTimer = 0.0f;


	Player& m_player;
	const float BUCKET_SIZE = 10.0f;
	const float TOLERANCE = 10.0f;

	float DecayExplorationRate(float initialRate, float minRate, int currentSize, int targetSize);

	int GetDistanceBucket(float distance)
	{
		return static_cast<int>(distance / BUCKET_SIZE);
	}

	float CalculateReward(const State& state, Action action, int enemyId, const std::vector<Action>& squadActions);

	float GetMaxQValue(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	// Choose an action based on epsilon-greedy strategy for a specific enemy
	Action ChooseAction(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	// Update Q-value for a given state-action pair for a specific enemy
	void UpdateQValue(const State& currentState, Action action, const State& nextState, float reward,
		int enemyId, std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);


	// USING PRECOMPUTED Q-VALUES START

	// Choose an action based on the highest Q-value for a specific enemy
	Action ChooseActionFromTrainedQTable(const State& state, int enemyId,
		std::unordered_map<std::pair<State, Action>, float, PairHash>* qTable);

	// USING PRECOMPUTED Q-VALUES END

	// NASH LEARNING EMD

	void HasDealtDamage() override;
	void HasKilledPlayer() override;

	Texture m_normal{};
	Texture m_metallic{};
	Texture m_roughness{};
	Texture m_ao{};
	Texture m_emissive{};

	glm::vec3 m_initialPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	int m_id;
	EventManager& m_eventManager;
	BTNodePtr m_behaviorTree;

	std::vector<glm::vec3> m_waypointPositions = {
	//m_grid->SnapToGrid(glm::vec3(0.0f, 0.0f, 0.0f)),
	//m_grid->SnapToGrid(glm::vec3(0.0f, 0.0f, 70.0f)),
	//m_grid->SnapToGrid(glm::vec3(40.0f, 0.0f, 0.0f)),
	//m_grid->SnapToGrid(glm::vec3(40.0f, 0.0f, 70.0f))
	};

	float m_health = 100.0f;
	bool m_isPlayerDetected;
	bool m_isPlayerVisible;
	bool m_isPlayerInRange;
	bool m_isTakingDamage;
	bool m_hasTakenDamage = false;
	bool m_isDying = false;
	bool m_isDead = false;
	bool m_hasDied = false;
	bool m_isInCover;
	bool m_isSeekingCover;
	bool m_isTakingCover;
	bool m_isAttacking = false;
	bool m_hasDealtDamage = false;
	bool m_hasKilledPlayer = false;
	bool m_isPatrolling = false;
	bool m_provideSuppressionFire = false;
	bool m_allyHasDied = false;

	int m_numDeadAllies = 0;

	std::vector<glm::ivec2> m_currentPath;
	float m_dt = 0.0f;
	size_t m_pathIndex = 0;
	size_t m_prevPathIndex = 0;
	std::vector<glm::ivec2> m_prevPath = {};

	glm::vec3 m_aabbColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_aabbScale = glm::vec3(3.8f, 3.3f, 3.5f);

	float m_speed = 7.5f;
	float m_enemyCameraYaw;
	float m_enemyCameraPitch = 10.0f;
	glm::vec3 m_enemyFront;
	glm::vec3 m_enemyRight;
	glm::vec3 m_enemyUp;
	glm::vec3 m_front;
	glm::vec3 m_right;
	glm::vec3 m_up;
	bool m_reachedDestination = false;
	bool m_reachedPlayer = false;
	glm::vec3 m_currentWaypoint;
	std::string m_state = "Patrolling";

	bool m_uploadVertexBuffer = true;
	ShaderStorageBuffer m_enemyDualQuatSsBuffer{};

	AABB* m_aabb;
	Shader* m_aabbShader;

	std::vector<glm::vec3> verts;

	AudioComponent* m_takeDamageAc;
	AudioComponent* m_shootAc;
	AudioComponent* m_deathAc;

	int m_animNum = 1;
	int m_sourceAnim = 1;
	int m_destAnim = 1;
	bool m_destAnimSet = false;
	float m_blendSpeed = 5.0f;
	float m_blendFactor = 0.0f;
	bool m_blendAnim = false;
	bool m_resetBlend = false;

	bool m_takingDamage = false;
	float m_damageTimer = 0.0f;
	float m_dyingTimer = 0.0f;
	float m_coverTimer = 0.0f;
	bool m_reachedCover = false;
	float m_shootAudioCooldown = 0.0f;

	float m_accuracy = 60.0f;
	glm::vec3 m_enemyShootPos = glm::vec3(0.0f);
	glm::vec3 m_enemyShootDir = glm::vec3(0.0f);
	glm::vec3 m_enemyHitPoint = glm::vec3(0.0f);
	float m_enemyShootDistance = 100000.0f;
	float m_enemyShootCooldown = 0.0f;
	float m_enemyRayDebugRenderTimer = 0.1f;
	bool m_enemyHasShot = false;
	bool m_enemyHasHit = false;
	bool m_playerIsVisible = false;

	bool m_startingSuppressionFire = true;
	bool m_playNotVisibleAudio = true;

	void VacatePreviousCell();

	void BuildBehaviorTree();

	void DetectPlayer();

	bool IsHealthZeroOrBelow();
	bool IsTakingDamage();
	bool IsPlayerDetected();
	bool IsPlayerVisible();
	bool IsCooldownComplete();
	bool IsHealthBelowThreshold();
	bool IsPlayerInRange();
	bool IsTakingCover();
	bool IsInCover();
	bool IsAttacking();
	bool IsPatrolling();
	bool ShouldProvideSuppressionFire();


	NodeStatus EnterDyingState();
	NodeStatus EnterTakingDamageState();
	NodeStatus AttackShoot();
	NodeStatus AttackChasePlayer();
	NodeStatus SeekCover();
	NodeStatus TakeCover();
	NodeStatus EnterInCoverState();
	NodeStatus Patrol();
	NodeStatus InCoverAction();
	NodeStatus Die();
};
