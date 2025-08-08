#pragma once
#include <vector>
#include <string>
#include "glm/glm.hpp"

class GLTFPrimitive {

public:
	GLuint vao;
	GLuint indexBuffer;
	GLsizei indexCount;
	GLsizei vertexCount;
	GLenum indexType;
	GLenum mode;
	int material;
	std::vector<glm::vec3> verts;
	std::vector<unsigned int> indices;

	GLTFPrimitive() = default;
	///////////// ANIMATION AND MODEL \\\\\\\\\\\\\\\\\\\\\\\

//std::shared_ptr<Mesh> getSkeleton(bool enableSkinning);
	int getPrimJointMatrixSize()
	{
		return (int)m_jointMatrices.size();
	}
	std::vector<glm::mat4> getJointMatrices();
	int getJointDualQuatsSize();
	std::vector<glm::mat2x4> getJointDualQuats();

	void PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
	void PlayAnimation(int sourceAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards);
	void BlendAnimationFrame(int animNumber, float time, float blendFactor);
	void CrossBlendAnimationFrame(int sourceAnimNumber, int destAnimNumber, float time, float blendFactor);
	float GetAnimationEndTime(int animNum);
	std::string GetClipName(int animNum);
	int GetAnimClipsSize() const { return static_cast<int>(m_animClips.size()); }

	void ResetNodeData();
	std::string GetNodeName(int nodeNum);
	int GetNodeCount() const { return m_nodeCount; }

	//void GetAnimations();
	//void GetJointData();
	//void GetWeightData();
	//void GetInvBindMatrices();
	//void GetNodes(std::shared_ptr<GltfNode> treeNode);
	//void GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	//void ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);

	void UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode);
	void UpdateAdditiveMask(std::shared_ptr<GltfNode> treeNode, int splitNodeNum);


	//////////// ANIMATION AND MODEL \\\\\\\\\\\\\\\\\\\\\\\


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

};