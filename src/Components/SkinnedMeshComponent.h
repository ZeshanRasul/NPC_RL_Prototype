#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tiny_gltf.h>

#include "Model/GltfNode.h"
#include "Model/GltfAnimationClip.h"
#include "Logger.h"

// Owns a tinygltf::Model and all skeletal animation state that was duplicated
// between Player and Enemy. Call Load() then InitSkeleton() before use.
class SkinnedMeshComponent {
public:
    bool Load(const std::string& filename);

    // Build the node tree and load animations from the already-loaded model.
    // Pass filterSkinNodes=true for models where child nodes that carry skin
    // metadata (skin != -1) confuse the skeleton traversal (Player model quirk).
    void InitSkeleton(bool filterSkinNodes = false);

    tinygltf::Model* GetModel() { return m_model.get(); }

    void PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards);
    void BlendAnimationFrame(int animNum, float time, float blendFactor);

    float GetAnimationEndTime(int animNum) const { return m_animClips.at(animNum)->GetClipEndTime(); }
    std::string GetClipName(int animNum) const { return m_animClips.at(animNum)->GetClipName(); }
    int GetAnimClipsSize() const { return static_cast<int>(m_animClips.size()); }
    int GetJointDualQuatsSize() const { return static_cast<int>(m_jointDualQuats.size()); }
    const std::vector<glm::mat2x4>& GetJointDualQuats() const { return m_jointDualQuats; }

    std::string GetNodeName(int nodeNum) const;
    void ResetNodeData();

private:
    std::unique_ptr<tinygltf::Model> m_model;
    bool m_filterSkinNodes = false;

    std::vector<std::shared_ptr<GltfNode>> m_rootNodes{};
    std::vector<std::shared_ptr<GltfNode>> m_nodeList{};
    int m_nodeCount = 0;

    std::vector<glm::tvec4<uint16_t>> m_jointVec{};
    std::vector<glm::vec4> m_weightVec{};
    std::vector<glm::mat4> m_inverseBindMatrices{};
    std::vector<glm::mat4> m_jointMatrices{};
    std::vector<glm::mat2x4> m_jointDualQuats{};
    std::vector<int> m_attribAccessors{};
    std::vector<int> m_nodeToJoint{};

    std::vector<std::shared_ptr<GltfAnimationClip>> m_animClips{};
    std::vector<bool> m_additiveAnimationMask{};
    std::vector<bool> m_invertedAdditiveAnimationMask{};

    void GetJointData();
    void GetWeightData();
    void GetInvBindMatrices();
    void GetNodes(std::shared_ptr<GltfNode> treeNode);
    void GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
    void GetAnimations();
    void UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
    void UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode);
    void ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix);
};
