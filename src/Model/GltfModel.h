#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <tiny_gltf.h>

#include "Texture.h"

#include "src/OpenGL/RenderData.h"
#include "Model/GltfNode.h"

class GltfModel {
public:
    bool loadModel(RenderData& renderData, std::string modelFilename,
        std::string textureFilename);
    void draw();
    void cleanup();
    void uploadVertexBuffers();
    void applyVertexSkinning(bool enableSkinning);
    void uploadPositionBuffer();
    void uploadIndexBuffer();
    std::shared_ptr<Mesh> getSkeleton(bool enableSkinning);


private:
    void createVertexBuffers();
    void createIndexBuffer();
    int getTriangleCount();
    void getSkeletonPerNode(std::shared_ptr<GltfNode> treeNode, bool enableSkinning);

    void getJointData();
    void getWeightData();
    void getInvBindMatrices();
    void getNodes(std::shared_ptr<GltfNode> treeNode);
    void getNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);

    std::vector<glm::tvec4<uint16_t>> mJointVec{};
    std::vector<glm::vec4> mWeightVec{};
    std::vector<glm::mat4> mInverseBindMatrices{};
    std::vector<glm::mat4> mJointMatrices{};

    std::vector<int> mAttribAccessors{};
    std::vector<int> mNodeToJoint{};

    std::vector<glm::vec3> mAlteredPositions{};

    std::shared_ptr<GltfNode> mRootNode = nullptr;
    std::shared_ptr<tinygltf::Model> mModel = nullptr;

    std::shared_ptr<Mesh> mSkeletonMesh = nullptr;

    GLuint mVAO = 0;
    std::vector<GLuint> mVertexVBO{};
    GLuint mIndexVBO = 0;
    std::map<std::string, GLint> attributes =
    { {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2} };

    Texture mTex{};
};