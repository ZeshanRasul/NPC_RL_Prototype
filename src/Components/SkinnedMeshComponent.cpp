#include "SkinnedMeshComponent.h"

bool SkinnedMeshComponent::Load(const std::string& filename)
{
    m_model = std::make_unique<tinygltf::Model>();

    tinygltf::TinyGLTF gltfLoader;
    std::string errors;
    std::string warnings;

    bool result = gltfLoader.LoadBinaryFromFile(m_model.get(), &errors, &warnings, filename);

    if (!warnings.empty())
        Logger::Log(1, "%s: warnings loading '%s':\n%s\n", __FUNCTION__, filename.c_str(), warnings.c_str());
    if (!errors.empty())
        Logger::Log(1, "%s: errors loading '%s':\n%s\n", __FUNCTION__, filename.c_str(), errors.c_str());
    if (!result)
        Logger::Log(1, "%s: could not load '%s'\n", __FUNCTION__, filename.c_str());

    return result;
}

void SkinnedMeshComponent::InitSkeleton(bool filterSkinNodes)
{
    m_filterSkinNodes = filterSkinNodes;

    if (!m_model || m_model->skins.empty())
        return;

    GetJointData();
    GetWeightData();
    GetInvBindMatrices();

    m_nodeCount = static_cast<int>(m_model->nodes.size());
    Logger::Log(1, "%s: model has %i nodes\n", __FUNCTION__, m_nodeCount);
    Logger::Log(1, "skin count = %zu\n", m_model->skins.size());

    for (size_t i = 0; i < m_model->skins.size(); ++i)
        Logger::Log(1, "skin %zu joint count = %zu\n", i, m_model->skins[i].joints.size());

    m_nodeList.resize(m_nodeCount);
    m_rootNodes.clear();

    for (const int rootNodeIdx : m_model->scenes.at(0).nodes)
    {
        Logger::Log(1, "%s: scene root node is %i\n", __FUNCTION__, rootNodeIdx);
        auto root = GltfNode::CreateRoot(rootNodeIdx);
        m_rootNodes.push_back(root);
        m_nodeList.at(rootNodeIdx) = root;
        GetNodeData(root, glm::mat4(1.0f));
        GetNodes(root);
        root->PrintTree();
    }

    GetAnimations();

    m_additiveAnimationMask.resize(m_nodeCount);
    m_invertedAdditiveAnimationMask.resize(m_nodeCount);
    std::fill(m_additiveAnimationMask.begin(), m_additiveAnimationMask.end(), true);
    m_invertedAdditiveAnimationMask = m_additiveAnimationMask;
    m_invertedAdditiveAnimationMask.flip();
}

std::string SkinnedMeshComponent::GetNodeName(int nodeNum) const
{
    if (nodeNum >= 0 && nodeNum < static_cast<int>(m_nodeList.size()) && m_nodeList.at(nodeNum))
        return m_nodeList.at(nodeNum)->GetNodeName();
    return "(Invalid)";
}

void SkinnedMeshComponent::ResetNodeData()
{
    for (auto& root : m_rootNodes)
    {
        if (!root) continue;
        GetNodeData(root, glm::mat4(1.0f));
        ResetNodeData(root, glm::mat4(1.0f));
    }
}

void SkinnedMeshComponent::ResetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
    glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();
    for (auto& childNode : treeNode->GetChilds())
    {
        GetNodeData(childNode, treeNodeMatrix);
        ResetNodeData(childNode, treeNodeMatrix);
    }
}

void SkinnedMeshComponent::PlayAnimation(int animNum, float speedDivider, float blendFactor, bool playBackwards)
{
    double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());

    if (playBackwards)
    {
        BlendAnimationFrame(animNum,
            m_animClips.at(animNum)->GetClipEndTime() -
            std::fmod(currentTime / 1000.0 * speedDivider, m_animClips.at(animNum)->GetClipEndTime()),
            blendFactor);
    }
    else
    {
        BlendAnimationFrame(animNum,
            std::fmod(currentTime / 1000.0 * speedDivider, m_animClips.at(animNum)->GetClipEndTime()),
            blendFactor);
    }
}

void SkinnedMeshComponent::BlendAnimationFrame(int animNum, float time, float blendFactor)
{
    m_animClips.at(animNum)->BlendAnimationFrame(m_nodeList, m_additiveAnimationMask, time, blendFactor);
    for (auto& root : m_rootNodes)
    {
        if (root)
            UpdateNodeMatrices(root, glm::mat4(1.0f));
    }
}

