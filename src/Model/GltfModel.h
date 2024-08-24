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
    bool loadModel(RenderData& renderData, std::string modelFilename,
        std::string textureFilename);
    void draw();
    void cleanup();
    void uploadVertexBuffers();
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

private:
    void createVertexBuffers();
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

    std::vector<std::shared_ptr<GltfAnimationClip>> mAnimClips{};
    size_t animClipsSize;

    GLuint mVAO = 0;
    std::vector<GLuint> mVertexVBO{};
    GLuint mIndexVBO = 0;
    std::map<std::string, GLint> attributes =
    { {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, { "JOINTS_0", 3 }, { "WEIGHTS_0", 4 }
};

    Texture mTex{};
};