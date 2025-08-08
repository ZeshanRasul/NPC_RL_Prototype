#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <tiny_gltf.h>

#include "Texture.h"

#include "src/OpenGL/RenderData.h"
#include "Model/GltfNode.h"
#include "Model/GltfAnimationClip.h"

class GltfModel
{
public:
	std::shared_ptr<GltfModel> Clone() const;

	bool LoadModelNoAnim(std::string modelFilename);
	bool LoadModel(std::string modelFilename, bool isEnemy = false);
	Texture LoadTexture(std::string textureFilename, bool flip);

	void draw(Texture tex);
	void drawNoTex();
	void cleanup();
	void uploadVertexBuffers();
	void uploadEnemyVertexBuffers();
	void uploadVertexBuffersMap();
	void uploadVertexBuffersNoAnimations();
	void uploadIndexBuffer();
	std::shared_ptr<Mesh> getSkeleton(bool enableSkinning);
	int getJointMatrixSize();
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

	std::vector<glm::vec3> GetVertices() { return m_vertices; }
	Texture* GetTexture() { return &m_tex; }
	std::string GetFilename() const { return m_filename; }

	std::string filename;




private:
	void CreateVertexBuffers(bool isEnemy = false);
	void CreateIndexBuffer();
	int GetTriangleCount();

	void GetAnimations();
	void GetJointData();
	void GetWeightData();
	void GetInvBindMatrices();
	void GetNodes(std::shared_ptr<GltfNode> treeNode);
	void GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	void ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);

	void UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode);
	void UpdateAdditiveMask(std::shared_ptr<GltfNode> treeNode, int splitNodeNum);

	std::string m_filename;
	std::vector<glm::tvec4<uint16_t>> m_jointVec{};
	std::vector<glm::vec4> m_weightVec{};
	std::vector<glm::mat4> m_inverseBindMatrices{};
	std::vector<glm::mat4> m_jointMatrices{};
	std::vector<glm::mat2x4> m_jointDualQuats{};

	std::vector<int> m_attribAccessors{};
	std::vector<int> m_nodeToJoint{};

	std::shared_ptr<GltfNode> m_rootNode = nullptr;
	std::shared_ptr<tinygltf::Model> m_model = nullptr;

	std::vector<std::shared_ptr<GltfNode>> m_nodeList;
	int m_nodeCount = 0;

	std::vector<std::shared_ptr<GltfAnimationClip>> m_animClips{};
	size_t m_clipsSize;

	std::vector<bool> m_additiveAnimationMask{};
	std::vector<bool> m_invertedAdditiveAnimationMask{};

	std::vector<glm::vec3> m_vertices{};
	GLuint m_vao = 0;
	std::vector<GLuint> m_vertexVbo{};
	GLuint m_indexVbo = 0;
	std::map<std::string, GLint> m_attributes =
	{
		{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"TEXCOORD_1", 3}, {"COLOR_0", 4}, {"COLOR_1", 5},
		{"JOINTS_0", 6}, {"WEIGHTS_0", 7}, {"TANGENT", 8}
	};

	std::map<std::string, GLint> m_enemyAttributes =
	{
		{"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"TEXCOORD_1", 3}, {"JOINTS_0", 4}, {"WEIGHTS_0", 5},
		{"TANGENT", 6}
	};


	Texture m_tex{};
};
