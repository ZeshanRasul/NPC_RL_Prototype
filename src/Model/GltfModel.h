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

class GltfModel {
public:
	std::shared_ptr<GltfModel> clone() const;

	bool loadModel(std::string modelFilename, bool isEnemy = false);
	bool loadModelNoAnim(std::string modelFilename);
	Texture loadTexture(std::string textureFilename, bool flip);

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

	void playAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
	void playAnimation(int sourceAnimNum, int destAnimNum, float speedDivider, float blendFactor, bool playBackwards);
	void blendAnimationFrame(int animNumber, float time, float blendFactor);
	void crossBlendAnimationFrame(int sourceAnimNumber, int destAnimNumber, float time, float blendFactor);
	float getAnimationEndTime(int animNum);
	std::string getClipName(int animNum);
	int getAnimClipsSize() const { return (int)mAnimClips.size(); }

	void resetNodeData();
	void setSkeletonSplitNode(int nodeNum);
	std::string getNodeName(int nodeNum);
	int getNodeCount() const { return mNodeCount; }

	std::vector<glm::vec3> getVertices() { return mVertices; }
	Texture* getTexture() { return &mTex; }

	std::string filename;

private:
	void createVertexBuffers(bool isEnemy = false);
	void createIndexBuffer();
	int getTriangleCount();
	void getSkeletonPerNode(std::shared_ptr<GltfNode> treeNode, bool enableSkinning);

	void getJointData();
	void getWeightData();
	void getInvBindMatrices();
	void getAnimations();
	void getNodes(std::shared_ptr<GltfNode> treeNode);
	void getNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	void resetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);

	void updateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
	void updateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode);
	void updateAdditiveMask(std::shared_ptr<GltfNode> treeNode, int splitNodeNum);

	std::vector<glm::tvec4<uint16_t>> mJointVec{};
	std::vector<glm::vec4> mWeightVec{};
	std::vector<glm::mat4> mInverseBindMatrices{};
	std::vector<glm::mat4> mJointMatrices{};
	std::vector<glm::mat2x4> mJointDualQuats{};

	std::vector<int> mAttribAccessors{};
	std::vector<int> mNodeToJoint{};

	std::shared_ptr<GltfNode> mRootNode = nullptr;
	std::shared_ptr<tinygltf::Model> mModel = nullptr;

	std::shared_ptr<Mesh> mSkeletonMesh = nullptr;

	std::vector<std::shared_ptr<GltfNode>> mNodeList;
	int mNodeCount = 0;

	std::vector<std::shared_ptr<GltfAnimationClip>> mAnimClips{};
	size_t animClipsSize;

	std::vector<bool> mAdditiveAnimationMask{};
	std::vector<bool> mInvertedAdditiveAnimationMask{};

	std::vector<glm::vec3> mVertices{};
	GLuint mVAO = 0;
	std::vector<GLuint> mVertexVBO{};
	GLuint mIndexVBO = 0;
	std::map<std::string, GLint> attributes =
	{ {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, { "TEXCOORD_1", 3 }, { "COLOR_0", 4 }, { "COLOR_1", 5 }, { "JOINTS_0", 6 }, { "WEIGHTS_0", 7 }, {"TANGENT", 8} };

	std::map<std::string, GLint> enemyAttributes =
	{ {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, { "TEXCOORD_1", 3 }, { "JOINTS_0", 4 }, { "WEIGHTS_0", 5 }, { "TANGENT", 6 } };


	Texture mTex{};
};