void SkinnedMeshComponent::UpdateNodeMatrices(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
    treeNode->CalculateNodeMatrix(parentNodeMatrix);
    UpdateJointMatricesAndQuats(treeNode);

    glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();
    for (auto& childNode : treeNode->GetChilds())
        UpdateNodeMatrices(childNode, treeNodeMatrix);
}

void SkinnedMeshComponent::UpdateJointMatricesAndQuats(std::shared_ptr<GltfNode> treeNode)
{
    const int nodeNum = treeNode->GetNodeNum();

    if (nodeNum < 0 || nodeNum >= static_cast<int>(m_nodeToJoint.size()))
        return;

    const int j = m_nodeToJoint[nodeNum];
    if (j < 0) return;

    if (j >= static_cast<int>(m_jointMatrices.size()) ||
        j >= static_cast<int>(m_inverseBindMatrices.size()) ||
        j >= static_cast<int>(m_jointDualQuats.size()))
    {
        Logger::Log(0, "%s: joint %d out of range\n", __FUNCTION__, j);
        return;
    }

    const glm::mat4 J = treeNode->GetNodeMatrix() * m_inverseBindMatrices[j];
    m_jointMatrices[j] = J;

    glm::quat orientation;
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;

    if (glm::decompose(J, scale, orientation, translation, skew, perspective))
    {
        glm::dualquat dq;
        dq.real = orientation;
        dq.dual = glm::quat(0.0f, translation) * orientation * 0.5f;
        m_jointDualQuats[j] = glm::mat2x4_cast(dq);
    }
}

void SkinnedMeshComponent::GetNodeData(std::shared_ptr<GltfNode> treeNode, glm::mat4 parentNodeMatrix)
{
    int nodeNum = treeNode->GetNodeNum();
    const tinygltf::Node& node = m_model->nodes.at(nodeNum);
    treeNode->SetNodeName(node.name);

    if (node.translation.size())
        treeNode->SetTranslation(glm::make_vec3(node.translation.data()));
    if (node.rotation.size())
        treeNode->SetRotation(glm::make_quat(node.rotation.data()));
    if (node.scale.size())
        treeNode->SetScale(glm::make_vec3(node.scale.data()));

    treeNode->CalculateLocalTrsMatrix();
    treeNode->CalculateNodeMatrix(parentNodeMatrix);
    UpdateJointMatricesAndQuats(treeNode);
}

void SkinnedMeshComponent::GetNodes(std::shared_ptr<GltfNode> treeNode)
{
    int nodeNum = treeNode->GetNodeNum();
    std::vector<int> childNodes = m_model->nodes.at(nodeNum).children;

    if (m_filterSkinNodes)
    {
        auto removeIt = std::remove_if(childNodes.begin(), childNodes.end(),
            [&](int num) { return m_model->nodes.at(num).skin != -1; });
        childNodes.erase(removeIt, childNodes.end());
    }

    treeNode->AddChilds(childNodes);
    glm::mat4 treeNodeMatrix = treeNode->GetNodeMatrix();

    for (auto& childNode : treeNode->GetChilds())
    {
        m_nodeList.at(childNode->GetNodeNum()) = childNode;
        GetNodeData(childNode, treeNodeMatrix);
        GetNodes(childNode);
    }
}

void SkinnedMeshComponent::GetAnimations()
{
    for (const auto& anim : m_model->animations)
    {
        Logger::Log(1, "%s: loading animation '%s' with %zu channels\n",
            __FUNCTION__, anim.name.c_str(), anim.channels.size());
        auto clip = std::make_shared<GltfAnimationClip>(anim.name);
        for (const auto& channel : anim.channels)
            clip->AddChannel(m_model.get(), anim, channel);
        m_animClips.push_back(clip);
    }
}

void SkinnedMeshComponent::GetJointData()
{
    const std::string attr = "JOINTS_0";
    m_jointVec.clear();

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

    for (size_t mi = 0; mi < m_model->meshes.size(); ++mi)
    {
        const auto& mesh = m_model->meshes[mi];
        for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
        {
            const auto& prim = mesh.primitives[pi];
            auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end())
                continue;

            int accIdx = it->second;
            const auto& acc = m_model->accessors[accIdx];
            const auto& bv  = m_model->bufferViews[acc.bufferView];
            const auto& buf = m_model->buffers[bv.buffer];

            const uint8_t* srcBase = buf.data.data() + bv.byteOffset + acc.byteOffset;
            size_t stride = bv.byteStride;
            if (stride == 0) stride = elemSize(acc.type, acc.componentType);

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
                if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    const uint8_t* v = reinterpret_cast<const uint8_t*>(s);
                    j = glm::u16vec4(v[0], v[1], v[2], v[3]);
                }
                else
                {
                    const uint16_t* v = reinterpret_cast<const uint16_t*>(s);
                    j = glm::u16vec4(v[0], v[1], v[2], v[3]);
                }
                m_jointVec[start + k] = j;
            }

            Logger::Log(1, "%s: mesh %zu prim %zu JOINTS_0 acc %d count %zu\n",
                __FUNCTION__, mi, pi, accIdx, static_cast<size_t>(acc.count));
        }
    }

    const tinygltf::Skin& skin = m_model->skins.at(0);
    m_nodeToJoint.assign(m_model->nodes.size(), -1);
    for (int i = 0; i < static_cast<int>(skin.joints.size()); ++i)
    {
        int jointNode = skin.joints[i];
        if (jointNode >= 0 && jointNode < static_cast<int>(m_nodeToJoint.size()))
            m_nodeToJoint[jointNode] = i;
    }

    m_inverseBindMatrices.resize(skin.joints.size());
    m_jointMatrices.resize(skin.joints.size());
    m_jointDualQuats.resize(skin.joints.size());
}

void SkinnedMeshComponent::GetWeightData()
{
    const std::string attr = "WEIGHTS_0";
    m_weightVec.clear();

    auto elemSize = [](int gltfType, int compType) -> size_t {
        int ncomp = (gltfType == TINYGLTF_TYPE_SCALAR) ? 1 :
                    (gltfType == TINYGLTF_TYPE_VEC2) ? 2 :
                    (gltfType == TINYGLTF_TYPE_VEC3) ? 3 :
                    (gltfType == TINYGLTF_TYPE_VEC4) ? 4 : 0;
        size_t csize = (compType == TINYGLTF_COMPONENT_TYPE_BYTE ||
                        compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) ? 1 :
                       (compType == TINYGLTF_COMPONENT_TYPE_SHORT ||
                        compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ? 2 :
                       (compType == TINYGLTF_COMPONENT_TYPE_INT ||
                        compType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
                        compType == TINYGLTF_COMPONENT_TYPE_FLOAT) ? 4 : 0;
        return ncomp * csize;
    };

    for (size_t mi = 0; mi < m_model->meshes.size(); ++mi)
    {
        const auto& mesh = m_model->meshes[mi];
        for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
        {
            const auto& prim = mesh.primitives[pi];
            auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end())
                continue;

            int accIdx = it->second;
            const auto& acc = m_model->accessors[accIdx];
            const auto& bv  = m_model->bufferViews[acc.bufferView];
            const auto& buf = m_model->buffers[bv.buffer];

            const uint8_t* srcBase = buf.data.data() + bv.byteOffset + acc.byteOffset;
            size_t stride = bv.byteStride;
            if (stride == 0) stride = elemSize(acc.type, acc.componentType);

            if (acc.type != TINYGLTF_TYPE_VEC4)
            {
                Logger::Log(1, "%s: WEIGHTS_0 must be vec4\n", __FUNCTION__);
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
                    Logger::Log(1, "%s: unexpected WEIGHTS_0 component type\n", __FUNCTION__);
                    continue;
                }

                float sum = w.x + w.y + w.z + w.w;
                if (sum > 0.0f) w /= sum;
                m_weightVec[start + k] = w;
            }

            Logger::Log(1, "%s: mesh %zu prim %zu WEIGHTS_0 acc %d count %zu\n",
                __FUNCTION__, mi, pi, accIdx, static_cast<size_t>(acc.count));
        }
    }
}

void SkinnedMeshComponent::GetInvBindMatrices()
{
    const tinygltf::Skin& skin = m_model->skins.at(0);
    int invBindMatAccessor = skin.inverseBindMatrices;

    const tinygltf::Accessor&   accessor   = m_model->accessors.at(invBindMatAccessor);
    const tinygltf::BufferView& bufferView = m_model->bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer&     buffer     = m_model->buffers.at(bufferView.buffer);

    m_inverseBindMatrices.resize(skin.joints.size());
    m_jointMatrices.resize(skin.joints.size());
    m_jointDualQuats.resize(skin.joints.size());

    std::memcpy(
        m_inverseBindMatrices.data(),
        buffer.data.data() + bufferView.byteOffset + accessor.byteOffset,
        sizeof(glm::mat4) * accessor.count
    );
}
